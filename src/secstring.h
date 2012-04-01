/** 
 * Copyright 2009-2012
 * 
 * Dieter Ladenhauf
 * Georg Neubauer
 * Christian Partl
 * Patrick Plaschzug
 * 
 * This file is part of Wunderwuzzi.
 * 
 * Wunderwuzzi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wunderwuzzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wunderwuzzi. If not, see <http://www.gnu.org/licenses/>.
 * 
 * -------------------------------------------------------------------
 * 
 * secstring.h
 * Definition of secString functions
 * 
 * \file secstring.h
 * \author Patrick Plaschzug, Christian Partl, Georg Neubauer, Dieter Ladenhauf
*/

#ifndef __SEC_STRING_H__
#define __SEC_STRING_H__

#include <stdlib.h>
#include <stdio.h>
#include "typedef.h"
#include "pipe.h"

/**
* Print format string into a buffer allocated by secMemory
*
* @param ccp_format the format string
* @param ... format values
* @return freshly allocated and formated string
*/
char* secPrint2String(const char* ccp_format, ...);

/**
* Append format string to existing string.
* Is able to handle NULL pointers.
*
* @param cpp_output output string
* @param ccp_format the format string
* @param ... format values
*/
void strAppendFormatString(char** cpp_output, const char* ccp_format, ...);

/**
* Appends the input string to the output string, it always
* determinate with '\0'. If NULL Pointer is given a new string
* is allocated by secMemory.
* 
* @param cpp_output double pointer to string where it should be appanded
* @param ccp_input pointer to string you want to append
*/
void strAppend(char** cpp_output, const char* ccp_input);

/**
* Get a part from a string determined by start and end offset.
* Returned string part is freshly allocated by secMemory.
* 
* @param ccp_output string to get part from
* @param start offset from where to start
* @param end offset where to end
* @return string that holds requested part
*/
void *secGetStringPart(const char* ccp_string, ssize_t start, ssize_t end);

/**
* Decode a given hex string to an unsigned long value.
*
* @param cp_string string to decode
* @param i_offset where to start
* @param i_len decode how many bytes from offset
* @return decoded value
*/
unsigned long strDecodeHexToULong(char* cp_string, ssize_t i_offset, ssize_t i_len);

/**
* Transforms an String to an Upper Case String
*
* @param char pointer to string wich should get transformed
*/
void stringToUpperCase(char* cp_input);

/**
* Decode one hex char to its dezimal representation.
*
* @param a the char
* @return decoded value
*/
char hextodec(char a);

/**
* Tries to check if the escaped chars hexdigit is correct
* 
* @param input char whiche should get checked
* @return check result
*/
bool isHexDigit(char c);

/**
* getNextLineFromString routine, is used like getline but gets next line
* delimited by '\n' from memory string. Uses secGetStringPart function.
* Auto frees given valid pointers.
* 
* @param ccp_string string where to get the line
* @param cpp_current_line where to store the line into
* @param i_offset where to start search for next line
* @return offset + 1 of last found '\n'
*/
ssize_t getNextLineFromString(const char* ccp_string, char** cpp_current_line, ssize_t i_offset);

#endif

