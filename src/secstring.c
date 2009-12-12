/** secstring.c
* Implementation of secString functions
* \file secstring.c
* \author Patrick Plaschzug, Christian Partl, Georg Neubauer, Dieter Ladenhauf
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "secmem.h"
#include "secstring.h"
#include "typedef.h"
#include "debug.h"
#include "parser.h"
#include "pipe.h"
#include "normalize.h"

char* secPrint2String(const char* ccp_format, ...)
{
	char* ucp_string = NULL;
	va_list va;
	va_start(va, ccp_format);
	if(vasprintf(&ucp_string, ccp_format, va) == -1 || ucp_string == NULL)
	{
		secAbort();
	}
	secRegister(ucp_string);
    va_end(va);
	return ucp_string;
}

void *secGetStringPart(const char* ccp_string, ssize_t start, ssize_t end)
{
	if(end < start || end > strlen(ccp_string) || end < 0 || start < 0)
		return NULL;
	ssize_t len = end - start + 2;
	ssize_t i;
	char *cp_fragment = secCalloc(len, sizeof(char));
	cp_fragment[len - 1] = '\0';
	for(i=0; i < len - 1; ++i)
		cp_fragment[i] = ccp_string[start + i];
	
	return cp_fragment;
}

ssize_t getNextLineFromString(const char* ccp_string, char** cpp_current_line, ssize_t i_offset)
{
	ssize_t i = 0;
	if(*cpp_current_line != NULL)
		secFree(*cpp_current_line);
	
	for(i = i_offset; i < strlen(ccp_string) && ccp_string[i] != '\n'; ++i);
	*cpp_current_line = secGetStringPart(ccp_string, i_offset, i);
	return i + 1;
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
	if(i_len_new < i_len_input || i_len_new < i_len_output){
		debugVerbose(NORMALISE, "Error in strAppend possible buffer overflow detected!\n");
		secAbort();
	}
	
	*cpp_output = secRealloc(*cpp_output, i_len_new * sizeof(char));
	strncat(*cpp_output, ccp_input, i_len_input);
	
	// just be sure to delimit with '\0'
	(*cpp_output)[i_len_new - 1] = '\0';
}

void strAppendFormatString(char** cpp_output, const char* ccp_format, ...)
{
	char* cp_form = NULL;
	va_list va;
	va_start(va, ccp_format);
	if(vasprintf(&cp_form, ccp_format, va) == -1 || cp_form == NULL)
	{
		secAbort();
	}
	strAppend(cpp_output, cp_form);
	free(cp_form);
	va_end(va);
	
}

void stringToUpperCase(char* cp_input){
	//just convert every char from the string to an uppercase char
	for(int i=0; i<strlen(cp_input);i++){
		cp_input[i] = toupper(cp_input[i]);
		if(cp_input[i] == '-')
			cp_input[i] = '_';
	}	
}

unsigned long strDecodeHexToULong(char* cp_string, ssize_t i_offset, ssize_t i_len)
{
	//If we found an % and the next to chars were hexdigit we decode them
	long result = 0;
	ssize_t i;
	if(i_offset + i_len > strlen(cp_string))
		i_len = strlen(cp_string) - i_offset;
	for(i = 0; i < i_len; ++i)
	{
		result = result << 4;
		result += hextodec(cp_string[i_offset + i]);
	}
	return result;
}

char hextodec(char c){
	//calculate the correct char
	if(c>47 && c<58)
		return c - 48;
	else if(c>64 && c<71)
		return c - 55;
	else if(c>96 && c<103)
		return c -= 87;
	return 0;
}

bool isHexDigit(char c){
	//check if the char is in a correct hexdigit range
	if(c>0x29 && c < 0x3A)
		return TRUE;
	else if(c>0x40 && c < 0x47)
		return TRUE;
	else if(c>0x60 && c < 0x67)
		return TRUE;
	else
		return FALSE;
}

