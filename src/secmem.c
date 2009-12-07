/** tiniweb
* \file secmem.c
* \author Georg Neubauer
*/

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "secmem.h"
#include "debug.h"
#include "httpresponse.h"
#include "typedef.h"

int isValid(const char* ccp_input, const size_t i_offset);

/**
* describes list entry structure for a double-linked list, that holds ptr to memory blocks
*/
typedef struct list_entry {
	struct list_entry *lep_before;
	void *vp_ptr;
	size_t i_len;
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
		if(lep_memory_handles_first->i_len)
			memset(lep_memory_handles_first->vp_ptr, 0x00,lep_memory_handles_first->i_len);
		free(lep_memory_handles_first->vp_ptr);
		memset(lep_memory_handles_first, 0x00, sizeof(list_entry));
		free(lep_memory_handles_first);
		lep_memory_handles_first = lep_iterate;
	}
	lep_memory_handles_first = 0;
	lep_memory_handles_last = 0;
}

void secRewind()
{
	if(!lep_memory_handles_first || !lep_memory_handles_last)
		return;
	while (lep_memory_handles_first->lep_before)
		lep_memory_handles_first = lep_memory_handles_first->lep_before;
	while (lep_memory_handles_last->lep_next)
		lep_memory_handles_last = lep_memory_handles_last->lep_next;
}

void secAddNewEntry()
{
	// on init, initialiase first occourences
	if (!lep_memory_handles_first)
	{
		lep_memory_handles_first = malloc(sizeof(list_entry));
		if(!lep_memory_handles_first)
			secAbort();
		lep_memory_handles_last = lep_memory_handles_first;
		lep_memory_handles_first->lep_before = 0;
	}
	else
	{
		// be sure to rewind
		secRewind();
		lep_memory_handles_last->lep_next = malloc(sizeof(list_entry));
		if(!lep_memory_handles_last)
			secAbort();
		lep_memory_handles_last->lep_next->lep_before = lep_memory_handles_last;
		lep_memory_handles_last = lep_memory_handles_last->lep_next;
	}
	lep_memory_handles_last->lep_next = 0;
	lep_memory_handles_last->i_len = 0;
}

void *secMalloc(size_t size)
{
	return secCalloc(1, size);
}
//void *ptr;
//ptr = malloc(size);
//secProof(ptr);
//secAddNewEntry();
//lep_memory_handles_last->vp_ptr = ptr;
//lep_memory_handles_last->i_len = size;
//return lep_memory_handles_last->vp_ptr;
//}

void *secCalloc(size_t nmemb, size_t size)
{
	void *ptr = NULL;
	if(!((nmemb < MAX_BUFFER_ALLOCATION_SIZE) &&
		(size < MAX_BUFFER_ALLOCATION_SIZE) &&
		((nmemb * size) < MAX_BUFFER_ALLOCATION_SIZE)))
	{
		debug(SEC_MEM, "Requestet size \n");
		secAbort();
	}
	ptr = calloc(nmemb, size);
	secProof(ptr);
	secAddNewEntry();
	lep_memory_handles_last->vp_ptr = ptr;
	lep_memory_handles_last->i_len = nmemb * size;
	return lep_memory_handles_last->vp_ptr;
}

void *secRealloc(void *ptr, size_t size)
{
	list_entry *le = 0;
	if(!(size < MAX_BUFFER_ALLOCATION_SIZE))
	{
		debug(SEC_MEM, "We defined a mximum Header Size, and we don't like to exceed it!\n");
		secAbort();
	}
	if (!ptr)
		return secMalloc(size);
	le = secFindElement(ptr);
	if (!le)
	{
		debug(SEC_MEM, "Pointer was not allocated via secMem!\n");
		secAbort();
	}
	if (!size) {
		secFree(ptr);
		return 0;
	}
	ptr = realloc(ptr, size);
	secProof(ptr);
	le->i_len = size;
	le->vp_ptr = ptr;
	return ptr;
}

void secFree(void *ptr) {
	list_entry *le = 0;
	le = secFindElement(ptr);
	if(!le)
		return;
	if(le->i_len)
		memset(le->vp_ptr, 0x00, le->i_len);
	free(le->vp_ptr);
	if (le->lep_before)
		le->lep_before->lep_next = le->lep_next;
	if (le->lep_next)
		le->lep_next->lep_before = le->lep_before;
	if (le == lep_memory_handles_first)
		lep_memory_handles_first = le->lep_next;
	if (le == lep_memory_handles_last)
		lep_memory_handles_last = le->lep_before;
	memset(le, 0x00, sizeof(list_entry));
	free(le);
}

void *secFindElement(void *ptr) {
	list_entry *le = 0;
	secRewind();
	le = lep_memory_handles_first;
	while (le) {
		if (le->vp_ptr == ptr)
			break;
		le = le->lep_next;
	}
	return le;
}

void secProof(void *ptr){
	if(!ptr){
		debug(SEC_MEM,"Got NULL-Pointer from memory call, abnormal behaviour detected, we will abort!");
		secAbort();
	}
}

void secRegister(void *ptr){
	if(secFindElement(ptr))
		return;
	secAddNewEntry();
	lep_memory_handles_last->vp_ptr=ptr;
}

void secAbort(){
	//TODO:provide error handling! error responses, maybe get used of a secExit function that sends responses!
	// use secAbort only for internal failures
	
	sendHTTPErrorMessage(STATUS_INTERNAL_SERVER_ERROR);
	
	debug(SEC_MEM, "-----INTERNAL FAILURE, SERVER IS GOING TO ABORT-----\n");
	debug(SEC_MEM, "------ERROR-MESSAGE: 500 Internal Server Error------\n");	
	secCleanup();
	//TODO: cleanup open files and pipes
	abort();
}

void secExit(int i_status){
	
	sendHTTPErrorMessage(i_status);
	
	debug(SEC_MEM, "-----FAILURE, SERVER IS GOING TO EXIT-----\n");	
	debug(SEC_MEM, "------ERROR-MESSAGE: %s------\n",getStatusCode(i_status));
	
	secCleanup();
	exit(-1);	
}

ssize_t secGetline(char** cpp_lineptr, FILE *stream){
	size_t i_num_reads = 0;
	ssize_t i_ret = 0;
	ssize_t i = 0;
	if(*cpp_lineptr)
		secFree(*cpp_lineptr);
	
	*cpp_lineptr = NULL;
	i_ret = getline(cpp_lineptr, &i_num_reads, stream);
	// proof our input, TODO: BAD request
	if(i_ret == -1)
		secAbort();
	for(; i < i_ret; ++i)
		if(isValid(*cpp_lineptr, i) == EXIT_FAILURE)
			secAbort();
		secProof(*cpp_lineptr);
	secRegister(*cpp_lineptr);
	
	return i_ret;
}

// void *secGetStringPart(const char* ccp_string, ssize_t start, ssize_t end){
   // 	if(end < start || end > strlen(ccp_string))
   // 		//TODO: something went wrong we would return NULL
   // 		return NULL;
   // 		//secAbort();
   // 	ssize_t len = end - start + 2;
   // 	ssize_t i;
   // 	char *cp_fragment = secCalloc(len, sizeof(char));
   // 	cp_fragment[len - 1] = '\0';
   // 	for(i=0; i < len - 1; ++i)
   // 	cp_fragment[i] = ccp_string[start + i];
   // 	
   // 	return cp_fragment;
   // }
   
   // //TODO: remove this function, if not needed anymore
   // void sec_test() {
   //     char *ptr;
   //     int i;
   // 
   //     for (i = 0; i < 500; i++) {
   //         secMalloc(i);
   //         secCalloc(i * 3, i);
   //     }
   //     ptr = secMalloc(30);
   // 
   //     for (i = 0; i < 100; ++i)
   //         ptr = secRealloc(ptr, i);
   // 
   //     ptr = secMalloc(300000);
   //     for (i = 0; i < 3000; ++i)
   //         ptr = secRealloc(ptr, i);
   //     ptr = secRealloc(0, 222);
   //     ptr = secRealloc(ptr, 0);
   // 
   //     secCleanup();
   //     ptr = secCalloc(5, 30);
   //     secFree(ptr);
   //     ptr = secCalloc(5, 30);
   //     secCleanup();
   // }
   
