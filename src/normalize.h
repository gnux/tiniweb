#ifndef __NORMALIZE_H__
#define __NORMALIZE_H__
#include "typedef.h"
#include <stdio.h>

/**
* Normalizes the Input after the definition from RFC
* returns an normalized struct you can find in typedef.h
* If you want, to normalize an cgi-response we just have to skip
* the first line.
* 
* @param ccp_header complete header stored in one big string
* @param b_cgiresponse bool false for http, true for cgi
* @return a pointer to normalized header struct
*/
http_norm *normalizeHttp(const char* ccp_header, bool b_cgiresponse);

/**
* Normalizes all fields defined in given structur
* 
* @param hnp_info Struct provided by the Normalizer
*/
void normalizeHeaderFields(http_norm* hnp_http_info);

/**
* Normalizes given line, makes a secRealloc
* 
* @param cpp_input pointer to the line
*/
void normalizeSingleLine(char** cpp_input);

/**
* Print the complete structure in verbose mode
* 
* @param hnp_info Struct provided by the Normalizer
*/
void printHttpNorm(http_norm* hnp_http_info);

/**
* Retrieves the header field form given input, allocates buffer
* 
* @param cpp_output pointer to output string
* @param ccp_input  pointer to string
*/
void getHeaderFieldName(char** cpp_output, const char* ccp_input);

/**
* Retrieves the field body form given input, allocates buffer
* 
* @param cpp_output pointer to output string
* @param ccp_input  pointer to string
*/
void getHeaderFieldBody(char** cpp_output, const char* ccp_input);

/**
* Checks if the input char is a blank or not
* 
* @param ccp_input pointer to string 
* @param i_offset position of char from the string
*/
int isBlank(const char* ccp_input, const size_t i_offset);

/**
* Checks if the input char is a New Line or not
* 
* @param ccp_input pointer to string 
* @param i_offset position of char from the string
*/
int isNewLineChars(const char* ccp_input, const size_t i_offset);

/**
* Checks if the input char is a blank or a New Line
* 
* @param ccp_input pointer to string 
* @param i_offset position of char from the string
*/
int isBlankNewLineChars(const char* ccp_input, const size_t i_offset);

/**
* Checks if the input char is bigger 32 and smaller 127
* 
* @param ccp_input pointer to string 
* @param i_offset position of char from the string
*/
int isCharacter(const char* ccp_input, const size_t i_offset);

/**
* Performs isBlankNewLineChars and isCharacter checking
* 
* @param ccp_input pointer to string 
* @param i_offset position of char from the string
*/
int isValid(const char* ccp_input, const size_t i_offset);

/**
* Checks if the string has an Valid Headerfield
* Definition of headerfields varied between cgi and http
* If you want to check an cgi response set b_skipfirstline to true 
* 
* @param ccp_input pointer to string
* @param b_cgiresponse true for CGI
*/
int isValidHeaderFieldStart(const char* ccp_input, bool b_cgiresponse);

#endif

