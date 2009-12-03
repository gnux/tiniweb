/** tiniweb
 * \file secstring.h
 * \author Georg Neubauer
 */

#ifndef __SEC_STRING_H__
#define __SEC_STRING_H__

#include <stdlib.h>
#include <stdio.h>


unsigned char* secPrint2String(const unsigned char* cucp_format, ...);

void strAppendFormatString(unsigned char** ucpp_output, const unsigned char* cucp_format, ...);

void strAppend(unsigned char** ucpp_output, const unsigned char* cucp_input);


void *secGetStringPart(const unsigned char* cucp_string, ssize_t start, ssize_t end);


unsigned int strDecodeHexToUInt(unsigned char* cp_string, ssize_t i_offset, ssize_t i_len);

/**
 * Transforms an String to an Upper Case String
 * 
 * @params char pointer to string wich should get transformed
 */
void stringToUpperCase(unsigned char* ucp_input);


unsigned char hextodec(unsigned char a);

#endif