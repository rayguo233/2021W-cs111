#include <stdio.h>
#include <stdbool.h>
#include <errno.h> // for errno
#include <getopt.h> // for getopt_long()
#include <unistd.h> // for dup, read, write, close
#include <signal.h> // for signal()
#include <string.h> // for strerror()
#include <stdlib.h> // for exit()
#include <sys/stat.h> // for open()
#include <fcntl.h> // for O_CREAT ...

#define BUFSIZE 1024

char buf[BUFSIZE];

struct option args[] =
	{
		{"input", 1, NULL, 'i'},
		{"output", 1, NULL, 'o'},
		{"segfault", 0, NULL, 's'},
		{"catch", 0, NULL, 'c'},
		{0, 0, 0, 0}			
	};

void sigsegv_handler(int sig) {
	if (sig == SIGSEGV) {
		fprintf(stderr, "Segfault successfully caught!\n");
	}
	exit(4);
}

void take_segfault() {
	char* ptr = NULL;
	(*ptr) = 0;
}

int main(int argc, char **argv) {	
	int i, ifd, ofd;
	bool seg_fault;
	seg_fault = false;
	while((i = getopt_long(argc, argv, "", args, NULL)) != -1) {
		switch(i) {
			case 'i':
				ifd = open(optarg, O_RDONLY);
				if (ifd == -1) {
					fprintf(stderr, "(--input) '%s': %s\n", optarg, strerror(errno));
					exit(2);
				}
				if (ifd >= 0) {
					close(0);
					dup(ifd);
					close(ifd);
				}
				break;
			case 'o': 
				ofd = open(optarg, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
				if (ofd == -1 ) {
					fprintf(stderr, "(--output) '%s': %s\n", optarg, strerror(errno));
					exit(3);
				}
				if (ofd >= 0) {
					close(1);
					dup(ofd);
					close(ofd);
				}
				break;
			case 's':
				seg_fault = true;
				break;
			case 'c':
				signal(SIGSEGV, sigsegv_handler);
				break;
			default:
				fprintf(stderr, "usage: ./lab0 [--input=file] [--output=file] [--segfault] [--catch]\n");
				exit(1);
				break;
		}
	} 
	// segfault
	if (seg_fault) {
		take_segfault();
	}

	// write
	int ret;
	while ((ret = read(0, buf, BUFSIZE)) > 0) {
		write(1, buf, ret);
	}

	exit(0);
}
