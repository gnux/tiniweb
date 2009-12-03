#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "secmem.h"
#include "secstring.h"
#include "typedef.h"
#include "debug.h"
#include "parser.h"

unsigned char* secPrint2String(const unsigned char* cucp_format, ...)
{
  unsigned char* ucp_string = NULL;
  va_list va;
  va_start(va, cucp_format);
  if(vasprintf((char**) &ucp_string, (char*) cucp_format, va) == -1 || ucp_string == NULL)
  {
	  secAbort();
  }
  secRegister(ucp_string);
  va_end(va);
  return ucp_string;
}

void *secGetStringPart(const unsigned char* cucp_string, ssize_t start, ssize_t end){
	if(end < start || end > strlen(cucp_string))
		//TODO: something went wrong we would return NULL
		return NULL;
		//secAbort();
	ssize_t len = end - start + 2;
	ssize_t i;
	char *cp_fragment = secCalloc(len, sizeof(char));
	cp_fragment[len - 1] = '\0';
	for(i=0; i < len - 1; ++i)
	cp_fragment[i] = cucp_string[start + i];
	
	return cp_fragment;
}

void strAppend(unsigned char** ucpp_output, const unsigned char* cucp_input){
	if(cucp_input == NULL)
	  return;
	if(!*ucpp_output){
		*ucpp_output = secCalloc(1, sizeof(char));
		(*ucpp_output)[0] = '\0';
	}
	
	size_t i_len_input = strlen((const char*) cucp_input);
	size_t i_len_output = strlen(*ucpp_output);
	size_t i_len_new = i_len_input + i_len_output + 1;
	
	// prevent overflow
	// TODO: search for possible overflows!
	if(i_len_new < i_len_input || i_len_new < i_len_output){
		debugVerbose(NORMALISE, "Error in strAppend possible buffer overflow detected!\n");
		secAbort();
	}
	
	*ucpp_output = secRealloc(*ucpp_output, i_len_new);
	strncat((char*) *ucpp_output, (const char*) cucp_input, i_len_input);
	
	// just be sure to delimit with '\0'
	(*ucpp_output)[i_len_new - 1] = '\0';
}

void strAppendFormatString(unsigned char** ucpp_output, const unsigned char* cucp_format, ...)
{
	unsigned char* ucp_form = NULL;
	va_list va;
  va_start(va, cucp_format);
  if(vasprintf((char**) &ucp_form, (char*) cucp_format, va) == -1 || ucp_form == NULL)
  {
	  secAbort();
  }
  strAppend(ucpp_output, ucp_form);
  free(ucp_form);
  va_end(va);

}


void stringToUpperCase(unsigned char* ucp_input){
	//just convert every char from the string to an uppercase char
	for(int i=0; i<strlen(ucp_input);i++){
		ucp_input[i] = toupper(ucp_input[i]);
		if(ucp_input[i] == '-')
			ucp_input[i] = '_';
	}	
}



unsigned int strDecodeHexToUInt(unsigned char* cp_string, ssize_t i_offset, ssize_t i_len){
	//If we found an % and the next to chars were hexdigit we decode them
	unsigned int result = 0;

	ssize_t i;
	if(i_offset + i_len > strlen(cp_string))
		i_len = strlen(cp_string) - i_offset;
	for(i = 0; i < i_offset; ++i)
	{
		result = result << 4;
		result += hextodec(cp_string[i_offset + i]);
	}
//	unsigned char a;
//	unsigned char b;
//	a = cp_string[i_offset + 1];
//	b = cp_string[i_offset + 2];
//	hextodec(&a);
//	hextodec(&b);
	return result;
	//return a*16 + b;
}

unsigned char hextodec(unsigned char a){
	//calculate the correct char
	if(a>47 && a<58)
		return a - 48;
	else if(a>64 && a<71)
		return a - 55;
	else if(a>96 && a<103)
		return a -= 87;
	return 0;
}
