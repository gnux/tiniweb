#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "secmem.h"
#include "secstring.h"
#include "typedef.h"
#include "debug.h"

unsigned char* secPrint2String(unsigned char* ucp_format, ...)
{
  unsigned char* ucp_string = NULL;
  va_list va;
  va_start(va, ucp_format);
  if(vasprintf((char**) &ucp_string, (char*) ucp_format, va) == -1 || ucp_string == NULL)
  {
	  secAbort();
  }
  secRegister(ucp_string);
  va_end(va);
  return ucp_string;
}

void *secGetStringPart(const char* ccp_string, ssize_t start, ssize_t end){
	if(end < start || end > strlen(ccp_string))
		//TODO: something went wrong we would return NULL
		return NULL;
		//secAbort();
	ssize_t len = end - start + 2;
	ssize_t i;
	char *cp_fragment = secCalloc(len, sizeof(char));
	cp_fragment[len - 1] = '\0';
	for(i=0; i < len - 1; ++i)
	cp_fragment[i] = ccp_string[start + i];
	
	return cp_fragment;
}

void strAppend(char** cpp_output, const char* ccp_input){
	if(ccp_input == NULL)
	  return;
	if(!*cpp_output){
		*cpp_output = secCalloc(1, sizeof(char));
		(*cpp_output)[0] = '\0';
	}
	
	size_t i_len_input = strlen(ccp_input);
	size_t i_len_output = strlen(*cpp_output);
	size_t i_len_new = i_len_input + i_len_output + 1;
	
	// prevent overflow
	// TODO: search for possible overflows!
	if(i_len_new < i_len_input || i_len_new < i_len_output){
		debugVerbose(NORMALISE, "Error in strAppend possible buffer overflow detected!\n");
		secAbort();
	}
	
	*cpp_output = secRealloc(*cpp_output, i_len_new);
	strncat(*cpp_output, ccp_input, i_len_input);
	
	// just be sure to delimit with '\0'
	(*cpp_output)[i_len_new - 1] = '\0';
}

void strAppendFormatString(char** cpp_output, const char* cucp_format, ...)
{
	unsigned char* ucp_form = NULL;
	va_list va;
  va_start(va, cucp_format);
  if(vasprintf((char**) &ucp_form, (char*) cucp_format, va) == -1 || ucp_form == NULL)
  {
	  secAbort();
  }
  strAppend(cpp_output, ucp_form);
  free(ucp_form);
  va_end(va);

}

