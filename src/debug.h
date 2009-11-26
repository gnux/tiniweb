#ifndef __DEBUG_H__
#define __DEBUG_H__
#include <stdlib.h>
#include "typedef.h"

static const enum SCE_DEBUG_TYPES
{
    MAIN,
    SEC_MEM,
    AUTH,
    NORMALISE,
    PARSER,
    ENVVAR,
    CGICALL,
    PATH
    
} SCE_DEBUG_TYPE;

void debug(int type, const char *ptr, ...);
void debugVerbose(int type, const char *ptr, ...);

/**
 * Special debug method, that prints a hash value to stderr too.
 * 
 * @param i_type type of the debug message
 * @param cuca_hash the hash to print
 * @param i_hash_len the length of thee hash
 * @param cca_ptr the normal message
 */
void debugVerboseHash(int type, const unsigned char* cuca_hash, int i_hash_len, const char* cca_ptr, ...);

#endif

