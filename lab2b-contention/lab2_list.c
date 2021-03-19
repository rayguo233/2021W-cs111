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
unsigned long nthreads = 1;
unsigned long niteratns = 1;
int sublist_num = 1;
int opt_yield = 0;
char headname[30];
SortedList_t *listheads = NULL;
pthread_mutex_t *mutexes;
long *locks;
SortedListElement_t *elements;
enum sync_option {U, M, S}; // U for unsynchronized
int opt_sync = U; 
struct option opts[] =
	{
		{"threads", 1, NULL, 't'},
		{"iterations", 1, NULL, 'i'},
		{"sync", 1, NULL, 's'},
		{"yield", 1, NULL, 'y'},
		{"lists", 1, NULL, 'l'},
		{0, 0, 0, 0}			
	};

void itoa(int n, char s[]);
int get_list_num(const char *key);
unsigned long lock_general(int list_num);
void unlock_general(int ist_num);
void init_list_and_ele(SortedListElement_t elements[]);
void hash_str(char *str, int list_num);
void get_args(int argc, char **argv);
void * thread_worker(void *arg);
void handleerror(char* funcname);
void process_sync(char* option);
void process_yield(char* option);
void set_headname(char *optarg_yield, char *optarg_sync);
void cleanup();
int myclock_gettime(clockid_t clock_id, struct timespec *tp);
int mypthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg);
int mypthread_join(pthread_t thread, void **retval);
int mypthread_mutex_init(pthread_mutex_t *restrict mutex,
			const pthread_mutexattr_t *restrict attr);
void *mymalloc(size_t size);
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
	unsigned long total_lock_time = 0;
    get_args(argc, argv);
    elements = mymalloc(sizeof(SortedListElement_t)*nthreads*niteratns);
	
	SortedList_t heads[sublist_num];
	listheads = heads;
    for (int i = 0; i < sublist_num; i++) {
		listheads[i].key = NULL;
		listheads[i].next = &listheads[i];
		listheads[i].prev = &listheads[i];
	}

    init_list_and_ele(elements);
	pthread_t threads[nthreads];
	myclock_gettime(CLOCK_MONOTONIC, &begin);
	for (unsigned long i = 0; i < nthreads; i++) 
		mypthread_create(&threads[i], NULL, thread_worker, &elements[i*niteratns]);
	for (unsigned long i = 0; i < nthreads; i++) {
		void *retval;
        mypthread_join(threads[i], &retval);
		total_lock_time += (unsigned long) retval;
	}
	myclock_gettime(CLOCK_MONOTONIC, &end);
	diff = get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&begin);
	for (int i = 0; i < sublist_num; i++) {
		if (SortedList_length(&listheads[i]) != 0) {
			fprintf(stderr, "Error: length is not 0\n");
			cleanup();
			return 2;
		}
	}
    unsigned long n_oper;
	n_oper = nthreads * niteratns * 3;
	printf("%s,%ld,%ld,%d,%ld,%ld,%ld,%ld\n", headname, nthreads, niteratns,
			sublist_num, n_oper, diff, diff/n_oper, total_lock_time/n_oper);
	cleanup();
	return 0;
}

int get_list_num(const char *key) {
	return atoi(key) % sublist_num;
}

void init_list_and_ele(SortedListElement_t *elements) {
	int list_num = 0;
	for (unsigned long i = 0; i < nthreads; i++) {
		for (unsigned long j = 0; j < niteratns; j++) {
			char *str = mymalloc(sizeof(char)*STR_SIZE);
			hash_str(str, list_num);
			// printf("%d, %d\n", list_num, get_list_num(str));
			(elements[i*niteratns+j]).key = str;
			(elements[i*niteratns+j]).prev = NULL;
			(elements[i*niteratns+j]).next = NULL;
			list_num = (list_num + 1) % sublist_num;
		}
	}
}

void itoa(int n, char s[])
 {
     int i = 0;
     do {     
         s[i++] = n % 10 + '0'; 
     } while ((n /= 10) > 0);   
     s[i] = '\0';

	 // reverse string
	 int k, j;
     char c;
     for (k = 0, j = strlen(s)-1; k<j; k++, j--) {
         c = s[k];
         s[k] = s[j];
         s[j] = c;
     }
}  

void hash_str(char *str, int list_num) {
    int temp = rand() % (1 << 15);
	temp = temp - (temp % sublist_num) + list_num;
	itoa(temp, str);
}

void get_args(int argc, char **argv) {
    int i;
    char *optarg_sync = "none";
    char *optarg_yield = "none";
	bool is_sync = false;
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
				is_sync = true;
				break;
			case 'y':
                optarg_yield = optarg;
                process_yield(optarg);
				break;
			case 'l':
                sublist_num = atoi(optarg);
				break;
			default:
				fprintf(stderr, "usage: ./lab2_add [--iterations=numite] [--threads=numthread] [--sync={m,s}] [--yield=[idl]]\n");
				exit(1);
				break;
		}
	}
    if (is_sync) process_sync(optarg_sync);
	if ((unsigned long)sublist_num > niteratns) sublist_num = niteratns;
	set_headname(optarg_yield, optarg_sync);
}

void process_sync(char* option) {
	if (strcmp(option, "m") == 0) {
		opt_sync = M;
		mutexes = mymalloc(sizeof(pthread_mutex_t)*sublist_num);
		for (int i = 0; i < sublist_num; i++)
			mypthread_mutex_init(&mutexes[i], NULL);
	}
	else if (strcmp(option, "s") == 0) {
		opt_sync = S;
		locks = mymalloc(sizeof(long)*sublist_num);
		for (int i = 0; i < sublist_num; i++)
			locks[i] = 0;
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
	int list_num;
	unsigned long res = 0;
    SortedListElement_t *elements = (SortedListElement_t*) arg;
	for (i = 0; i < niteratns; i++) {
		list_num = get_list_num((elements+i)->key);
		// printf("list: %d\n", list_num);
		res += lock_general(list_num);
        SortedList_insert(&listheads[list_num], elements+i);
		unlock_general(list_num);
	}
	for (int j; j < sublist_num; j++) {
		res += lock_general(j);
		if ((ret = SortedList_length(&listheads[j])) == -1) {
			fprintf(stderr, "Error: length() - Linked list is corrupted\n");
			exit(2);
		};
		unlock_general(j);
		// printf("list: %d, length: %d\n", j, ret);
	}
	for (i = 0; i < niteratns; i++) {
		list_num = get_list_num((elements+i)->key);
		// printf("list_num: %d\n", list_num);
		res += lock_general(list_num);
		ret = SortedList_delete(SortedList_lookup(&listheads[list_num], (elements+i)->key));
		unlock_general(list_num);
		if (ret == 1) {
			fprintf(stderr, "Error: delete() - corrtuped prev/next pointers\n");
			exit(2);
		}
    }
	return (void *) res;
}

unsigned long lock_general(int list_num) {
	struct timespec begin, end;
	unsigned long diff = 0;
	switch (opt_sync) {
		case M:
			myclock_gettime(CLOCK_MONOTONIC, &begin);
			pthread_mutex_lock(&mutexes[list_num]);
			myclock_gettime(CLOCK_MONOTONIC, &end);
			diff = get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&begin);
			break;
		case S:
			myclock_gettime(CLOCK_MONOTONIC, &begin);
			while (__sync_lock_test_and_set (&locks[list_num], 1)) {};
			myclock_gettime(CLOCK_MONOTONIC, &end);
			diff = get_nanosec_from_timespec(&end) - get_nanosec_from_timespec(&begin);
			break;
		default:
			break;
	}
	return diff;
}

void unlock_general(int list_num) {
	switch (opt_sync) {
		case M:
			pthread_mutex_unlock(&mutexes[list_num]);
			break;
		case S:
			__sync_lock_release(&locks[list_num]);
			break;
		default:
			break;
	}
}

void handleerror(char* funcname) {
	fprintf(stderr, "ERROR: '%s()' - %s\n", funcname, strerror(errno));
	cleanup();
	exit(1);
}

void set_headname(char *optarg_yield, char *optarg_sync) {
	strcpy(headname, "list-");
    strcat(headname, optarg_yield);
    strcat(headname, "-");
    strcat(headname, optarg_sync);
}

void cleanup() {
	switch (opt_sync)
	{
	case M:
		free(mutexes);
		break;
	case S:
		free(locks);
		break;
	default:
		break;
	}
	for (unsigned long i = 0; i < nthreads*niteratns; i++) {
		free((char*)elements[i].key);
	}
	free(elements);
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

int mypthread_mutex_init(pthread_mutex_t *restrict mutex,
			const pthread_mutexattr_t *restrict attr) {
	int ret;
	ret = pthread_mutex_init(mutex, attr);
	if (ret != 0) {
		handleerror("pthread_mutex_init");
	}
	return ret;
}

void *mymalloc(size_t size) {
	void *ret;
	if ((ret = malloc(size)) == NULL)
        handleerror("malloc");
	return ret;
}
