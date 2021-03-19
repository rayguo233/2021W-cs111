#include <getopt.h> // for getopt_long
#include <stdlib.h> // for exit, atoi
#include <stdio.h>
#include <time.h> // for clock_gettime
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h> // for strerror
#include <sched.h> // for sched_yield

int nthreads = 1;
unsigned long niteratns = 1;
int opt_yield = 0;
long long sum = 0;
char* headname;
pthread_mutex_t mutex;
long lock;
enum sync_option {U, M, S, C}; // U for unsynchronized
int opt_sync = U; 
struct option opts[] =
	{
		{"threads", 1, NULL, 't'},
		{"iterations", 1, NULL, 'i'},
		{"sync", 1, NULL, 's'},
		{"yield", 0, NULL, 'y'},
		{0, 0, 0, 0}			
	};

void add_none(long long *pointer, long long value);
void add_m(long long *pointer, long long value);
void add_s(long long *pointer, long long value);
void add_c(long long *pointer, long long value);
void get_args(int argc, char **argv);
void * thread_worker(void *arg);
void handleerror(char* funcname);
void process_sync(char* option);
void set_headname();
void (*add) (long long *, long long) = add_none;
int myclock_gettime(clockid_t clock_id, struct timespec *tp);
int mypthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg);
int mypthread_join(pthread_t thread, void **retval);
static inline unsigned long get_nanosec_from_timespec(struct timespec * spec) {
	unsigned long ret= spec->tv_sec; //seconds
	ret = ret * 1000000000 + spec->tv_nsec; //nanoseconds return ret;
	return ret;
}

int main(int argc, char **argv) {
	struct timespec begin, end;
	unsigned long diff = 0;
    get_args(argc, argv);
	pthread_t threads[nthreads];
	myclock_gettime(CLOCK_MONOTONIC, &begin);
	for (int i = 0; i < nthreads; i++) 
		mypthread_create(&threads[i], NULL, thread_worker, &niteratns);
	for (int i = 0; i < nthreads; i++) mypthread_join(threads[i], NULL);
	myclock_gettime(CLOCK_MONOTONIC, &end);
	diff = get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&begin);
	unsigned long noper;
	noper = nthreads * niteratns * 2;
	printf("%s,%d,%ld,%ld,%ld,%ld,%lld\n", headname,
			nthreads, niteratns, noper, diff, diff/noper, sum);
	return 0;
}

void add_none(long long *pointer, long long value) {
    long long sum = *pointer + value;
	if (opt_yield) sched_yield();
    *pointer = sum;
}

void add_m(long long *pointer, long long value) {
	pthread_mutex_lock(&mutex);
	long long sum = *pointer + value;
	if (opt_yield) sched_yield();
    *pointer = sum;
	pthread_mutex_unlock(&mutex);
}

void add_s(long long *pointer, long long value) {
	while (__sync_lock_test_and_set (&lock, 1)) {};
	long long sum = *pointer + value;
	if (opt_yield) sched_yield();
    *pointer = sum;
	__sync_lock_release(&lock);
}

void add_c(long long *pointer, long long value) {
	long long prev, sum;
	do {
		prev = *pointer;
		sum = prev + value;
		if (opt_yield) sched_yield();
	} while(__sync_val_compare_and_swap(pointer, prev, sum) != prev);
}

void get_args(int argc, char **argv) {
    int i;
    while((i = getopt_long(argc, argv, "", opts, NULL)) != -1) {
		switch(i) {
			case 't':
				nthreads = atoi(optarg);
				break;
			case 'i': 
                niteratns = atoi(optarg);
				break;
			case 's': 
                process_sync(optarg);
				break;
			case 'y':
				opt_yield = 1;
				headname= "add-yield-none";
				break;
			default:
				fprintf(stderr, "usage: ./lab2_add [--iterations=numite] [--threads=numthread] [--sync=mode] [--yield]\n");
				exit(1);
				break;
		}
	}
	set_headname();
}

void process_sync(char* option) {
	if (strcmp(option, "m") == 0) {
		opt_sync = M;
		pthread_mutex_init(&mutex, NULL);
		add = add_m;
	}
	else if (strcmp(option, "s") == 0) {
		opt_sync = S;
		lock = 0;
		add = add_s;
	}
	else if (strcmp(option, "c") == 0) {
		opt_sync = C;
		add = add_c;
	}
	else {
		fprintf(stderr, "incorrect option for '--sync': only \"m/s/c\" are allowed\n");	
		exit(1);
	}
}

void * thread_worker(void *arg) {
	unsigned long iter = *((unsigned long*) arg), i = 0; //iter = 2047; 
	for (i = 0; i < iter; i++) add(&sum, 1);
	for (i = 0; i < iter; i++) add(&sum, -1);
	return NULL;
}

void handleerror(char* funcname) {
	fprintf(stderr, "ERROR: '%s()' - %s\n", funcname, strerror(errno));
	exit(1);
}

void set_headname() {
	if (opt_yield) {
		switch (opt_sync) {
			case U:
				headname = "add-yield-none"; break;
			case M:
				headname = "add-yield-m"; break;
			case S:
				headname = "add-yield-s"; break;
			case C:
				headname = "add-yield-c"; break;
			default: break;
		}
	}
	else {
		switch (opt_sync) {
			case U:
				headname = "add-none"; break;
			case M:
				headname = "add-m"; break;
			case S:
				headname = "add-s"; break;
			case C:
				headname = "add-c"; break;
			default: break;
		}
	}
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