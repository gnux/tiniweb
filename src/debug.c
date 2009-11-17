#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "debug.h"
#include "typedef.h"

extern unsigned char sb_flag_verbose_;

static const char* SCCA_DEBUG_TYPES[] = {
  "MAIN", //0
  "SEC_MEM", //1
  "CGI", //2
  "\0"
};

void debug(int type, const char *ptr, ...){
  va_list va;
  fprintf(stderr,"%s>> ", SCCA_DEBUG_TYPES[type]);
  va_start(va, ptr);
  vfprintf(stderr, ptr, va);
  va_end(va);
}

void debugVerbose(int type, const char *ptr, ...){
  va_list va;
  if(!sb_flag_verbose_)
    return;
  fprintf(stderr,"%s_VERBOSE>> ", SCCA_DEBUG_TYPES[type]);
  va_start(va, ptr);
  vfprintf(stderr, ptr, va);
  va_end(va);
}

