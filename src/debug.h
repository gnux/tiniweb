#ifndef __DEBUG_H__
#define __DEBUG_H__
#include <stdlib.h>
#include "typedef.h"

void debug(size_t type, const char *ptr, ...);
void debugVerbose(size_t type, const char *ptr, ...);

/**
 * Special debug method, that prints a hash value to stderr too.
 * 
 * @param i_type type of the debug message
 * @param cuca_hash the hash to print
 * @param i_hash_len the length of thee hash
 * @param cca_ptr the normal message
 */
void debugVerboseHash(int i_type, const unsigned char* cuca_hash, int i_hash_len, const char* cca_ptr, ...);

#endif

