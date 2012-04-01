/** 
 * Copyright 2009-2012
 * 
 * Dieter Ladenhauf
 * Georg Neubauer
 * Christian Partl
 * Patrick Plaschzug
 * 
 * This file is part of Wunderwuzzi.
 * 
 * Wunderwuzzi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wunderwuzzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wunderwuzzi. If not, see <http://www.gnu.org/licenses/>.
 * 
 * -------------------------------------------------------------------
 * 
 * secmem.c
 * Implementation of secMemory functions
 * 
 * @file secmem.c
 * @author Patrick Plaschzug, Christian Partl, Georg Neubauer, Dieter Ladenhauf
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
	lep_memory_handles_first = NULL;
	lep_memory_handles_last = NULL;
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
	void *ptr = 0;
	if(!((nmemb < MAX_BUFFER_ALLOCATION_SIZE) &&
		(size < MAX_BUFFER_ALLOCATION_SIZE) &&
		((nmemb * size) < MAX_BUFFER_ALLOCATION_SIZE)))
	{
		debug(SEC_MEM, "Requested size is too big\n");
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
		return NULL;
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
	le = lep_memory_handles_first;
	while (le != 0) {
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

void secRegister(void *ptr)
{
	if(secFindElement(ptr) != 0)
		return;
	secAddNewEntry();
	lep_memory_handles_last->vp_ptr=ptr;
}

void secAbort()
{
	sendHTTPErrorMessage(STATUS_INTERNAL_SERVER_ERROR);
	
	debug(SEC_MEM, "-----INTERNAL FAILURE, SERVER IS GOING TO ABORT-----\n");
	debug(SEC_MEM, "------ERROR-MESSAGE: 500 Internal Server Error------\n");	
	secCleanup();
	abort();
}

void secExit(int i_status)
{
	if(i_status > STATUS_CANCEL)
	{
		sendHTTPErrorMessage(i_status);
		debug(SEC_MEM, "-----FAILURE, SERVER IS GOING TO EXIT-----\n");	
		debug(SEC_MEM, "------ERROR-MESSAGE: %s------\n",getStatusCode(i_status));
	}
	else if(i_status == STATUS_CANCEL)
	{
		debug(SEC_MEM, "-----CANCEL, SERVER IS GOING TO EXIT-----\n");
	}
	else if(i_status == STATUS_OK)
	{
		debug(SEC_MEM, "-----OK, SERVER IS GOING TO EXIT-----\n");	
		debug(SEC_MEM, "------MESSAGE: %s------\n",getStatusCode(i_status));
	}
	
	secCleanup();
	exit(i_status);
}

