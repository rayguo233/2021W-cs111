#include <getopt.h> // for getopt_long
#include <stdlib.h> // for exit, atoi
#include <stdio.h>
#include <time.h> // for clock_gettime
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h> // for strerror
#include <sched.h> // for sched_yield
#include <signal.h> // for signal
#include "SortedList.h"

#define STR_SIZE 25
SortedList_t head;
int nthreads = 1;
unsigned long niteratns = 1;
int opt_yield = 0;
char headname[30];
pthread_mutex_t mutex;
long lock;
enum sync_option {U, M, S}; // U for unsynchronized
int opt_sync = U; 
struct option opts[] =
	{
		{"threads", 1, NULL, 't'},
		{"iterations", 1, NULL, 'i'},
		{"sync", 1, NULL, 's'},
		{"yield", 1, NULL, 'y'},
		{0, 0, 0, 0}			
	};

void lock_general();
void unlock_general();
void init_list_and_ele(SortedListElement_t elements[]);
void rand_str(char *str);
void get_args(int argc, char **argv);
void * thread_worker(void *arg);
void handleerror(char* funcname);
void process_sync(char* option);
void process_yield(char* option);
void set_headname(char *optarg_yield, char *optarg_sync);
int myclock_gettime(clockid_t clock_id, struct timespec *tp);
int mypthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg);
int mypthread_join(pthread_t thread, void **retval);
static inline unsigned long get_nanosec_from_timespec(struct timespec * spec) {
	unsigned long ret= spec->tv_sec; //seconds
	ret = ret * 1000000000 + spec->tv_nsec; //nanoseconds return ret;
	return ret;
}

void sigsegv_handler(int sig) {
	if (sig == SIGSEGV) {
		fprintf(stderr, "Segfault due to corrupted linked list\n");
	}
	exit(1);
}

int main(int argc, char **argv) {
    signal(SIGSEGV, sigsegv_handler);
	struct timespec begin, end;
	unsigned long diff = 0;
    get_args(argc, argv);
    SortedListElement_t elements[nthreads*niteratns];
    init_list_and_ele(elements);
	pthread_t threads[nthreads];
	myclock_gettime(CLOCK_MONOTONIC, &begin);
	for (int i = 0; i < nthreads; i++) 
		mypthread_create(&threads[i], NULL, thread_worker, &elements[i*niteratns]);
	for (int i = 0; i < nthreads; i++) 
        mypthread_join(threads[i], NULL);
	myclock_gettime(CLOCK_MONOTONIC, &end);
	diff = get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&begin);
	if (SortedList_length(&head) != 0) {
		fprintf(stderr, "Error: length is not 0\n");
        return 2;
    }
    unsigned long n_oper;
	n_oper = nthreads * niteratns * 3;
	printf("%s,%d,%ld,1,%ld,%ld,%ld\n", headname,
			nthreads, niteratns, n_oper, diff, diff/n_oper);
	return 0;
}

void init_list_and_ele(SortedListElement_t elements[]) {
    head.key = NULL;
    head.next = &head;
    head.prev = &head;
	for (int i = 0; i < nthreads; i++) {
		for (unsigned long j = 0; j < niteratns; j++) {
			char str[STR_SIZE];
			rand_str(str);
			(elements[i*niteratns+j]).key = str;
			(elements[i*niteratns+j]).prev = NULL;
			(elements[i*niteratns+j]).next = NULL;
		}
	}
}

void rand_str(char *str) {
    const char alpha[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < STR_SIZE-1; i++) {
        str[i] = alpha[rand() % (sizeof(alpha) - 1)];
    }
    str[STR_SIZE-1] = 0;
}

void get_args(int argc, char **argv) {
    int i;
    char *optarg_sync = "none";
    char *optarg_yield = "none";
    while((i = getopt_long(argc, argv, "", opts, NULL)) != -1) {
		switch(i) {
			case 't':
				nthreads = atoi(optarg);
				break;
			case 'i': 
                niteratns = atoi(optarg);
				break;
			case 's': 
                optarg_sync = optarg;
                process_sync(optarg);
				break;
			case 'y':
                optarg_yield = optarg;
                process_yield(optarg);
				break;
			default:
				fprintf(stderr, "usage: ./lab2_add [--iterations=numite] [--threads=numthread] [--sync={m,s}] [--yield=[idl]]\n");
				exit(1);
				break;
		}
	}
	set_headname(optarg_yield, optarg_sync);
}

void process_sync(char* option) {
	if (strcmp(option, "m") == 0) {
		opt_sync = M;
		pthread_mutex_init(&mutex, NULL);
	}
	else if (strcmp(option, "s") == 0) {
		opt_sync = S;
		lock = 0;
	}
	else {
		fprintf(stderr, "incorrect option for '--sync': only {s,m} are allowed\n");	
		exit(1);
	}
}

void process_yield(char* option) {
	if (strcmp(option, "i") == 0) {
		opt_yield = INSERT_YIELD;
	}
	else if (strcmp(option, "d") == 0) {
        opt_yield = DELETE_YIELD;
	}
    else if (strcmp(option, "l") == 0) {
		opt_yield = LOOKUP_YIELD;
	}
    else if (strcmp(option, "id") == 0) {
		opt_yield = DELETE_YIELD + INSERT_YIELD;
	}
    else if (strcmp(option, "il") == 0) {
		opt_yield = LOOKUP_YIELD + INSERT_YIELD;
	}
    else if (strcmp(option, "dl") == 0) {
		opt_yield = LOOKUP_YIELD + DELETE_YIELD;
	}
    else if (strcmp(option, "idl") == 0) {
		opt_yield = LOOKUP_YIELD + DELETE_YIELD + INSERT_YIELD;
	}
	else {
		fprintf(stderr, "incorrect option for '--sync': only {s,m} are allowed\n");	
		exit(1);
	}
}

void * thread_worker(void *arg) {
	unsigned long i = 0;
	int ret;
    SortedListElement_t *elements = (SortedListElement_t*) arg; 
	for (i = 0; i < niteratns; i++) {
		lock_general();
        SortedList_insert(&head, elements+i);
		unlock_general();
	}
	lock_general();
    ret = SortedList_length(&head);
	if (ret == -1) {
		fprintf(stderr, "Error: length() - Linked list is corrupted\n");
		exit(2);
	}
	unlock_general();
	for (i = 0; i < niteratns; i++) {
		lock_general();
		ret = SortedList_delete(SortedList_lookup(&head, (elements+i)->key));
		if (ret == 1) {
			fprintf(stderr, "Error: delete() - corrtuped prev/next pointers\n");
			exit(2);
		}
		unlock_general();
    }
	return NULL;
}

void lock_general() {
	switch (opt_sync) {
		case M:
			pthread_mutex_lock(&mutex);
			break;
		case S:
			while (__sync_lock_test_and_set (&lock, 1)) {};
			break;
		default:
			break;
	}
}

void unlock_general() {
	switch (opt_sync) {
		case M:
			pthread_mutex_unlock(&mutex);
			break;
		case S:
			__sync_lock_release(&lock);
			break;
		default:
			break;
	}
}

void handleerror(char* funcname) {
	fprintf(stderr, "ERROR: '%s()' - %s\n", funcname, strerror(errno));
	exit(1);
}

void set_headname(char *optarg_yield, char *optarg_sync) {
	strcpy(headname, "list-");
    strcat(headname, optarg_yield);
    strcat(headname, "-");
    strcat(headname, optarg_sync);
}

int myclock_gettime(clockid_t clock_id, struct timespec *tp) {
	if (clock_gettime(clock_id, tp) != 0) handleerror("clock_gettime");
	return 0;
}

int mypthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg){
	if (pthread_create(thread, attr, start_routine, arg) != 0)
        handleerror("pthread_create");
	return 0;
}

int mypthread_join(pthread_t thread, void **retval) {
	if (pthread_join(thread, retval) != 0)
        handleerror("pthread_join");
	return 0;	
}