/** tiniweb
 * \file parser.c
 * \author Patrick Plaschzug
 */

/*typedef struct header_info {
    char[] field;
    char[] data;
    header_info *next;
    
} header_info;

typedef struct body_info {
    char[] field;
    char[] data;
    header_info *next;
    
} body_info;*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "typedef.h"
#include "parser.h"
#include "secmem.h"

const char BLANKS[] = " \t";
const char BLANKS_NEW_LINE[] = " \t\n";

void parse(char input[], int max_bufsize){
	fprintf(stderr, "tiniweb: your input %s \n", input);
  	char* pch_get;
  	char* pch_host;
  	pch_get = strstr (input,"GET");
  	pch_host = strstr (input,"Host:");
	fprintf(stderr, "Found? %s \n", pch_get);
	fprintf(stderr, "Found? %s \n", pch_host);
	normalizeHeader(input);
	if(isChar('a')==TRUE){
		fprintf(stderr, "it was an a \n");
	}

}

void normalizeHeader(char input[]){
	fprintf(stderr, "Size of Input: %i \n", strlen(input));
	char normalized_input[strlen(input)][strlen(input)];
	int j=0;
	int line_count=0;
	for(int i=0; input[i]!='\0' && i<= strlen(input);i++){
		if(isWhiteSpace(input[i])==TRUE){
			//Do nothing except the sign bevor this sign wasn't a whitespace
			// and the next sign isn't a :
			if(i>0 && i<strlen(input)){
				if((isWhiteSpace(input[i-1])==FALSE) && input[i+1]!=':'){
					normalized_input[line_count][j]=' ';
					j++;
				}	
			}
		}
		else{
			//Check if we made a mistake and ther is an space bevor our : in the normalized_input
			if(i>0 && i<strlen(input)){
				if(input[i]==':' && isWhiteSpace(normalized_input[line_count][j-1])==TRUE){
					j--;
				}
			}
			//Copy input to normalized_input
			if(isChar(input[i]==TRUE)){
				normalized_input[line_count][j]=input[i];
				j++;
				//Now we have to check if after the : an whitespace is going to be written next time or not
				if(i>0 && i<strlen(input)){
					if(input[i]==':' && isWhiteSpace(input[i+1])==FALSE){
						normalized_input[line_count][j]=' ';
						j++;
					}
				}
			}
			else if(isNewLine(input[i],input[i+1])==FALSE){
				line_count++;
			}
			else{
				//we don't know this char
			}
			
			
		}
	}
	normalized_input[line_count][j]='\0';
	//secRealloc(normalized_input, (strlen(normalized_input) + 1) * sizeof(char));
	for(int i=0; i<=line_count;i++){
		fprintf(stderr, "Normalized: %s \n", normalized_input[i]);
	}
	//fprintf(stderr, "Normalized: %s \n", normalized_input);
}

bool isChar(char input){
	if(input < 32 || input > 126 || input!='\t')
		return FALSE;
	else
		return TRUE;
}

bool isWhiteSpace(char input){
	if(input==' ' || input=='\t')
			return TRUE;
	return FALSE;
}

bool isNewLine(char input, char input2){
	if(input=='\n' || (input=='\r' && input2=='\n'))
		return TRUE;
	return FALSE;
}
/*bool isBlankNewLine(char input)
{
	int i;
	for(i = 0; BLANKS_NEW_LINE[i] != '\0'; ++i)
		if(BLANKS_NEW_LINE[i] == input)
			return TRUE;
	return FALSE;
}

bool checkLineValidChars(const char* line)
{
	size_t i;
	for(i = 0; '\0' != line[i]; ++i)
		if(line[i] < 32 || line[i] > 126)
			if(FALSE == isBlankNewLine(line[i]))
					return TRUE;
	return FALSE;
}

bool isEmptyLine(char* line)
{
	size_t i;
	for(i = 0; '\0' != line[i]; ++i)
		if(FALSE == isBlankNewLine(line[i]))
			return FALSE;
	return TRUE;
}*/

