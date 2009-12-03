/** tiniweb
 * \file secstring.h
 * \author Georg Neubauer
 */

#ifndef __SEC_STRING_H__
#define __SEC_STRING_H__

#include <stdlib.h>
#include <stdio.h>


unsigned char* secPrint2String(unsigned char* ucp_format, ...);

void strAppendFormatString(char** cpp_output, const char* cucp_format, ...);

void strAppend(char** cpp_output, const char* ccp_input);


void *secGetStringPart(const char* ccp_string, ssize_t start, ssize_t end);


#endif

