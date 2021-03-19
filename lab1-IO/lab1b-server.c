#include <stdbool.h>
#include <unistd.h> // for read, write
#include <stdio.h> // for printf()
#include <getopt.h> // for getopt_long()
#include <stdlib.h> // for exit(), atoi()
#include <poll.h> // for poll()
#include <signal.h> // for SIGINT (maybe)
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <zlib.h>

struct option opts[] =
	{
		{"port", 1, NULL, 'p'},
		{"compress", 0, NULL, 'c'},
		{0, 0, 0, 0}			
	};
struct Args {
    int port;
    bool comp;
};

#define BUFSIZE 1024
int pid;
int to_term, from_term; // file descriptors on the two pipes
int socket_fd;
bool comp = false;

void sigpipe_handler(int sig);
struct Args get_args(int argc, char **argv);
int server_connect(unsigned int port_num);
void fork_bash();
void read_socket();
void read_bash();
void read_batch_bash();
void read_batch_socket();
void zip(char *in, char *out, size_t in_size, size_t *out_size);
void unzip(char *in, char *out, size_t in_size, size_t *out_size);
void done(int status);
int mywrite(int fd, void* buf, size_t size);
ssize_t myread(int fd, void *buf, size_t nbytes);
int mykill(pid_t pid, int sig);
pid_t mywaitpid(pid_t pid, int *stat_loc, int options);
int myshutdown(int socket, int how);

int main(int argc, char **argv) {	
    struct Args args;
    args = get_args(argc, (char **)argv);
    socket_fd = server_connect(args.port);
    fork_bash();
}

int server_connect(unsigned int port_num) {
    int sockfd, new_fd; /* listen on sock_fd, new connection on new_fd */ 
    struct sockaddr_in my_addr; /* my address */
    struct sockaddr_in their_addr; /* connector addr */
    int sin_size;
    /* create a socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { 
        //some error handling
        fprintf(stderr, "ERROR: 'socket()' - %s\n", strerror(errno));
        exit(1);
    }
    /* set the address info */
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port_num); /* short, network byte order */
    my_addr.sin_addr.s_addr = INADDR_ANY;
    // INADDR_ANY allows clients to connect to any one of the host’s IP address.
    
    memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero)); //padding zeros
    
    /* bind the socket to the IP address and port number */
    if (bind(sockfd, (struct sockaddr*) &my_addr, 
             sizeof(struct sockaddr_in)) == -1) {
        //some error handling
        fprintf(stderr, "socket bind error\n");
        exit(1);
    }

    if (listen(sockfd, 1) == -1) {/* maximum 1 pending connections */
        //some error handling
        fprintf(stderr, "socket listen error\n");
        exit(1);
    }

    sin_size = sizeof(struct sockaddr_in);
    /* wait for client’s connection, their_addr stores client’s address */
    if ((new_fd = accept(sockfd, (struct sockaddr*) &their_addr, (socklen_t*) &sin_size)) == -1) {
        fprintf(stderr, "ERROR: 'accept()' - %s\n", strerror(errno));
        exit(1);
    }
    return new_fd; /* new_fd is returned not sock_fd*/
}

void fork_bash() {
    int term_to_shell[2];
    int shell_to_term[2];
    pipe(term_to_shell);
    pipe(shell_to_term);
    int to_shell = term_to_shell[0];
    int from_shell = shell_to_term[1];
    from_term = term_to_shell[1];
    to_term = shell_to_term[0];

    pid = fork();
    if (pid == 0) { // child/shell
        close(from_term);
        close(to_term);

        close(0);
        dup(to_shell); // 0 -> to_shell
        close(to_shell);

        close(1);
        dup(from_shell); // 1 -> from_shell

        close(2);
        dup(from_shell); // 2 -> from_shell
        close(from_shell);

        execlp("/bin/bash", "bash", NULL);
    }
    else if (pid > 0) { // parent/terminal
        signal(SIGPIPE, sigpipe_handler);

        close(from_shell);
        close(to_shell);

        struct pollfd pollfds[2];
        pollfds[0].fd = socket_fd;
        pollfds[0].events = POLLIN + POLLHUP + POLLERR;
        pollfds[1].fd = to_term;
        pollfds[1].events = POLLIN + POLLHUP + POLLERR;

        while (1) {
            poll(pollfds, 2, -1); // -1 means no time out
            if (pollfds[0].revents & POLLIN) {
                // read from socket
                if (comp)
                    read_batch_socket();
                else 
                    read_socket();
            }
            if (pollfds[1].revents & POLLIN) {
                // read from bash
                if (comp)
                    read_batch_bash();
                else 
                    read_bash();
            }
            if (pollfds[0].revents & (POLLHUP | POLLERR)) {
                done(0);
            }
            if (pollfds[1].revents & (POLLHUP | POLLERR)) {
                done(0);
            }
        }
    }
    else {
        fprintf(stderr, "ERROR: 'fork()' - %s\n", strerror(errno));
        exit(1);
    }
}

void read_socket() {
    int ret, j;
    char buffer[BUFSIZE], c;

    ret = myread(socket_fd, buffer, sizeof(buffer));
    for (j = 0; j < ret; j++) {
        c = buffer[j];
        if (c == 0x3) { // ^C
            mykill(pid, SIGINT);
        }
        if (c == 0x4) { // ^D
            done(0);
        }
        mywrite(from_term, &c, sizeof(c));
    }
}

void read_bash() {
    int ret, j;
    char buffer[BUFSIZE], c;

    ret = myread(to_term, buffer, sizeof(buffer));
    for (j = 0; j < ret; j++) {
        c = buffer[j];
        if (c == 0x4) { // C-d
            printf("0x4\n");
            done(0);
        }
        mywrite(socket_fd, &c, sizeof(c));
    }
}

void read_batch_socket() {
    int ret;
    char in[BUFSIZE], out[BUFSIZE], c;

    ret = myread(socket_fd, in, sizeof(in));
    size_t size, j;
    unzip(in, out, ret, &size);
    for (j = 0; j < size; j++) {
        c = out[j];
        if (c == 0x3) { // ^C
            mykill(pid, SIGINT);
        }
        if (c == 0x4) { // ^D
            done(0);
        }
        mywrite(1, &c, sizeof(c));
        mywrite(from_term, &c, sizeof(c));
    }
}

void read_batch_bash() {
    int ret, j;
    char in[BUFSIZE], out[BUFSIZE], c;
    out[0] = '\0';
    ret = myread(to_term, in, sizeof(in));
    mywrite(1, in, ret);
    size_t size;
    zip(in, out, ret, &size);
    for (j = 0; j < ret; j++) {
        c = in[j];
        if (c == 0x4) { // C-d
            mywrite(socket_fd, out, size);
            done(0);
        }
    }
    mywrite(socket_fd, out, size);
}

void zip(char *in, char *out, size_t in_size, size_t *out_size) {
    int ret;
    z_stream strm;
    /* allocate deflate state */
    strm.zalloc = Z_NULL; // set to Z_NULL, to request zlib use the default memory allocation routines
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION); /* pointer to the structure to be initialized, and the compression level */
    if (ret != Z_OK) // make sure that it was able to allocate memory, and the provided arguments were valid. 
    {
        fprintf(stderr, "ERROR: 'deflateInit()' - %s\n", strerror(errno));
        exit(1);
    }
    strm.avail_in = in_size;/* number of bytes available at next_in */ 
    strm.next_in = (unsigned char*)in; /* next input byte*/
    strm.avail_out = BUFSIZE; /* remaining free space at next_out */ 
    strm.next_out = (unsigned char*)out; /* next output byte */
    do {
        deflate(&strm, Z_SYNC_FLUSH);
    } while (strm.avail_in > 0);
    *out_size = BUFSIZE - strm.avail_out;
    deflateEnd(&strm);
}

void unzip(char *in, char *out, size_t in_size, size_t *out_size) {
    int ret;
    z_stream strm;
    /* allocate deflate state */
    strm.zalloc = Z_NULL; // set to Z_NULL, to request zlib use the default memory allocation routines
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = inflateInit(&strm); /* pointer to the structure to be initialized, and the compression level */
    if (ret != Z_OK) // make sure that it was able to allocate memory, and the provided arguments were valid. 
    {
        fprintf(stderr, "ERROR: 'inflateInit()' - %s\n", strerror(errno));
        exit(1);
    }
    strm.avail_in = in_size;/* number of bytes available at next_in */ 
    strm.next_in = (unsigned char*)in; /* next input byte*/
    strm.avail_out = BUFSIZE; /* remaining free space at next_out */ 
    strm.next_out = (unsigned char*)out; /* next output byte */
    do {
        inflate(&strm, Z_SYNC_FLUSH);
    } while (strm.avail_in > 0);
    *out_size = BUFSIZE - strm.avail_out;
    inflateEnd(&strm);
}

struct Args get_args(int argc, char **argv) {
    int i;
    struct Args args;
    args.port = -1;
    args.comp = false;
    while((i = getopt_long(argc, argv, "", opts, NULL)) != -1) {
		switch(i) {
			case 'p':
				args.port = atoi(optarg);
				break;
			case 'c':
				args.comp = true;
                comp = true;
				break;
			default:
				fprintf(stderr, "usage: ./lab1b-server --port=num [--compress]\n");
				exit(1);
				break;
		}
	}
    if (args.port == -1) {
        fprintf(stderr, "ERROR: missing an argument for port number\n\
                        usage: ./lab1b-client --port=num [--compress]\n");
        exit(1);
    }
    return args;
}

void last_read(int exit_stat) {
    int ret, j;
    char buffer[BUFSIZE], c;

    while ((ret = read(to_term, buffer, sizeof(buffer))) > 0) {
        for (j = 0; j < ret; j++) {
            c = buffer[j];
            if (c == 0x4) { // C-d
                char out[] = "^D";
                mywrite(socket_fd, &out, sizeof(out));
                int wstatus;
                mywaitpid(pid, &wstatus, 0);
                fprintf(stderr, "SHELL EXIT, SIGNAL=%d STATUS=%d\n", 
                            WTERMSIG(wstatus), WEXITSTATUS(wstatus));
                exit(exit_stat);
            }
            else if (c == '\n') {
                char out[] = "\r\n";
                mywrite(socket_fd, &out, 2);
            }
            else {
                mywrite(socket_fd, &c, 1);
            }
        }
    }
}

void sigpipe_handler(int sig) {
	if (sig == SIGPIPE) {
        done(0);
	}
}

void done(int status) {
    close(from_term);
    last_read(status);
    myshutdown(socket_fd, SHUT_RDWR);
    int wstatus;
    mywaitpid(pid, &wstatus, 0);
    fprintf(stderr, "SHELL EXIT, SIGNAL=%d STATUS=%d\n", 
                    WTERMSIG(wstatus), WEXITSTATUS(wstatus));
    exit(status);
}

int mywrite(int fd, void* buf, size_t size) {
    int ret;
    ret = write(fd, buf, size);
    if (ret < 0) {
        fprintf(stderr, "ERROR: 'write()' - %s\n", strerror(errno));
        exit(1);
    }
    return ret;
}

ssize_t myread(int fd, void *buf, size_t nbytes) {
    int ret;
    ret = read(fd, buf, nbytes);
    if (ret < 0) {
        fprintf(stderr, "ERROR: 'read()' - %s\n", strerror(errno));
        exit(1);
    }
    return ret;
}

int mykill(pid_t pid, int sig) {
    int ret;
    ret = kill(pid, sig);
    if (ret < 0) {
        fprintf(stderr, "ERROR: 'kill()' - %s\n", strerror(errno));
        exit(1);
    }
    return ret;
}

pid_t mywaitpid(pid_t pid, int *stat_loc, int options) {
    int ret;
    ret = waitpid(pid, stat_loc, options);
    if (ret < 0) {
        fprintf(stderr, "ERROR: 'waitpid()' - %s\n", strerror(errno));
        exit(1);
    }
    return ret;
}

int myshutdown(int socket, int how) {
    int ret;
    ret = shutdown(socket, how);
    if (ret < 0) {
        fprintf(stderr, "ERROR: 'shutdown()' - %s\n", strerror(errno));
        exit(1);
    }
    return ret;
}
