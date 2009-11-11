/** tiniweb
 * \file secmem.h
 * \author Georg Neubauer
 */

#ifndef __SEC_MEM_H__
#define __SEC_MEM_H__

#include <stdlib.h>

/** sec_malloc routine, wrapper for malloc that registers allocated memory
 * \param size size of memory to allocate
 * \return ptr to new memory block
 */
void *sec_malloc(size_t size);

/** sec_calloc routine, wrapper for calloc that registers allocated memory
 * \param nmemb number of blocks to allocate
 * \param size size of memory to allocate
 * \return ptr to new memory block
 */
void *sec_calloc(size_t nmemb, size_t size);

/** sec_realloc routine, wrapper for realloc that registers allocated memory
 * \param ptr ptr to previous allocated memory block
 * \param size size of memory to allocate
 * \return ptr to new memory block
 */
void *sec_realloc(void *ptr, size_t size);

/** sec_find_element routine, find element in ptr lists
 * \param ptr search for this pointer
 * \return ptr to list entry or null if argument was invalid
 */
void *sec_find_element(void *ptr);

/** sec_addnewentry routine, at a new list entry at end of ptr list
 *
 */
void sec_addnewentry();

/** sec_cleanup routine, cleanup referenced pointers
 *
 */
void sec_cleanup();

/** sec_free routine, free allocated memory
 * \param ptr to memory block that should be freed
 */
void sec_free(void *ptr);

/** sec_rewind routine, rewind referenced pointer list
 *
 */
void sec_rewind();

//TODO: remove this function, if not needed anymore
void sec_test();

#endif
