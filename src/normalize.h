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
 * @param fp_input pointer to File you want to read from
 * @para, b_skipfirstline bool false for http, true for cgi
 */
http_norm *normalizeHttp(FILE* fp_input, bool b_skipfirstline);

/**
 * Checks if the input char is a blank or not, blanks are defined here SCCP_BLANK
 * 
 * @param ccp_input pointer to string 
 * @param i_offset position of char from the string
 */
int isBlank(const char* ccp_input, const size_t i_offset);

/**
 * Checks if the input char is a New Line or not, blanks are defined here SCCPA_NEW_LINE
 * 
 * @param ccp_input pointer to string 
 * @param i_offset position of char from the string
 */
int isNewLineChars(const char* ccp_input, const size_t i_offset);

/**
 * Checks if the input char is bigger 32 and smaller 127
 * 
 * @param ccp_input pointer to string 
 * @param i_offset position of char from the string
 */
int isCharacter(const char* ccp_input, const size_t i_offset);

/**
 * Checks if the string has an Valid Headerfield
 * Definition of headerfields varied between cgi and http
 * If you want to check an cgi response set b_skipfirstline to true 
 * 
 * @param ccp_input pointer to string
 * @b_skipfirstline true for CGI
 */
int isValidHeaderFieldStart(const char* ccp_input, bool b_skipfirstline);
/**
 * Appends the input string to the output string, it always
 * determinate with '\0'
 * 
 * @param ccp_output double pointer to string where it should be appanded
 * @param ccp_input  pointer to string you want to appand
 */
void strAppend(char** cpp_output, const char* ccp_input);
/**
 * lalal
 * 
 * @param ccp_output double pointer to string 
 * @param ccp_input  pointer to string
 */
void getHeaderFieldBody(char** cpp_output, const char* ccp_input);
/**
 * lalal
 * 
 * @param ccp_output double pointer to string 
 * @param ccp_input  pointer to string
 */
void getHeaderFieldName(char** cpp_output, const char* ccp_input);
/**
 * lalal
 * 
 * @param hnp_info Struct provided by the Normalizer
 */
void printHttpNorm(http_norm* hnp_http_info);
/**
 * lalal
 * 
 * @param hnp_info Struct provided by the Normalizer
 */
void normalizeHeaderFields(http_norm* hnp_http_info);
/**
 * lalal
 * 
 * @param lalal description
 */
int isBlankNewLineChars(const char* ccp_input, const size_t i_offset);
/**
 * lalal
 * 
 * @param lalal description
 */
int isValid(const char* ccp_input, const size_t i_offset);
/**
 * lalal
 * 
 * @param lalal description
 */
void normalizeSingleLine(char** cpp_input);
/**
 * lalal
 * 
 * @param lalal description
 */
void restoreNormalizedHeader();

#endif
