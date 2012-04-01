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
 * secmem.h
 * Definitions of secMemory functions
 * 
 * \file secmem.h
 * \author Patrick Plaschzug, Christian Partl, Georg Neubauer, Dieter Ladenhauf
*/

#ifndef __SEC_MEM_H__
#define __SEC_MEM_H__

#include <stdlib.h>
#include <stdio.h>
#include "typedef.h"

/**
* describes list entry structure for a double-linked list, that holds ptr to memory block
*/
typedef struct list_entry {
	struct list_entry *lep_before; /**< pointer to last entry */
	void *vp_ptr; /**< pointer to memory block */
	ssize_t i_len; /**< length of allocated memory */
	struct list_entry *lep_next; /**< pointer to next entry */
} list_entry;

/** 
* secMalloc routine, wrapper for malloc that registers allocated memory
* \param size size of memory to allocate
* \return ptr to new memory block
*/
void *secMalloc(size_t size);

/**
* secCalloc routine, wrapper for calloc that registers allocated memory
* \param nmemb number of blocks to allocate
* \param size size of memory to allocate
* \return ptr to new memory block
*/
void *secCalloc(size_t nmemb, size_t size);

/**
* secRealloc routine, wrapper for realloc that registers allocated memory
* \param ptr ptr to previous allocated memory block
* \param size size of memory to allocate
* \return ptr to new memory block
*/
void *secRealloc(void *ptr, size_t size);

/**
* secFindElement routine, find element in ptr lists
* \param ptr search for this pointer
* \return ptr to list entry or null if argument was invalid
*/
void *secFindElement(void *ptr);

/**
* secAddNewEntry routine, at a new list entry at end of ptr list
*/
void secAddNewEntry();

/**
* secCleanup routine, cleanup referenced pointers
*/
void secCleanup();

/**
* secFree routine, free allocated memory
* \param ptr to memory block that should be freed
*/
void secFree(void *ptr);

/**
* secProof routine, proof memory pointers
*/
void secProof(void *ptr);

/**
* secRegister routine, register a pointer not get by sec* functions
*/
void secRegister(void *ptr);

/**
* secAbort routine, used to abort with controlled cleanup
*/
void secAbort();

/**
* secExit routine, used to exit with controlled cleanup and -1
*/
void secExit(int i_status);

#endif

