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
#include <ctype.h>
#include "typedef.h"
#include "parser.h"
#include "secmem.h"
#include "normalize.h"
#include "debug.h"
#include "envvar.h"


static const char BLANKS[] = " \t";
static const char BLANKS_NEW_LINE[] = " \t\n";
static const char* METHOD[] = {"GET", "POST", "HEAD"};
static const char HTTPVERSION[] = "HTTP/1.1";

int MAX_HEADER_BUFSIZE=0;
bool selectedMETHOD[] = {FALSE, FALSE, FALSE};
bool foundARGUMENTS[] = {FALSE};
/*
 * - char* first line
- char* normalisierte http_req/rep
- char* body
- char** header_fields
- char** header_bodys
- size_t anzahl_fields
 */

void parse(http_norm *hnp_info){
	
	
	char** outputline = NULL;	
	MAX_HEADER_BUFSIZE = strlen(hnp_info->cp_header);
	int offset = 0;
	int arguments_valid = 0;
	int first_line_valid =  parseHttpRequestHeader(hnp_info->cp_first_line, outputline, offset);
	if(first_line_valid == EXIT_FAILURE){
		//TODO: ERROR
	}
	
	if(selectedMETHOD[0]==TRUE){
		appendToEnvVarList("REQUEST_METHOD","GET");
		/*if(hnp_info->cpp_header_field_body != NULL){
			arguments_valid=EXIT_FAILURE;
			//TODO: ERROR
		}
		else*/
			arguments_valid = parseArguments(hnp_info,outputline);
	}
	else if(selectedMETHOD[1]==TRUE){
		appendToEnvVarList("REQUEST_METHOD","POST");
		if(hnp_info->cpp_header_field_body == NULL){
			arguments_valid=EXIT_FAILURE;
			//TODO: ERROR
		}
		else
			arguments_valid = parseArguments(hnp_info,outputline);
	}
	else if(selectedMETHOD[2]==TRUE){
		appendToEnvVarList("REQUEST_METHOD","HEAD");
		if(hnp_info->cpp_header_field_body != NULL){
			arguments_valid=EXIT_FAILURE;
			//TODO: ERROR
		}
		else
			arguments_valid = parseArguments(hnp_info,outputline);
	}
	else{
		//can't happen
	}
	
	if(arguments_valid==EXIT_FAILURE){
		//TODO: ERROR
	}
	debugVerbose(3, "Arguments valid\n");

}

int parseArguments(http_norm *hnp_info, char** outputline){
	debugVerbose(3, "Parse Arguments\n");
	
	
	for(size_t i = 0; i < hnp_info->i_num_fields; ++i){
		char cp_name_to_add[] = "HTTP_";
		char* cp_name = NULL;
		strAppend(&cp_name, cp_name_to_add);
		strAppend(&cp_name, hnp_info->cpp_header_field_name[i]);
		stringToUpperCase(cp_name);
		appendToEnvVarList(cp_name,hnp_info->cpp_header_field_body[i]);
	}


	return TRUE;	
}

int parseHttpRequestHeader(char* input, char** outputline, int offset){
	
	debugVerbose(3, "Parse HttpRequestHeader\n");
	offset = parseRequestLine(input,outputline,offset);
	if(offset!=EXIT_FAILURE){
		debugVerbose(3, "Method found\n");
		return TRUE;
	}
	return EXIT_FAILURE;	
}

int parseRequestLine(char* input, char** outputline, int offset){
	
	int new_offset=0;
	new_offset = parseMethod(input,outputline,offset);
	if(new_offset!=EXIT_FAILURE){
		if(input[new_offset]== ' '){
			new_offset = offsetPP(new_offset, 1);
			new_offset = parseRequestURI(input,outputline,new_offset);
			if(new_offset!=EXIT_FAILURE && isEOF(input[new_offset])==FALSE){
				if(isWhiteSpace(input[new_offset])==TRUE){
					new_offset = offsetPP(new_offset, 1);
					new_offset = parseHttpVersion(input,outputline,new_offset);
					if(new_offset!=EXIT_FAILURE){
						if(isEOF(input[new_offset])==TRUE){
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
		//appendToEnvVarList("SERVER_PROTOCOL","HTTP/1.1");	
		return offset;
	}
	else
		return EXIT_FAILURE;
}

int parseHttpRequest(char* input, char** outputline, int offset){
	return offset;
}

int parseRequestURI(char* input, char** outputline, int offset){
	
//##################################################################################
	char* cp_uri = NULL;
	char* cp_fragment = NULL;
	char* cp_request = NULL;
	char* my_char = NULL;
	char* cgi_path = NULL;
	bool fragment_found = FALSE;
	bool query_found = FALSE;
	bool cgi_bin_found = FALSE;
	int save_position = 0;
	
	

	if(input[offset]=='/'){
		
		if(strncmp("/cgi-bin/",input+offset,9)==0){
			cgi_bin_found = TRUE;
			debugVerbose(3, "CGI bin\n");
			/*for(int i = offset+9; i< strlen(input)-1;i++){
				if(input[i]=='/'){
					save_position = i;
				}
				if(isWhiteSpace(input[i])==TRUE || isEOF(input[i])==TRUE || input[i]=='#' || input[i]=='?')
					break;
			}
			strncpy(cgi_path,input+offset+9,save_position);*/
			debugVerbose(3, "CGI path %s\n",cgi_path);
		}
		
		
		
		while(isWhiteSpace(input[offset])==FALSE && isEOF(input[offset])==FALSE){
			my_char = input[offset];
			
			if(isNonEscapedChar(input[offset])==TRUE)
				return EXIT_FAILURE;
			strAppend(&cp_uri, &my_char);
			
			if(input[offset]=='#'){
				fragment_found = TRUE;
			}
			else if(input[offset]=='?'){
				query_found = TRUE;
				fragment_found = FALSE;
			}
			else if(fragment_found == TRUE){
				strAppend(&cp_fragment, &my_char);
			}
			else if(query_found == TRUE){
				strAppend(&cp_request, &my_char);
			}
			else{
				//??
			}
			offset=offsetPP(offset,1);
			
		}
		if(isEOF(input[offset]==TRUE)){
			appendToEnvVarList("SERVER_PROTOCOL","HTTP/1.1");
		}
		
		appendToEnvVarList("REQUEST_URI",cp_uri);
		//appendToEnvVarList("FRAGMENT",cp_fragment);
		appendToEnvVarList("QUERY_STRING",cp_request);
		return offset;
	}
	else{
		return EXIT_FAILURE;
	}
	
}

bool isChar(char input){
	if(input < 32 || input > 126 || input!='\t')
		return FALSE;
	else
		return TRUE;
}

bool isNonEscapedChar(char input){
	
	//Ohne Fragezeichen
	if(input==' ' || input=='%' || input==';' || input==':' || input=='@')
		return TRUE;
	if(input=='&' || input=='=' || input=='+' || input=='$' || input==',')
		return TRUE;
	return FALSE;
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
bool isEOF(char input){
	if(input=='\0')
		return TRUE;
	return FALSE;
}

int offsetPP(int offset, int count){
	if(offset+count < MAX_HEADER_BUFSIZE){
		offset= offset + count;
	}
	return offset;
}

void stringToUpperCase(char* input){
	
	for(int i=0; i<strlen(input);i++){
		input[i] = toupper(input[i]);
	}	
}

