#include <termios.h>
#include <unistd.h>
#include <stdlib.h> // for exit()
#include <stdio.h> // for putchar()
#include <getopt.h> // for getopt_long()
#include <stdbool.h>
#include <poll.h> // for poll()
#include <sys/types.h> // for kill()
#include <signal.h> // for SIGINT (maybe)
#include <sys/wait.h> // for waitpid()
#include <errno.h> // for errno
#include <string.h> // for strerror()

#define BUFSIZE 1024
#define WR 1
#define RD 0

int pid;
int to_term, from_term; // file descriptors on the two pipes
struct termios orig;
struct option args[] =
	{
		{"shell", 0, NULL, 's'},
		{0, 0, 0, 0}			
	};

void terminal_setup(void);
void regular(void);
void bash(void);
void read_bash();
void read_stdin();
void last_read(int exit_stat);
void done(int status);
void sigpipe_handler(int sig);

// error handling system calls
void myexit(int status);
int mywrite(int fd, void* buf, size_t size);
ssize_t myread(int fd, void *buf, size_t nbytes);
pid_t mywaitpid(pid_t _pid, int *stat_loc, int options);

int main(int argc, char **argv) {	
    terminal_setup();

    // process arguments
    bool shell;
    shell = false;
    int i;
    while((i = getopt_long(argc, argv, "", args, NULL)) != -1) {
		switch(i) {
			case 's': // --shell=
				shell = true;
                break;
			default:
				fprintf(stderr, "usage: ./lab1a [--shell]\n");
				myexit(1);
				break;
		} 
    } 
    
    if (shell) {
        bash();
    }
    else {
        regular();
    }

    tcsetattr(0, TCSANOW, &orig);
    return 0;
}

void regular(void) {
    int ret;
    char buf[BUFSIZE];
    int j;
    char c;
    while (1) {
        ret = myread(0, buf, BUFSIZE);
        for (j = 0; j < ret; j++) {
            c = buf[j];
            if (c == 0x4) { // C-d
                char out[] = "^D";
                mywrite(1, &out, sizeof(out));
                myexit(0);
            }
            else if (c == 0x3) { // C-c
                char out[] = "^C";
                mywrite(1, &out, sizeof(out));
                myexit(0);
            }
            else if (c == '\r' || c == '\n') {
                char out[] = "\r\n";
                mywrite(1, &out, sizeof(out));
            }
            else {
                mywrite(1, &c, 1);
            }
        }
    }
}

void bash(void) {
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
        pollfds[0].fd = 0;
        pollfds[0].events = POLLIN + POLLHUP + POLLERR;
        pollfds[1].fd = to_term;
        pollfds[1].events = POLLIN + POLLHUP + POLLERR;

        while (1) {
            poll(pollfds, 2, -1); // -1 means no time out
            if (pollfds[0].revents & POLLIN) {
                // read from stdin
                read_stdin();
            }
            if (pollfds[1].revents & POLLIN) {
                // read from bash
                read_bash();
            }
            if (pollfds[0].revents & (POLLHUP | POLLERR)) {
                done(1);
            }
            if (pollfds[1].revents & (POLLHUP | POLLERR)) {
                done(0);
            }
        }
    }
    else {
        fprintf(stderr, "ERROR: 'fork()' - %s\n", strerror(errno));
        myexit(1);
    }
}

void read_stdin() {
    int ret, j;
    char buffer[BUFSIZE], c;

    ret = myread(0, buffer, sizeof(buffer));
    for (j = 0; j < ret; j++) {
        c = buffer[j];
        if (c == 0x3) { // C-c
            char out[] = "^C";
            mywrite(1, &out, sizeof(out));
            ret = kill(pid, SIGINT);
        }
        else if (c == 0x4) { // C-d
            char out[] = "^D";
            mywrite(1, &out, sizeof(out));
            done(0);
        }
        else if (c == '\r' || c == '\n') {
            char out[] = "\r\n";
            mywrite(1, &out, 2);
            c = '\n';
            mywrite(from_term, &c, 1);
        }
        else {
            mywrite(1, &c, 1);
            mywrite(from_term, &c, 1);
        }
    }
}

void read_bash() {
    int ret, j;
    char buffer[BUFSIZE], c;

    ret = myread(to_term, buffer, sizeof(buffer));
    for (j = 0; j < ret; j++) {
        c = buffer[j];
        if (c == 0x4) { // C-d
            char out[] = "^D";
            mywrite(1, &out, sizeof(out));
            done(0);
        }
        else if (c == '\n') {
            char out[] = "\r\n";
            mywrite(1, &out, 2);
        }
        else {
            mywrite(1, &c, 1);
        }
    }
}

void last_read(int exit_stat) {
    int ret, j;
    char buffer[BUFSIZE], c;

    while ((ret = read(to_term, buffer, sizeof(buffer))) > 0) {
        for (j = 0; j < ret; j++) {
            c = buffer[j];
            if (c == 0x4) { // C-d
                char out[] = "^D";
                mywrite(1, &out, sizeof(out));
                int wstatus;
                mywaitpid(pid, &wstatus, 0);
                fprintf(stderr, "SHELL EXIT, SIGNAL=%d STATUS=%d\n", 
                            WTERMSIG(wstatus), WEXITSTATUS(wstatus));
                myexit(exit_stat);
            }
            else if (c == '\n') {
                char out[] = "\r\n";
                mywrite(1, &out, 2);
            }
            else {
                mywrite(1, &c, 1);
            }
        }
    }
}

void terminal_setup() {
    tcgetattr(0, &orig);
    struct termios tmp;
    tmp = orig;
    tmp.c_iflag = ISTRIP, tmp.c_oflag = 0, tmp.c_lflag = 0;
    tcsetattr(0, TCSANOW, &tmp);
}

void done(int status) {
    close(from_term);
    last_read(status);
    int wstatus;
    mywaitpid(pid, &wstatus, 0);
    fprintf(stderr, "SHELL EXIT, SIGNAL=%d STATUS=%d\n", 
                    WTERMSIG(wstatus), WEXITSTATUS(wstatus));
    myexit(status);
}

void sigpipe_handler(int sig) {
	if (sig == SIGPIPE) {
        done(1);
	}
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

pid_t mywaitpid(pid_t pid, int *stat_loc, int options) {
    int ret;
    ret = waitpid(pid, stat_loc, options);
    if (ret < 0) {
        fprintf(stderr, "ERROR: 'waitpid()' - %s\n", strerror(errno));
        myexit(1);
    }
    return ret;
}
