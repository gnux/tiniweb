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

bool isBlankNewLine(char input);
bool checkLineValidChars(const char* line);
bool isEmptyLine(char* line);



#endif /*PARSER_H_*/
