#ifndef __NORMALIZE_H__
#define __NORMALIZE_H__
#include "typedef.h"
#include <stdio.h>

//http_norm *normalizeHttp(FILE* fp_input, bool b_skipfirstline);
http_norm *normalizeHttp(int fd, bool b_skipfirstline);
int isBlank(const char* ccp_input, const size_t i_offset);
int isNewLineChars(const char* ccp_input, const size_t i_offset);
int isCharacter(const char* ccp_input, const size_t i_offset);
int isValidHeaderFieldStart(const char* ccp_input, bool b_skipfirstline);
//void strAppend(char** cpp_output, const char* ccp_input);
void getHeaderFieldBody(char** cpp_output, const char* ccp_input);
void getHeaderFieldName(char** cpp_output, const char* ccp_input);
void printHttpNorm(http_norm* hnp_http_info);
void normalizeHeaderFields(http_norm* hnp_http_info);
int isBlankNewLineChars(const char* ccp_input, const size_t i_offset);
int isValid(const char* ccp_input, const size_t i_offset);
void normalizeSingleLine(char** cpp_input);
void restoreNormalizedHeader();
#endif
