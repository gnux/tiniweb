/** tiniweb
 * \file parser.h
 * \author Patrick Plaschzug
 */
 
#ifndef PARSER_H_
#define PARSER_H_
#include "typedef.h"


void parse(http_norm *hnp_info);
void normalizeHeader(char input[]);
bool isChar(char input);
bool isWhiteSpace(char input);
bool isNewLine(char input);
void stringToUpperCase(char* input);

int parseHttpRequestHeader(char* input, char** outputline, int offset);
int parseRequestLine(char* input, char** outputline, int offset);
int parseMethod(char* input, char** outputline, int offset);
int parseArguments(http_norm *hnp_info, char** outputline);
int parseHttpVersion(char* input, char** outputline, int offset);
int parseHttpRequest(char* input, char** outputline, int offset);
int parseRequestURI(char* input, char** outputline, int offset);

int offsetPP(int offset, int count);



#endif /*PARSER_H_*/
