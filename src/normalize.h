#ifndef __NORMALIZE_H__
#define __NORMALIZE_H__
#include "typedef.h"

http_norm *normaliseHttp(const char* ccp_input);
int isBlank(const char* cpp_input, const int i_offset);
int isNewLineChars(const char* cpp_input, const int i_offset);
int isCharacter(const char* cpp_input, const int i_offset);

#endif