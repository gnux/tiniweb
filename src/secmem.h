#ifndef __SEC_MEM_H__
#define __SEC_MEM_H__

#include <stdlib.h>

void *sec_malloc(size_t size);
void *sec_calloc(size_t nmemb, size_t size);
void *sec_realloc(void *ptr, size_t size);
void *sec_find_element(void *ptr);
void sec_addnewentry();
void sec_cleanup();
void sec_test();

#endif
