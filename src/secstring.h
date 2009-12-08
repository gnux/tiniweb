/** tiniweb
 * \file secstring.h
 * \author Georg Neubauer
 */

#ifndef __SEC_STRING_H__
#define __SEC_STRING_H__

#include <stdlib.h>
#include <stdio.h>
#include "typedef.h"
#include "pipe.h"


char* secPrint2String(const char* ccp_format, ...);

void strAppendFormatString(char** cpp_output, const char* ccp_format, ...);

void strAppend(char** cpp_output, const char* ccp_input);


void *secGetStringPart(const char* ccp_string, ssize_t start, ssize_t end);


long strDecodeHexToUInt(char* cp_string, ssize_t i_offset, ssize_t i_len);

/**
 * Transforms an String to an Upper Case String
 * 
 * @params char pointer to string wich should get transformed
 */
void stringToUpperCase(char* cp_input);


char hextodec(char a);

/**
 * Tries to check if the escaped chars hexdigit is correct
 * 
 * @params input char whiche should get checked
 */
bool isHexDigit(char c);

/**
 * secGetline routine, is used like getline but uses secMemory functions
 * every given pointer is tried to free, and new pointers are registered
 * therefor every pointer should be secure
 */
ssize_t secGetlineFromFDWithPollin(char** cpp_lineptr, int fd);


#endif

