/** secmem.c
* Implementation of secMemory functions
* \file typedef.h
* \author Patrick Plaschzug, Christian Partl, Georg Neubauer, Dieter Ladenhauf
*/

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "secmem.h"
#include "debug.h"
#include "httpresponse.h"
#include "typedef.h"

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
		secProof(lep_memory_handles_first);
		lep_memory_handles_last = lep_memory_handles_first;
		lep_memory_handles_first->lep_before = 0;
	}
	else
	{
		// be sure to rewind
		secRewind();
		lep_memory_handles_last->lep_next = malloc(sizeof(list_entry));
		secProof(lep_memory_handles_last->lep_next);
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

void *secCalloc(size_t nmemb, size_t size)
{
	void *ptr = NULL;
	if(!((nmemb < MAX_BUFFER_ALLOCATION_SIZE) &&
		(size < MAX_BUFFER_ALLOCATION_SIZE) &&
		((nmemb * size) < MAX_BUFFER_ALLOCATION_SIZE)))
	{
		debug(SEC_MEM, "Requested size \n");
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


//TODO: this 2 functions!!!
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

