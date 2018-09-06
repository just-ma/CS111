#include <stdio.h>
#include <string.h>
#include <sched.h>

#include "SortedList.h"

void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
    if (list == NULL || element == NULL)
        return;
    SortedListElement_t *cur = list->next;
    while (cur != list){
        if (strcmp(cur->key, element->key) < 0)
            break;
        cur = cur->next;
    }
    
    if (opt_yield & INSERT_YIELD)
        sched_yield();
    
    cur->prev->next = element;
    element->prev = cur->prev;
    cur->prev = element;
    element->next = cur;
}

int SortedList_delete( SortedListElement_t *element){
    if (element == NULL || element->next->prev != element || element->prev->next != element)
        return 1;
    
    if (opt_yield & DELETE_YIELD)
        sched_yield();
    element->prev->next = element->next;
    element->next->prev = element->prev;
    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
    if (list == NULL || key == NULL)
        return NULL;
    SortedListElement_t *cur = list->next;
    while (cur != list){
        if (strcmp(cur->key, key) == 0)
            return cur;
        
        if (opt_yield & LOOKUP_YIELD)
            sched_yield();

        cur = cur->next;
    }
    return NULL;
}

int SortedList_length(SortedList_t *list){
    if (list == NULL)
        return -1;
    SortedListElement_t *cur = list->next;
    int count = 0;
    while (cur != list){
        count++;
        
        if (opt_yield & LOOKUP_YIELD)
            sched_yield();

        cur = cur->next;
    }
    return count;
}
