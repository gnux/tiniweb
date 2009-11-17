/** tiniweb
 * \file parser.h
 * \author Patrick Plaschzug
 */
 
#ifndef PARSER_H_
#define PARSER_H_
#include "typedef.h"


void parse(char input[], int max_bufsize);
void normalizeHeader(char input[]);
bool isChar(char input);
bool isWhiteSpace(char input);
bool isNewLine(char input, char input2);

bool parseHttpRequestHeader(char* input, char** outputline);
bool parseRequestLine(char* input, char** outputline);
bool parseMethod(char* input, char** outputline);
bool parseHttpVersion(char* input, char** outputline);
bool parseHttpRequest(char* input, char** outputline);



#endif /*PARSER_H_*/
