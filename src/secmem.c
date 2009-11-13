/** tiniweb
 * \file secmem.c
 * \author Georg Neubauer
 */

#include <memory.h>
#include <stdio.h>
#include "secmem.h"

/**
 * describes list entry structure for a double-linked list, that holds ptr to memory blocks
 */
typedef struct list_entry {
    struct list_entry *lep_before;
    void *vp_ptr;
    struct list_entry *lep_next;
} list_entry;

static list_entry *lep_memory_handles_first = 0;
static list_entry *lep_memory_handles_last = 0;

void secCleanup() {
    list_entry *lep_iterate = 0;
    secRewind();
    // clean up all registered pointers
    while (lep_memory_handles_first) {
        lep_iterate = lep_memory_handles_first->lep_next;
        free(lep_memory_handles_first->vp_ptr);
        free(lep_memory_handles_first);
        lep_memory_handles_first = lep_iterate;
    }
    lep_memory_handles_first = 0;
    lep_memory_handles_last = 0;
}

void secRewind() {
  if(!lep_memory_handles_first || !lep_memory_handles_last)
    return;
    while (lep_memory_handles_first->lep_before)
        lep_memory_handles_first = lep_memory_handles_first->lep_before;
    while (lep_memory_handles_last->lep_next)
        lep_memory_handles_last = lep_memory_handles_last->lep_next;
}

void secAddNewEntry() {
    // on init, initialiase first occourences
    if (!lep_memory_handles_first) {
        lep_memory_handles_first = malloc(sizeof(list_entry));
        lep_memory_handles_last = lep_memory_handles_first;
        lep_memory_handles_first->lep_before = 0;
    } else {
        // be sure to rewind
        secRewind();
        lep_memory_handles_last->lep_next = malloc(sizeof(list_entry));
        lep_memory_handles_last->lep_next->lep_before = lep_memory_handles_last;
        lep_memory_handles_last = lep_memory_handles_last->lep_next;
    }
    lep_memory_handles_last->lep_next = 0;
}

void *secMalloc(size_t size) {
    void *ptr;
    ptr = malloc(size);
    secProof(ptr);
    secAddNewEntry();
    lep_memory_handles_last->vp_ptr = ptr;
    return lep_memory_handles_last->vp_ptr;
}

void *secCalloc(size_t nmemb, size_t size) {
    void *ptr;
    ptr = calloc(nmemb, size);
    secProof(ptr);
    secAddNewEntry();
    lep_memory_handles_last->vp_ptr = ptr;
    return lep_memory_handles_last->vp_ptr;
}

void *secRealloc(void *ptr, size_t size) {
    list_entry *le = 0;
    if (!ptr)
        return secMalloc(size);
    le = secFindElement(ptr);
    if (!le)
        return 0;
    if (!size) {
        secFree(ptr);
        return 0;
    }
    ptr = realloc(ptr, size);
    secProof(ptr);
    le->vp_ptr = ptr;
    return ptr;
}

void secFree(void *ptr) {
    list_entry *le = 0;
    le = secFindElement(ptr);
    if(!le)
        return;
    free(le->vp_ptr);
    if (le->lep_before)
        le->lep_before->lep_next = le->lep_next;
    if (le->lep_next)
        le->lep_next->lep_before = le->lep_before;
    if (le == lep_memory_handles_first)
        lep_memory_handles_first = le->lep_next;
    if (le == lep_memory_handles_last)
        lep_memory_handles_last = le->lep_before;
    free(le);
}

void *secFindElement(void *ptr) {
    list_entry *le = 0;
    secRewind();
    while (le) {
        if (le->vp_ptr == ptr)
            break;
        le = le->lep_next;
    }
    return le;
}

void secProof(void *ptr){
  if(!ptr){
    fprintf(stderr, "Got NULL-Pointer from memory call, abnormall behaviour detected, we will abort!");
    secCleanup();
    abort();
  }
}

void secRegister(void *ptr){
  if(secFindElement(ptr))
    return;
  secAddNewEntry();
  lep_memory_handles_last->vp_ptr=ptr;
}

//TODO: remove this function, if not needed anymore
void sec_test() {
    char *ptr;
    int i;

    for (i = 0; i < 500; i++) {
        secMalloc(i);
        secCalloc(i * 3, i);
    }
    ptr = secMalloc(30);

    for (i = 0; i < 100; ++i)
        ptr = secRealloc(ptr, i);

    ptr = secMalloc(300000);
    for (i = 0; i < 3000; ++i)
        ptr = secRealloc(ptr, i);
    ptr = secRealloc(0, 222);
    ptr = secRealloc(ptr, 0);

    secCleanup();
    ptr = secCalloc(5, 30);
    secFree(ptr);
    ptr = secCalloc(5, 30);
    secCleanup();
}