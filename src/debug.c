#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "debug.h"
#include "typedef.h"

extern unsigned char sb_flag_verbose_;

char* getTypeString(int type)
{
    switch(type)
    {
        case AUTH:
            return "AUTH";
            break;
        case SEC_MEM:
            return "SEC_MEM";
            break;
        case NORMALISE:
            return "NORMALISE";
            break;
        case PARSER:
            return "PARSER";
            break;
        case ENVVAR:
            return "ENVVAR";
            break;
        case CGICALL:
            return "CGICALL";
            break;
        case PATH:
            return "PATH";
            break;
        default:
            return "UNSPECIFIED";
    }    
}

void debug(int type, const char *ptr, ...){
  va_list va;
  fprintf(stderr,"%s>> ", getTypeString(type));
  va_start(va, ptr);
  vfprintf(stderr, ptr, va);
  va_end(va);
}

void debugVerbose(int type, const char *ptr, ...){
  va_list va;
  if(!sb_flag_verbose_)
    return;
  fprintf(stderr,"%s_VERBOSE>> ", getTypeString(type));
  va_start(va, ptr);
  vfprintf(stderr, ptr, va);
  va_end(va);
}

void debugVerboseHash(int type, const unsigned char* cuca_hash, int i_hash_len, const char* cca_ptr, ...)
{
    va_list va;
    int i = 0;
    if(!sb_flag_verbose_)
        return;
    
    fprintf(stderr,"%s_VERBOSE>> ", getTypeString(type));
    
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
