#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "debug.h"
#include "typedef.h"

extern unsigned char sb_flag_verbose_;

static const char* SCCA_DEBUG_TYPES[] = {
  "MAIN", //0
  "SEC_MEM", //1
  "NORMALISE", //2
  "PARSER", //3
  "ENVVAR", //4
  "CGICALL", //5
  "AUTH", //6
  "\0"
};

void debug(size_t type, const char *ptr, ...){
  va_list va;
  fprintf(stderr,"%s>> ", SCCA_DEBUG_TYPES[type]);
  va_start(va, ptr);
  vfprintf(stderr, ptr, va);
  va_end(va);
}

void debugVerbose(size_t type, const char *ptr, ...){
  va_list va;
  if(!sb_flag_verbose_)
    return;
  fprintf(stderr,"%s_VERBOSE>> ", SCCA_DEBUG_TYPES[type]);
  va_start(va, ptr);
  vfprintf(stderr, ptr, va);
  va_end(va);
}

void debugVerboseHash(int i_type, const unsigned char* cuca_hash, int i_hash_len, const char* cca_ptr, ...)
{
    va_list va;
    int i = 0;
    if(!sb_flag_verbose_)
        return;
    
    fprintf(stderr,"%s_VERBOSE>> ", SCCA_DEBUG_TYPES[i_type]);
    
    va_start(va, cca_ptr);
    vfprintf(stderr, cca_ptr, va);
    va_end(va);
    
    fprintf(stderr, " Hash: ");
    
    for (i = 0; i < i_hash_len; i++)
    {
            fprintf(stderr, "%x", cuca_hash[i]);
    }
    
    fprintf(stderr, "\n");
    
}
