#include <stdlib.h> // for exit
#include <stdio.h>
#include <string.h> // for strerror
#include <sched.h> // for sched_yield
#include "SortedList.h"

void connect(SortedListElement_t *prev, SortedListElement_t *next) {
    // if (prev->key != NULL && next->key != NULL && 
    //     (strcmp(prev->key, next->key) > 0 || prev == next)) {
	// 	fprintf(stderr, "Error: connect() - Linked list is corrupted\n");
    //     exit(2);
    // }
    prev->next = next;
    next->prev = prev;
}

void insert_before(SortedList_t *list, SortedListElement_t *element, int yield) {
    SortedListElement_t *prev = list->prev;
    if (yield) sched_yield();
    connect(prev, element);
    connect(element, list);
}

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
    SortedList_t *ptr = list->next;
    while (ptr != list && (strcmp(ptr->key, element->key) > 0)) {
        ptr = ptr->next;
    }
    insert_before(ptr, element, opt_yield & INSERT_YIELD);
}

int SortedList_delete( SortedListElement_t *element) {
    // if (element == NULL) {
	// 	fprintf(stderr, "Error: delete() - Linked list is corrupted\n");
    //     exit(2);
    // }
    SortedListElement_t* prev = element->prev;
    SortedListElement_t* next = element->next;
    if (next->prev != element || prev->next != element) {
		// fprintf(stderr, "Error: delete() - Linked list is corrupted\n");
        return 1;
    }
    if (opt_yield & DELETE_YIELD) sched_yield();
    connect(prev, next);
    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    SortedList_t *ptr = list->next;
    while (ptr != list) {
        if (strcmp(ptr->key, key) == 0) {
            return ptr;
        }
        ptr = ptr->next;
        if (opt_yield & LOOKUP_YIELD) sched_yield();
    }
    // fprintf(stderr, "Error: lookup() - Linked list is corrupted\n");
    return NULL;
}


int SortedList_length(SortedList_t *list) {
    if (list->next == NULL || list->prev == NULL) {
        return -1;
    }
    int ret = 0;
    SortedList_t *ptr = list->next; 
    while (list != ptr) {
        if (ptr->next == NULL || ptr->prev == NULL) {
            return -1;
        }
        if (ptr->next->prev != ptr || ptr->prev->next != ptr) {
            return -1;
        }
        ret++;
        ptr = ptr->next;
        if (opt_yield & LOOKUP_YIELD) sched_yield();
    }
    return ret;

}
