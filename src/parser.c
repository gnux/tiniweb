/** tiniweb
* \file parser.c
* \author Patrick Plaschzug, Georg Neubauer
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "typedef.h"
#include "parser.h"
#include "secmem.h"
#include "normalize.h"
#include "debug.h"
#include "envvar.h"

static const char* SCCA_KNOWN_METHODS[] = {"GET", "POST", "HEAD"};
static const int SCI_NUM_KNOWN_METHODS = 3;
static const char* SCCP_KNOWN_HTTPVERSION = "HTTP/1.1";
static const char* SCCP_CGI_BIN ="/cgi-bin/";
static const char* SCCP_HTTP_HEADER_FIELD_MARKER = "HTTP_";
enum SCE_KNOWN_METHODS e_used_method = UNKNOWN;

bool B_CGI_BIN_FOUND = FALSE;
//bool B_HOST_FOUND = FALSE;
bool B_BODY_FOUND = FALSE;
bool B_CONTENT_LENGTH_FOUND = FALSE;
bool B_SEARCH_AUTORIZATION = FALSE;
bool B_AUTORIZATION_FOUND = FALSE;
bool B_RESPONSE_HEADER = FALSE;
bool B_REQUEST_HEADER = FALSE;
//TODO: really use global variable???
http_autorization* http_autorization_ = NULL;
http_cgi_response *http_cgi_response_ = NULL;
http_request *http_request_ = NULL;

int min(int a, int b){
	return a < b ? a : b;
}

void parse(http_norm *hnp_info){
 	//TODO: BETTER ERROR HANDLING
 	//parseCgiResponseHeader(hnp_info);
	if(parseHttpRequestHeader(hnp_info->cp_first_line) == EXIT_FAILURE){
			secAbort();
	}
//	if(parseRequiredArguments(hnp_info) == EXIT_FAILURE)
//		secAbort();
//	B_SEARCH_AUTORIZATION = TRUE;
//	http_autorization_ = secCalloc(1, sizeof(http_autorization));
//	http_autorization_->cp_nonce = NULL;
//	http_autorization_->cp_realm = NULL;
//	http_autorization_->cp_response = NULL;
//	http_autorization_->cp_uri = NULL;
//	http_autorization_->cp_username = NULL;

	if(parseArguments(hnp_info) == EXIT_FAILURE)
		secAbort();
	
	parsePrintStructures();
}

int parseCgiResponseHeader(http_norm *hnp_info){
	char* cp_content = NULL;
	char* cp_status = NULL;
	bool content_found =FALSE;
	bool status_found = FALSE;
	http_cgi_response_ = secCalloc(1, sizeof(http_cgi_response));
	http_cgi_response_->content_type = NULL;
	http_cgi_response_->status = NULL;


	debugVerbose(PARSER, "Check CGI response\n");
	
	//Find Content-Type
	ssize_t i = 0;
	for(; i < hnp_info->i_num_fields; ++i){

		if(i==0 && strlen(hnp_info->cpp_header_field_name[i])>=12)
			if(strncasecmp("Content-Type",hnp_info->cpp_header_field_name[i],12)==0){
				cp_content = hnp_info->cpp_header_field_body[i];
				strAppend(&http_cgi_response_->content_type, cp_content);
				content_found = TRUE;
			}
		if(i==1 && strlen(hnp_info->cpp_header_field_name[i])>=6)
			if(strncasecmp("Status",hnp_info->cpp_header_field_name[i],6)==0){
				cp_status = hnp_info->cpp_header_field_body[i];
				strAppend(&http_cgi_response_->status, cp_status);
				status_found = TRUE;
			}

	}
	if(i==1 && content_found == TRUE && status_found == FALSE){
		strAppend(&http_cgi_response_->status, "200 OK");
		status_found = TRUE;
	}

	debugVerbose(PARSER, "CGI Content: %s\n",http_cgi_response_->content_type);
	debugVerbose(PARSER, "CGI Status: %s\n",http_cgi_response_->status);
	
	if(status_found == TRUE && content_found == TRUE)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;

}


int parseArguments(http_norm *hnp_info){
	char* cp_name = NULL;
	// search for required Arguments
	
	// we need a HOST field!
	cp_name = parseFindExplicitHeaderField(hnp_info, "Host");
	if(cp_name == NULL)
		//TODO: ERROR handling, no host field!
		return EXIT_FAILURE;
	
	switch(e_used_method){
		case POST:
			debugVerbose(PARSER, "Post Method found, go on!\n");
			break;
		case GET:
			debugVerbose(PARSER, "GET Method found, go on!\n");
			break;
		case HEAD:
			debugVerbose(PARSER, "HEAD Method found, go on!\n");
			break;
		default:
			debugVerbose(PARSER, "This line should NEVER be reached!\n");
			secAbort();
			break;
	};
	
	//search for authorization field
	cp_name = parseFindExplicitHeaderField(hnp_info, "Authorization");
	if(cp_name != NULL){
		if(parseAuthorizationInfos(cp_name) == EXIT_FAILURE)
			//TODO: ERROR handling, invalid authorization request
			return EXIT_FAILURE;
		else
			B_AUTORIZATION_FOUND = TRUE;
	}
	
	// in case of cgi we would need to setup our envvars
	if(B_CGI_BIN_FOUND == TRUE){
		for(ssize_t i = 0; i < hnp_info->i_num_fields; ++i){
			cp_name = NULL;
			strAppend(&cp_name, SCCP_HTTP_HEADER_FIELD_MARKER);
			strAppend(&cp_name, hnp_info->cpp_header_field_name[i]);
			stringToUpperCase(cp_name);
			appendToEnvVarList(cp_name,hnp_info->cpp_header_field_body[i]);
		}
		appendToEnvVarList("GATEWAY_INTERFACE", "CGI/1.1");
		appendToEnvVarList("SERVER_SOFTWARE", "tiniweb/1.0");
		//TODO: What does it mean???? Which Content, body???
		// Just set content length in case of POST, header field must exist
		//WHERE to get???
		if(e_used_method == POST)
			appendToEnvVarList("CONTENT_LENGHT", "000");
		//TODO:Where to get???
		if(B_AUTORIZATION_FOUND == TRUE)
			appendToEnvVarList("REMOTE_USER",http_autorization_->cp_username);
		//TODO:Where to get???
		appendToEnvVarList("SCRIPT_FILENAME","0");
		//TODO:Where to get???
		appendToEnvVarList("DOCUMENT_ROOT","0");
	}
	return EXIT_SUCCESS;
}
	
/*	//
	
//		//TODO: provide own function, for req. header field searches!
//		if(strlen(cp_name)>=4)
//			if(strncmp(cp_name,"HTTP_HOST",4)==0)
//				B_HOST_FOUND = TRUE;
		
		
		if(B_SEARCH_AUTORIZATION==TRUE ){
			if(strlen(cp_name)>=17)
				if(strncmp(cp_name,"HTTP_AUTORIZATION",17)==0)
					B_AUTORIZATION_FOUND = TRUE;
			if(strlen(cp_name)>=13)
				if(strncmp(cp_name,"HTTP_USERNAME",13)==0){
					strAppend(&http_autorization_->username, hnp_info->cpp_header_field_body[i]);
					debugVerbose(PARSER, "Username found!\n");
				}
			if(strlen(cp_name)>=10)
				if(strncmp(cp_name,"HTTP_REALM",10)==0){
					strAppend(&http_autorization_->realm, hnp_info->cpp_header_field_body[i]);
					debugVerbose(PARSER, "REALM found!\n");	
				}
			if(strlen(cp_name)>=10)
				if(strncmp(cp_name,"HTTP_NONCE",10)==0){
					strAppend(&http_autorization_->nonce, hnp_info->cpp_header_field_body[i]);
					debugVerbose(PARSER, "NONCE found!\n");
				}
			if(strlen(cp_name)>=8)
				if(strncmp(cp_name,"HTTP_URI",8)==0){
					strAppend(&http_autorization_->uri,hnp_info->cpp_header_field_body[i]);
					debugVerbose(PARSER, "URI found!\n");
				}
			if(strlen(cp_name)>=13)
				if(strncmp(cp_name,"HTTP_RESPONSE",13)==0){
					strAppend(&http_autorization_->response,hnp_info->cpp_header_field_body[i]);
					debugVerbose(PARSER, "RESPONSE found!\n");
				}
		
		}
		
//		if(B_CGI_BIN_FOUND == TRUE)
			
//	}
	// set Constants
//	if(B_CGI_BIN_FOUND == TRUE){
//		appendToEnvVarList("GATEWAY_INTERFACE", "CGI/1.1");
//		appendToEnvVarList("SERVER_SOFTWARE", "tiniweb/1.0");
		//TODO: What does it mean???? Which Content, body???
		//Just set envvar in case of cgi_bin_found
		// Just set content length in case of POST, header field must exist
//		appendToEnvVarList("CONTENT_LENGHT", "0");
		//TODO:Where to get???
//		appendToEnvVarList("REMOTE_USER","0");
		//TODO:Where to get???
//		appendToEnvVarList("SCRIPT_FILENAME","0");
//		//TODO:Where to get???
//		appendToEnvVarList("DOCUMENT_ROOT","0");
//	}
//	else{
//	
//	}
//	return EXIT_SUCCESS:	
//}*/

char* parseFindExplicitHeaderField(http_norm* hnp_info, const char* ccp_what){
	ssize_t i = 0;
	for(; i < hnp_info->i_num_fields; ++i)
		if(strncasecmp(hnp_info->cpp_header_field_name[i], ccp_what, min(strlen(ccp_what), strlen(hnp_info->cpp_header_field_name[i]))) == 0)
			return hnp_info->cpp_header_field_body[i];
	return NULL;
}

int parseAuthorizationInfos(const char* ccp_authstr){
	char* cp_helper = NULL;
	//Must start with Digest
	if(strncasecmp("Digest", ccp_authstr, min(strlen(ccp_authstr), strlen("Digest"))) != 0)
		return EXIT_FAILURE;
	http_autorization_ = secCalloc(1, sizeof(http_autorization));
	cp_helper = parseSubstringByDelimStrings(ccp_authstr, "username=\"", "\"");
	if(cp_helper == NULL)
		return EXIT_FAILURE;
	http_autorization_->cp_username = cp_helper;
	cp_helper = parseSubstringByDelimStrings(ccp_authstr, "realm=\"", "\"");
	if(cp_helper == NULL)
		return EXIT_FAILURE;
	http_autorization_->cp_realm = cp_helper;
	cp_helper = parseSubstringByDelimStrings(ccp_authstr, "nonce=\"", "\"");
	if(cp_helper == NULL)
		return EXIT_FAILURE;
	http_autorization_->cp_nonce = cp_helper;
	cp_helper = parseSubstringByDelimStrings(ccp_authstr, "uri=\"", "\"");
	if(cp_helper == NULL)
		return EXIT_FAILURE;
	http_autorization_->cp_uri = cp_helper;
	cp_helper = parseSubstringByDelimStrings(ccp_authstr, "response=\"", "\"");
	if(cp_helper == NULL)
		return EXIT_FAILURE;
	http_autorization_->cp_response = cp_helper;
	return EXIT_SUCCESS;
}

char* parseSubstringByDelimStrings(const char* ccp_string, const char* ccp_stdelim, const char* ccp_endelim){
	ssize_t i_st = 0;
	ssize_t i_en = 0;
	ssize_t i_len_in = strlen(ccp_string);
	ssize_t i_len_st = strlen(ccp_stdelim);
	ssize_t i_len_en = strlen(ccp_endelim);
	char* cp_helper = NULL;
	debugVerbose(PARSER, "parserSubstringByDelimStrings: search for string delmited by %s and %s\n", ccp_stdelim, ccp_endelim);
	for(; i_st < i_len_in; ++i_st){
		cp_helper = secGetStringPart(ccp_string, i_st, i_st + i_len_st);
		if(cp_helper == NULL)
			return NULL;
		if(strncasecmp(cp_helper, ccp_stdelim, i_len_st) == 0)
			break;
		secFree(cp_helper);
	}
	secFree(cp_helper);
	if(i_st == i_len_in)
		return NULL;
	for(i_en = i_st + i_len_st; i_en < i_len_in; ++i_en){
		cp_helper = secGetStringPart(ccp_string, i_en, i_en + i_len_en - 1);
		if(cp_helper == NULL)
			return NULL;
		//fprintf(stderr, "i_en %d: %s \n", i_en, cp_helper);
		if(strncasecmp(cp_helper, ccp_endelim, i_len_st) == 0)
			break;
		secFree(cp_helper);
	}
	secFree(cp_helper);
	cp_helper = secGetStringPart(ccp_string, i_st + i_len_st, i_en - 1);
	debugVerbose(PARSER, "parserSubstringByDelimStrings: we found %s\n", cp_helper);
	return cp_helper;
}

int parseHttpRequestHeader(char* input){
	debugVerbose(PARSER, "Parse HttpRequestHeader\n");
	return parseRequestLine(input);
}

int parseRequestLine(char* input){
	ssize_t i_offset = parseRequestMethod(input, 0);
	// no suitable method? so we break here!
	if(i_offset == EXIT_FAILURE)
		return EXIT_FAILURE;
	http_request_ = secCalloc(1, sizeof(http_request));
	
	i_offset = parseRequestURI(input, i_offset);
	if( i_offset == EXIT_FAILURE){
		debugVerbose(PARSER, "Failure in request line detected, next step is to abort\n");
		return EXIT_FAILURE;
	}
	if(parseHttpVersion(input, i_offset + 1) == EXIT_FAILURE){
		debugVerbose(PARSER, "Unknown server protocol, next step is to abort\n");
		return EXIT_FAILURE;
	}
	
	switch (e_used_method){
		case POST: // TODO: provide BODY Field in this case and setup envvars here!
		case GET:
		case HEAD:
			http_request_->cp_method = secGetStringPart(SCCA_KNOWN_METHODS[e_used_method], 0, strlen(SCCA_KNOWN_METHODS[e_used_method]));
			//appendToEnvVarList("REQUEST_METHOD",SCCA_KNOWN_METHODS[e_used_method]);
			break;
		default:
			debugVerbose(PARSER, "Something went wrong, we schould never ever reach this line, shit happens\n");
			//debugVerbose(PARSER, "Unknown request Method detected, check for Response Header\n");
			//TODO: really, i don't think so?????
			return EXIT_FAILURE;
			break;
	};
	

	return EXIT_SUCCESS;
}

int parseRequestMethod(char* input, int offset){
	int i_offset_en = 0;
	int i = offset;
	char* cp_method = NULL;
	
	for(;i_offset_en < strlen(input) && input[i_offset_en] != ' '; ++i_offset_en);
	cp_method = secGetStringPart(input, 0, i_offset_en - 1);
	
	for(;i < SCI_NUM_KNOWN_METHODS; ++i)
		if(strncmp(SCCA_KNOWN_METHODS[i], cp_method, min(strlen(SCCA_KNOWN_METHODS[i]), strlen(cp_method))) == 0){
			e_used_method = i;
			return i_offset_en + 1;
		}
		return EXIT_FAILURE;
}

int parseRequestURI(char* input, int offset){
	char* cp_uri = NULL;
	char* cp_path = NULL;
	char* cp_fragment = NULL;
	char* cp_query = NULL;
	int i_offset_st = 0;
	int i_offset_en = 0;
	
	
	for(i_offset_st = offset; i_offset_st < strlen(input) && input[i_offset_st] != '/'; ++i_offset_st);
	if(i_offset_st >= strlen(input)){
		debugVerbose(PARSER, "No URI found\n");
		return EXIT_FAILURE;
	}
	
	for(i_offset_en = i_offset_st; i_offset_en < strlen(input) && input[i_offset_en] != ' '; ++i_offset_en);
	cp_uri = secGetStringPart(input, i_offset_st, i_offset_en - 1);
	
	if(strncmp(SCCP_CGI_BIN,cp_uri,min(strlen(SCCP_CGI_BIN), strlen(cp_uri)))==0){
		B_CGI_BIN_FOUND = TRUE;
		debugVerbose(PARSER, "CGI bin found\n");
	}
	
	for(i_offset_st = 0; i_offset_st < strlen(cp_uri) && cp_uri[i_offset_st] != '?'; ++i_offset_st);
	if(i_offset_st != strlen(cp_uri)){
		cp_path = secGetStringPart(cp_uri, 0, i_offset_st - 1);
		for(i_offset_en = i_offset_st; i_offset_en < strlen(cp_uri) && cp_uri[i_offset_en] != '#'; ++i_offset_en);
		cp_query = secGetStringPart(cp_uri, i_offset_st + 1, i_offset_en - 1);
		if(i_offset_en != strlen(cp_uri))
			cp_fragment = secGetStringPart(cp_uri, i_offset_en + 1, strlen(cp_uri) - 1);
	}
	else
		strAppend(&cp_path, cp_uri);

	if(cp_path){
		if(validateString(cp_path) == EXIT_FAILURE){
			debugVerbose(PARSER, "Invalid char detected in URI Path\n");
			return EXIT_FAILURE;
		}
	}
	// Fill up struct, remember it could contain NULL-pointers!
	http_request_->cp_uri = cp_uri;
	http_request_->cp_path = cp_path;
	http_request_->cp_query = cp_query;
	http_request_->cp_fragment = cp_fragment;
	return offset + strlen(cp_uri);
}

int parseHttpVersion(char* input, int offset){
	char* cp_http_version = NULL;
	
	if(offset > strlen(input))
		cp_http_version = secGetStringPart(SCCP_KNOWN_HTTPVERSION, 0, strlen(SCCP_KNOWN_HTTPVERSION));
	else
		cp_http_version = secGetStringPart(input, offset, strlen(input));
	
	if(strncmp(cp_http_version, SCCP_KNOWN_HTTPVERSION, min(strlen(SCCP_KNOWN_HTTPVERSION), strlen(cp_http_version))) != 0)
		return EXIT_FAILURE;
	appendToEnvVarList("SERVER_PROTOCOL",cp_http_version);
	return EXIT_SUCCESS;
}

int validateString(char* cp_string){
	int i = 0;
	
	for(;i<strlen(cp_string); ++i)
		if(isNonEscapedChar(cp_string, i) == TRUE)
			return EXIT_FAILURE;
		
		return EXIT_SUCCESS;
}

bool isNonEscapedChar(char* input, int i_offset){
	const char* ccp_invalids = " ;?:@&=+$,#";
	int i = 0;
	
	for(; i<strlen(ccp_invalids); ++i)
		if(input[i_offset] == ccp_invalids[i])
			return TRUE;
		if(input[i_offset]=='%'){
			if(strlen(input) - i_offset < 3)
				return TRUE;
			else if(isHexDigit(input[i_offset + 1]) == FALSE || isHexDigit(input[i_offset + 2]) == FALSE)
				return TRUE;
			else;
		}
		
		return FALSE;
}

bool isHexDigit(char input){
	if(input>0x29 && input < 0x3A)
		return TRUE;
	else if(input>0x40 && input < 0x47)
		return TRUE;
	else if(input>0x60 && input < 0x67)
		return TRUE;
	else
		return FALSE;
}

void stringToUpperCase(char* input){
	for(int i=0; i<strlen(input);i++){
		input[i] = toupper(input[i]);
		if(input[i] == '-')
			input[i] = '_';
	}	
}

void parsePrintStructures(){
	debugVerbose(PARSER, "Here we show our structs...\n");
	if(http_request_){
		debugVerbose(PARSER, "http_request_->cp_method = %s\n", http_request_->cp_method);
		debugVerbose(PARSER, "http_request_->cp_uri = %s\n", http_request_->cp_uri);
		debugVerbose(PARSER, "http_request_->cp_path = %s\n", http_request_->cp_path);
		debugVerbose(PARSER, "http_request_->cp_query = %s\n", http_request_->cp_query);
		debugVerbose(PARSER, "http_request_->cp_fragment = %s\n", http_request_->cp_fragment);
	}
	if(http_autorization_){
		debugVerbose(PARSER, "http_authorization_->cp_username = %s\n", http_autorization_->cp_username);
		debugVerbose(PARSER, "http_authorization_->cp_realm = %s\n", http_autorization_->cp_realm);
		debugVerbose(PARSER, "http_authorization_->cp_nonce = %s\n", http_autorization_->cp_nonce);
		debugVerbose(PARSER, "http_authorization_->cp_uri = %s\n", http_autorization_->cp_uri);
		debugVerbose(PARSER, "http_authorization_->cp_response = %s\n", http_autorization_->cp_response);
	}
	debugVerbose(PARSER, "...shown\n");
}

/*bool was_right = TRUE;
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
		}*/

/*	for(int i=0; i<strlen(HTTPVERSION);i++){
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
			*/
//return EXIT_FAILURE;
//}


//if(parseArguments());
	//char** outputline = NULL;	
	//MAX_HEADER_BUFSIZE = strlen(hnp_info->cp_header);
	//int offset = 0;
	//int arguments_valid = 0;
	//TODO: remove args 2,3
	
	//if(first_line_valid == EXIT_FAILURE){
		//TODO: ERROR
		//}
		
		// 	if(selectedMETHOD[0]==TRUE){
		// 		appendToEnvVarList("REQUEST_METHOD","GET");
		// 		/*if(hnp_info->cpp_header_field_body != NULL){
   // 			arguments_valid=EXIT_FAILURE;
   // 			//TODO: ERROR
   // 		}
   // 		else*/
   // 			arguments_valid = parseArguments(hnp_info,outputline);
   // 	}
   // 	else if(selectedMETHOD[1]==TRUE){
   // 		appendToEnvVarList("REQUEST_METHOD","POST");
   // 		if(hnp_info->cpp_header_field_body == NULL){
   // 			arguments_valid=EXIT_FAILURE;
   // 			//TODO: ERROR
   // 		}
   // 		else
   // 			arguments_valid = parseArguments(hnp_info,outputline);
   // 	}
   // 	else if(selectedMETHOD[2]==TRUE){
   // 		appendToEnvVarList("REQUEST_METHOD","HEAD");
   // 		if(hnp_info->cpp_header_field_body != NULL){
   // 			arguments_valid=EXIT_FAILURE;
   // 			//TODO: ERROR
   // 		}
   // 		else
   // 			arguments_valid = parseArguments(hnp_info,outputline);
   // 	}
   // 	else{
	   // 		//can't happen
	   // 	}
	   // 	
	   // 	if(arguments_valid==EXIT_FAILURE){
   // 		//TODO: ERROR
   // 	}
   // 	debugVerbose(3, "Arguments valid\n");
   
   //int parseHttpRequest(char* input, char** outputline, int offset){
   //	return offset;
   //}
   
   
   
   // 	if(input[offset]=='/'){
   // 		//If it is an cgi request we have to build our path from the uri
   // 		/*int length = strlen(input)-1;
   // 		if(strncmp("/cgi-bin/",input+offset,9)==0){
   // 			cgi_bin_found = TRUE;
   // 			debugVerbose(3, "CGI bin\n");
   // 
   // 			for(int i = offset+9; i< strlen(input)-1;i++){
   // 				if(input[i]=='/'){
   // 					save_position = i;
   // 				}
   // 				if(isWhiteSpace(input[i])==TRUE || isEOF(input[i])==TRUE || input[i]=='#' || input[i]=='?')
   // 					break;
   // 			}
   // 			strncpy(cgi_path,input+offset+9,save_position);
   // 			debugVerbose(3, "CGI path %s\n",cgi_path);
   // 		}
   // 		
   // 		//Try to find a fragment and/or the request
   // 		while(isWhiteSpace(input[offset])==FALSE && isEOF(input[offset])==FALSE){
   // 			my_char = input[offset];
   // 						
   // 			if(input[offset]=='#'){
   // 				fragment_found = TRUE;
   // 			}
   // 			else if(input[offset]=='?'){
   // 				query_found = TRUE;
   // 				fragment_found = FALSE;
   // 			}
   // 			else if(fragment_found == TRUE){
   // 				strAppend(&cp_fragment, &my_char);
   // 			}
   // 			else if(query_found == TRUE){
   // 				strAppend(&cp_request, &my_char);
   // 			}
   // 			else{
	   // 				//??
	   // 			}
	   // 			
	   // 			if(isNonEscapedChar(input[offset])==TRUE && query_found == FALSE)
	   // 				return EXIT_FAILURE;
	   // 			strAppend(&cp_uri, &my_char);
	   // 			
	   // 			offset=offsetPP(offset,1);
	   // 			
	   // 		}
	   // 		if(isEOF(input[offset]==TRUE)){
   // 			appendToEnvVarList("SERVER_PROTOCOL","HTTP/1.1");
   // 		}
   
   
   //	appendToEnvVarList("REQUEST_PATH", cp_path);
   //	appendToEnvVarList("FRAGMENT",cp_fragment);
   //	appendToEnvVarList("QUERY_STRING",cp_query);
   //	return offset;
   //}
   //	else{
	   //		return EXIT_FAILURE;
	   //	}
	   //	
	   //}
	   
	   // bool isChar(char input){
   // 	if(input < 32 || input > 126 || input!='\t')
   // 		return FALSE;
   // 	else
   // 		return TRUE;
   // }
   
   
   
   
   // /*bool isWhiteSpace(char input){
   // 	if(input==' ' || input=='\t')
   // 			return TRUE;
   // 	return FALSE;
   // }
   // 
   // bool isNewLine(char input){
   // 	if(input=='\n')
   // 		return TRUE;
   // 	return FALSE;
   // }
   // 
   // 
   // 
   // 
   // bool isEOF(char input){
   // 	if(input=='\0')
   // 		return TRUE;
   // 	return FALSE;
   // }
   // 
   // int offsetPP(int offset, int count){
   // /*	if(offset+count < MAX_HEADER_BUFSIZE){
   // 		offset= offset + count;
   // 	}
   // 	return offset;*/
   // }*/
