/** tiniweb
 * \file parser.h
 * \author Patrick Plaschzug
 */
 
#ifndef PARSER_H_
#define PARSER_H_
#include "typedef.h"

/**
 *
 */
void parse(http_norm *hnp_info);

/**
 *
 */
int parseArguments(http_norm *hnp_info);

/**
 *
 */
int parseHttpRequestHeader(char* input);

/**
 *
 */
int parseRequestLine(char* input);

/**
 *
 */
int parseMethod(char* input, int offset);

/**
 *
 */
int parseRequestURI(char* input, int offset);

/**
 *
 */
int parseHttpVersion(char* input, int offset);

/**
 *
 */
int validateString(char* cp_string);

/**
 *
 */
bool isNonEscapedChar(char* input, int i_offset);

/**
 *
 */
bool isHexDigit(char input);

/**
 *
 */
void stringToUpperCase(char* input);



//void normalizeHeader(char input[]);
//bool isChar(char input);
//bool isWhiteSpace(char input);
//bool isNewLine(char input);
//bool isEOF(char input);
//int parseHttpRequest(char* input, char** outputline, int offset);
//int offsetPP(int offset, int count);

#endif /*PARSER_H_*/
