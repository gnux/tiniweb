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

static const char BLANKS[] = " \t";
static const char BLANKS_NEW_LINE[] = " \t\n";
static const char* METHOD[] = {"GET", "POST", "HEAD"};
static const char HTTPVERSION[] = "HTTP/1.1";

int MAX_BUFSIZE=0;
bool selectedMETHOD[] = {FALSE, FALSE, FALSE};



void parse(char* input, int max_bufsize){
	MAX_BUFSIZE = max_bufsize;
	fprintf(stderr, "tiniweb: your input %s \n", input);
	char** outputline = NULL;
	int offset = 0;
	offset = parseHttpRequestHeader(input, outputline, offset);
	if(offset != EXIT_FAILURE){
		//TODO: ERROR
	}
/*  	char* pch_get;
  	char* pch_host;
  	pch_get = strstr (input,"GET");
  	pch_host = strstr (input,"Host:");
	fprintf(stderr, "Found? %s \n", pch_get);
	fprintf(stderr, "Found? %s \n", pch_host);
	normalizeHeader(input);
	if(isChar('a')==TRUE){
		fprintf(stderr, "it was an a \n");
	}
*/

}

void normalizeHeader(char* input){
	/*fprintf(stderr, "Size of Input: %i \n", strlen(input));
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
	//fprintf(stderr, "Normalized: %s \n", normalized_input);*/
}



int parseHttpRequestHeader(char* input, char** outputline, int offset){
	

	if(parseRequestLine(input,outputline,offset)!=EXIT_FAILURE){
		fprintf(stderr, "Parser: this is an correct input \n");
		return TRUE;
	}
	return FALSE;
		
	
	
}
int parseRequestLine(char* input, char** outputline, int offset){
	
	int new_offset=0;
	new_offset = parseMethod(input,outputline,offset);
	if(new_offset!=EXIT_FAILURE){
		if(input[new_offset]== ' '){
			new_offset = offsetPP(new_offset, 1);
			new_offset = parseRequestURI(input,outputline,new_offset);
			if(new_offset!=EXIT_FAILURE){
				if(isWhiteSpace(input[new_offset])==TRUE){
					new_offset = offsetPP(new_offset, 1);
					new_offset = parseHttpVersion(input,outputline,new_offset);
					if(new_offset!=EXIT_FAILURE){
						if(isNewLine(input[new_offset])==TRUE){
							new_offset = offsetPP(new_offset, 1);
							return new_offset;
						}
					}
				}
			}
		}
	}
	return EXIT_FAILURE;
	
}

int parseMethod(char* input, char** outputline, int offset){
	bool was_right = TRUE;
	if(offset == 0){
		for(int i=0; i<=2; i++){
			was_right = TRUE;
			for(int j=0; j<strlen(METHOD[i]); j++){
				if(input[j]!=METHOD[i][j]){
					was_right = FALSE;
					break;
				}
			}
			if(was_right==TRUE){
				selectedMETHOD[i]=TRUE;
				offset=offsetPP(offset,3);
				if(i>0){
					offset=offsetPP(offset,4);
				}
				return offset;
			}
		}
	}
	
	return EXIT_FAILURE;	
}

int parseHttpVersion(char* input, char** outputline, int offset){
	
	bool istrue = TRUE;
	
	for(int i=0; i<strlen(HTTPVERSION);i++){
		if(HTTPVERSION[i]!=input[offset]){
			istrue = FALSE;
			break;
		}
		offset = offsetPP(offset,1);
	}
	if(istrue==TRUE){	
		return offset;
	}
	else
		return EXIT_FAILURE;
}

int parseHttpRequest(char* input, char** outputline, int offset){
	return offset;
}

int parseRequestURI(char* input, char** outputline, int offset){
	
	int i=0;
	char uri[100]; //TODO FIX this
	char* uri_ = uri;
	while(isWhiteSpace(input[offset])!=TRUE){
		uri_[i]=input[offset];
		i++;
		offset=offsetPP(offset,1);
	}
	return offset;
	
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

bool isNewLine(char input){
	if(input=='\n')
		return TRUE;
	return FALSE;
}

int offsetPP(int offset, int count){
	if(offset+count < MAX_BUFSIZE){
		offset= offset + count;
	}
	return offset;
}

