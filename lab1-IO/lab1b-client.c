#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <termios.h>
#include <stdlib.h> // for exit(), atoi()
#include <getopt.h> // for getopt_long()
#include <poll.h> // for poll()
#include <ulimit.h> // for ulimit
#include <stdarg.h> // for va_list ...
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h> // for gethostbyname
#include <zlib.h>

struct option opts[] =
	{
		{"port", 1, NULL, 'p'},
		{"log", 1, NULL, 'l'},
		{"compress", 0, NULL, 'c'},
		{0, 0, 0, 0}			
	};
struct Args {
    int port;
    char *log;
    bool comp;
};
enum datatype {Send = 0, Receive = 1}; 

#define BUFSIZE 1024
#define LOGSIZE 1024
struct Args get_args(int argc, char **argv);
struct termios orig;
int socket_fd;
int logfd;
bool sending = false;
char logdata[LOGSIZE] = "";
bool comp = false;

void terminal_setup();
int client_connect(char* hostname, unsigned int port);
void read_socket();
void read_batch_stdin();
void read_batch_socket();
void zip(char *in, char *out, size_t in_size, size_t *out_size);
void unzip(char *in, char *out, size_t in_size, size_t *out_size);
void datalog(enum datatype type, char* buf, size_t size);
void myexit(int status);
int mywrite(int fd, void* buf, size_t size);
ssize_t myread(int fd, void *buf, size_t nbytes);
int myopen(const char *pathname, int flags, mode_t mode);
int check_dprintf(int ret, int fd);

int main(int argc, char **argv) {	
    struct Args args;
    args = get_args(argc, (char **)argv);
    // printf("%d, %s, %d\n", args.port, args.log, args.comp);
    socket_fd = client_connect("localhost", args.port);
    terminal_setup();

    struct pollfd pollfds[2];
    pollfds[0].fd = socket_fd;
    pollfds[0].events = POLLIN + POLLHUP + POLLERR;
    pollfds[1].fd = 0;
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
            read_batch_stdin();
        }
        if (pollfds[0].revents & (POLLHUP | POLLERR)) {
            myexit(0);
        }
        if (pollfds[1].revents & (POLLHUP | POLLERR)) {
            myexit(0);
        }
    }

    tcsetattr(0, TCSANOW, &orig);
}

void read_socket() {
    ulimit(UL_SETFSIZE, 10000);
    int ret, j;
    char buffer[BUFSIZE], c;

    ret = myread(socket_fd, buffer, sizeof(buffer));
    if (ret == 0) {
        char out = 0x4;
        mywrite(1, &out, sizeof(out));
        myexit(0);
    }
    for (j = 0; j < ret; j++) {
        c = buffer[j];
        if (c == 0x4) { // ^D
            char out = 0x4;
            mywrite(1, &out, sizeof(out));
            myexit(0);
        }
        else if (c == '\n') {
            char out[] = "\r\n";
            mywrite(1, &out, 2);
        }
        else {
            mywrite(1, &c, sizeof(c));
        }
    }
    if (ret > 0)
        datalog(Receive, (char*)&buffer, ret);
}

void read_batch_stdin() {
    ulimit(UL_SETFSIZE, 10000);
    int ret, j;
    char in[BUFSIZE], c, batch[BUFSIZE];
    int i = 0;
    bool end = false;
    while (!end) {
        if ((ret = myread(0, in, sizeof(in))) > 0){
            for (j = 0; j < ret; j++) {
                c = in[j];
                if (c == 0x3) {
                    char out[] = "^C";
                    mywrite(1, &out, sizeof(out));
                    end = true;
                }
                else if (c == 0x4) {
                    char out[] = "^D";
                    mywrite(1, &out, sizeof(out));
                    end = true;
                }
                else if (c == '\r' || c == '\n') {
                    char out[] = "\r\n";
                    mywrite(1, &out, 2);
                    c = '\n';
                    end = true;
                }
                else {
                    mywrite(1, &c, sizeof(c));
                }
                batch[i++] = c;
            }
            // strcat(batch, in);
        }
    }
    batch[i] = '\0';
    size_t size;
    if (comp) {
        zip(batch, in, strlen(batch), &size);
        datalog(Send, in, size);
        mywrite(socket_fd, in, size);
    }
    else {
        size = (size_t)i + 1;
        datalog(Send, batch, size);
        mywrite(socket_fd, batch, size);
    }
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
        myexit(1);
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

void read_batch_socket() {
    ulimit(UL_SETFSIZE, 10000);
    int ret;
    char in[BUFSIZE], out[BUFSIZE], c;
    ret = myread(socket_fd, in, sizeof(in));
    if (ret > 0)
        datalog(Receive, in, ret);
    if (ret == 0) {
        char eof = 0x4;
        mywrite(1, &eof, sizeof(eof));
        myexit(0);
    }
    size_t size, j;
    unzip(in, out, ret, &size);
    for (j = 0; j < size; j++) {
        c = out[j];
        if (c == '\n') {
            char nl[] = "\r\n";
            mywrite(1, &nl, sizeof(nl));
        }
        else {
            mywrite(1, &c, sizeof(c));
        }
    }
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
        myexit(1);
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

void datalog(enum datatype type, char* buf, size_t size) {
    if (logfd == -1) {
        return;
    }
    if (type == Send) {
        check_dprintf(dprintf(logfd, "SENT %ld bytes: ", size), logfd);
        check_dprintf(dprintf(logfd, "%s\n", buf), logfd);
    }
    else { // type == Receive
        check_dprintf(dprintf(logfd, "RECEIVED %ld bytes: ", size), logfd);
        check_dprintf(dprintf(logfd, "%s\n", buf), logfd);
    }
}

int client_connect(char* hostname, unsigned int port) {
    /* e.g. host_name:”google.com”, port:80, return the socket for subsequent communication */
    int sockfd;
    struct sockaddr_in serv_addr; /* server addr and port info */
    struct hostent* server;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { 
        fprintf(stderr, "ERROR: 'socket()' - %s\n", strerror(errno));
        exit(1);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons (port);
    server = gethostbyname(hostname); /* convert host_name to IP addr */

    /* copy ip address from server to serv_addr */
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    memset(serv_addr.sin_zero, '\0', sizeof serv_addr.sin_zero); /* padding zeros*/
    if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1) {
        /* initiate connection to the server*/
        fprintf(stderr, "ERROR: 'connect()' - %s\n", strerror(errno));
        exit(1);
    }
    return sockfd;
}

struct Args get_args(int argc, char **argv) {
    logfd = -1;
    int i;
    struct Args args;
    char c;
    c = '\0';
    args.port = -1;
    args.log = &c;
    args.comp = false;
    while((i = getopt_long(argc, argv, "", opts, NULL)) != -1) {
		switch(i) {
			case 'p':
				args.port = atoi(optarg);
				break;
			case 'l': 
                args.log = optarg;
                logfd = myopen(optarg, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
				break;
			case 'c':
				args.comp = true;
                comp = true;
				break;
			default:
				fprintf(stderr, "usage: ./lab1b-client --port=num [--log=file] [--compress]\n");
				exit(1);
				break;
		}
	}
    if (args.port == -1) {
        fprintf(stderr, "ERROR: missing an argument for port number\n\
                        usage: ./lab1b-client --port=num [--log=file] [--compress]\n");
        exit(1);
    }
    return args;
}

void terminal_setup() {
    tcgetattr(0, &orig);
    struct termios tmp;
    tmp = orig;
    tmp.c_iflag = ISTRIP, tmp.c_oflag = 0, tmp.c_lflag = 0;
    tcsetattr(0, TCSANOW, &tmp);
}

void myexit(int status) {
    tcsetattr(0, TCSANOW, &orig);
    exit(status);
}

int mywrite(int fd, void* buf, size_t size) {
    int ret;
    ret = write(fd, buf, size);
    if (ret < 0) {
        fprintf(stderr, "ERROR: 'write()' - %s\n", strerror(errno));
        myexit(1);
    }
    return ret;
}

ssize_t myread(int fd, void *buf, size_t nbytes) {
    int ret;
    ret = read(fd, buf, nbytes);
    if (ret < 0) {
        fprintf(stderr, "ERROR: 'read()' - %s\n", strerror(errno));
        myexit(1);
    }
    return ret;
}

int myopen(const char *pathname, int flags, mode_t mode) {
    int ret;
    ret = open(pathname, flags, mode);
    if (ret < 0) {
        fprintf(stderr, "ERROR: 'open(%s)' - %s\n", pathname, strerror(errno));
        myexit(1);
    }
    return ret;
}

int check_dprintf(int ret, int fd) {
    if (ret < 0) {
        fprintf(stderr, "ERROR: 'dprintf' to file descriptor %d failed - %s\n",
                        fd, strerror(errno));
        myexit(1);
    }
    return ret;
}

