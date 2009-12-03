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
#include "httpresponse.h"
#include "secstring.h"

static const char* SCCA_KNOWN_METHODS[] = {"GET", "POST", "HEAD"};
static const int SCI_NUM_KNOWN_METHODS = 3;
static const char* SCCP_KNOWN_HTTPVERSION = "HTTP/1.1";
static const char* SCCP_CGI_BIN ="/cgi-bin/";
static const char* SCCP_HTTP_HEADER_FIELD_MARKER = "HTTP_";
enum SCE_KNOWN_METHODS e_used_method = UNKNOWN;


//TODO: really use global variable???
bool B_CGI_BIN_FOUND = FALSE;
bool B_AUTORIZATION_FOUND = FALSE;
http_autorization* http_autorization_ = NULL;

http_request *http_request_ = NULL;

int min(int a, int b){
	return a < b ? a : b;
}

void parse(http_norm *hnp_info){
 	//TODO: BETTER ERROR HANDLING
 	
 	//We have to check if the first Line is correct else we can abort
 	if(parseHttpRequestHeader(hnp_info->cp_first_line) == EXIT_FAILURE)
		secExit(STATUS_BAD_REQUEST);
			
	//Now we try to find all Arguments
	if(parseArguments(hnp_info) == EXIT_FAILURE)
		secExit(STATUS_BAD_REQUEST);
	
	//Let's see what we have found
	parsePrintStructures();
}

http_cgi_response* parseCgiResponseHeader(http_norm *hnp_info){
	http_cgi_response *http_cgi_response_ = NULL;
	ssize_t i = 1;
	ssize_t i_len = 0;
	http_cgi_response_ = secCalloc(1, sizeof(http_cgi_response));
	http_cgi_response_->content_type = NULL;
	http_cgi_response_->status = NULL;
	http_cgi_response_->server = NULL;
	http_cgi_response_->connection = NULL;
	http_cgi_response_->cpp_header_field_body = NULL;
	http_cgi_response_->cpp_header_field_name = NULL;
	http_cgi_response_->i_num_fields = 0;


	debugVerbose(PARSER, "Check CGI response\n");
	
	// it is req that content type is first field!
	// If there is no field we can abort, if the doesn't has 12chars it can't be content-type so abort
	if(hnp_info->i_num_fields < 1 || strlen(hnp_info->cpp_header_field_name[0]) != 12)
		//TODO errror!!!
		secExit(STATUS_BAD_REQUEST);
	
	//Check if first field is realy content-type set the value to our structure
	if(strncasecmp("Content-Type",hnp_info->cpp_header_field_name[0],min(strlen(hnp_info->cpp_header_field_name[0]), 12))==0){
		http_cgi_response_->content_type = hnp_info->cpp_header_field_body[0];				
	}
	else
		//TODO errror!!!
		secExit(STATUS_BAD_REQUEST);

	//second can be Status, or no status is provided 
	//if no Status is provided we have to set it to "200 OK"
	if(hnp_info->i_num_fields > 1){
		if(strlen(hnp_info->cpp_header_field_name[1]) == 6)
			if(strncasecmp("Status",hnp_info->cpp_header_field_name[1],6)==0){
				http_cgi_response_->status = hnp_info->cpp_header_field_body[1];
				i++;
			}
		if(http_cgi_response_->status == NULL)
			strAppend(&http_cgi_response_->status, "200 OK");	
	}
	
	// DEFAULTS
	strAppend(&http_cgi_response_->connection, "close");
	strAppend(&http_cgi_response_->server, "tiniweb/1.0");
	
	// add defaults
	http_cgi_response_->i_num_fields = 3;
	http_cgi_response_->cpp_header_field_name = secRealloc(http_cgi_response_->cpp_header_field_name, sizeof(char*) * http_cgi_response_->i_num_fields);
	http_cgi_response_->cpp_header_field_body = secRealloc(http_cgi_response_->cpp_header_field_body, sizeof(char*) * http_cgi_response_->i_num_fields);
	
	http_cgi_response_->cpp_header_field_name[0] = "Server";
	http_cgi_response_->cpp_header_field_body[0] = http_cgi_response_->server;
	
	http_cgi_response_->cpp_header_field_name[1] = "Connection";
	http_cgi_response_->cpp_header_field_body[1] = http_cgi_response_->connection;
	
	http_cgi_response_->cpp_header_field_name[2] = "Content-Type";
	http_cgi_response_->cpp_header_field_body[2] = http_cgi_response_->content_type;
	
	
	//Find all other fields, but if we find server or connection do nothing	
	for(; i < hnp_info->i_num_fields; ++i){

		i_len = strlen(hnp_info->cpp_header_field_name[i]);
		if(i_len == 6 && strncasecmp("Server",hnp_info->cpp_header_field_name[i],6)==0)
			continue;
		if(i_len == 10 && strncasecmp("Connection",hnp_info->cpp_header_field_name[i],10)==0)
			continue;
			
		++http_cgi_response_->i_num_fields;
		
		http_cgi_response_->cpp_header_field_name = secRealloc(http_cgi_response_->cpp_header_field_name, sizeof(char*) * http_cgi_response_->i_num_fields);
		http_cgi_response_->cpp_header_field_body = secRealloc(http_cgi_response_->cpp_header_field_body, sizeof(char*) * http_cgi_response_->i_num_fields);
		
		http_cgi_response_->cpp_header_field_name[http_cgi_response_->i_num_fields - 1] = hnp_info->cpp_header_field_name[i];
		http_cgi_response_->cpp_header_field_body[http_cgi_response_->i_num_fields - 1] = hnp_info->cpp_header_field_body[i];
	}
		


	debugVerbose(PARSER, "CGI Content: %s\n",http_cgi_response_->content_type);
	debugVerbose(PARSER, "CGI Status: %s\n",http_cgi_response_->status);
	debugVerbose(PARSER, "CGI Connection: %s\n",http_cgi_response_->connection);
	debugVerbose(PARSER, "CGI Server: %s\n",http_cgi_response_->server);
	
	//Print all found header fields and bodys
	for(i = 0; i < http_cgi_response_->i_num_fields; ++i)
		debugVerbose(PARSER, "%s: %s\n", http_cgi_response_->cpp_header_field_name[i], http_cgi_response_->cpp_header_field_body[i]);
	
	//We return our new structure, not everything has to be set watch out for NULL
	return http_cgi_response_;
	
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
			secExit(STATUS_BAD_REQUEST);
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
	

char* parseFindExplicitHeaderField(http_norm* hnp_info, const char* ccp_what){
	ssize_t i = 0;
	//iterate over all header fields and try to find the one we search and return it
	//if we can't find it return NULL
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
		
	//If it is an autorization request all this fields must be available
	//we check for them and save them in our structur, if something went wrong
	//exit with EXIT_FAILURE else EXIT_SUCCESS
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
	
	//If there is no first Line we return EXIT_FAILURE
	if(input == NULL)
		return EXIT_FAILURE;
	//Check if it contains one out of our three supported Methods
	ssize_t i_offset = parseRequestMethod(input, 0);
	// no suitable method? so we break here!
	if(i_offset == EXIT_FAILURE)
		return EXIT_FAILURE;
	http_request_ = secCalloc(1, sizeof(http_request));
	
	//Check for an correct URI
	i_offset = parseRequestURI(input, i_offset);
	if( i_offset == EXIT_FAILURE){
		debugVerbose(PARSER, "Failure in request line detected, next step is to abort\n");
		return EXIT_FAILURE;
	}
	//If there is a http-version set it must be our supported one HTTP/1.1 if not EXIT_FAILURE
	//If there isn't any we set it to HTTP/1.1 in parseHttpVersion
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
	
	//Find the end of the first string
	for(;i_offset_en < strlen(input) && input[i_offset_en] != ' '; ++i_offset_en);
	cp_method = secGetStringPart(input, 0, i_offset_en - 1);
	
	//check if our string is one off our supported methods GET, POST, HEAD
	for(;i < SCI_NUM_KNOWN_METHODS; ++i)
		if(strncmp(SCCA_KNOWN_METHODS[i], cp_method, min(strlen(SCCA_KNOWN_METHODS[i]), strlen(cp_method))) == 0){
			e_used_method = i;
			//We found a correct Version, return the offset so everyone knows were to go on
			return i_offset_en + 1;
		}
	//We couldn't find it return EXIT_FAILURE
	return EXIT_FAILURE;
}

int parseRequestURI(char* input, int offset){
	char* cp_uri = NULL;
	char* cp_path = NULL;
	char* cp_fragment = NULL;
	char* cp_query = NULL;
	int i_offset_st = 0;
	int i_offset_en = 0;
	
	//if the Uri doesn't start with an "/" it isn't correct so we can return EXIT_FAILURE
	for(i_offset_st = offset; i_offset_st < strlen(input) && input[i_offset_st] != '/'; ++i_offset_st);
	if(i_offset_st >= strlen(input)){
		debugVerbose(PARSER, "No URI found\n");
		return EXIT_FAILURE;
	}
	
	//First find the end of the uri
	for(i_offset_en = i_offset_st; i_offset_en < strlen(input) && input[i_offset_en] != ' '; ++i_offset_en);
	cp_uri = secGetStringPart(input, i_offset_st, i_offset_en - 1);
	
	//Check if the URI starts with /cgi-bin/
	if(strncmp(SCCP_CGI_BIN,cp_uri,min(strlen(SCCP_CGI_BIN), strlen(cp_uri)))==0){
		B_CGI_BIN_FOUND = TRUE;
		debugVerbose(PARSER, "CGI bin found\n");
	}
	
	//It is possible that the URI contians an question and/or an fragment
	//First find our path, it ends after an ?,#,' ', or if the line ends
	for(i_offset_st = 0; i_offset_st < strlen(cp_uri) && cp_uri[i_offset_st] != '?' && cp_uri[i_offset_st] != '#'; ++i_offset_st);
	cp_path = secGetStringPart(cp_uri, 0, i_offset_st - 1);
	
	//After we found the path we have to check if ther is more in the string
	//is the next sign an "?" or and "#" set up our query and/or fragment vars
	if(i_offset_st != strlen(cp_uri)){
		if(cp_uri[i_offset_st] == '?'){
			for(i_offset_en = i_offset_st; i_offset_en < strlen(cp_uri) && cp_uri[i_offset_en] != '#'; ++i_offset_en);
			cp_query = secGetStringPart(cp_uri, i_offset_st + 1, i_offset_en - 1);
			if(i_offset_en != strlen(cp_uri))
				cp_fragment = secGetStringPart(cp_uri, i_offset_en + 1, strlen(cp_uri) - 1);
		}
		else
			cp_fragment = secGetStringPart(cp_uri, i_offset_st + 1, strlen(cp_uri) - 1);
	}
	else if(i_offset_st == strlen(cp_uri)){
		if(strncmp(cp_path,cp_uri,strlen(cp_uri))!=0)
			strAppend(&cp_path, cp_uri);
	}
	//why we have do append something wenn we are equal, changed statment with else if aboth correct?
	else
		strAppend(&cp_path, cp_uri);
	
	//if we found an possible correct path check if the encoding is correct and decode everything
	if(cp_path){
		if(validateAbspath(&cp_path) == EXIT_FAILURE){
			debugVerbose(PARSER, "Invalid encoding detected in URI Path\n");
			return EXIT_FAILURE;
		}
	}

	// Fill up struct, remember it could contain NULL-pointers!
	http_request_->cp_uri = cp_uri;
	http_request_->cp_path = cp_path;
	http_request_->cp_query = cp_query;
	http_request_->cp_fragment = cp_fragment;
	
	//return the correct offset, so in the next stepp we can check if ther is an HTTP-Version or not
	return offset + strlen(cp_uri);
}

int parseHttpVersion(char* input, int offset){
	char* cp_http_version = NULL;
	
	//If there isn't anything to read we set the http-version to our supported one
	if(offset > strlen(input))
		cp_http_version = secGetStringPart(SCCP_KNOWN_HTTPVERSION, 0, strlen(SCCP_KNOWN_HTTPVERSION));
	else
		cp_http_version = secGetStringPart(input, offset, strlen(input));
	
	//Check if the Version is correct
	if(strncmp(cp_http_version, SCCP_KNOWN_HTTPVERSION, min(strlen(SCCP_KNOWN_HTTPVERSION), strlen(cp_http_version))) != 0)
		secExit(STATUS_HTTP_VERSION_NOT_SUPPORTED);
	appendToEnvVarList("SERVER_PROTOCOL",cp_http_version);
	return EXIT_SUCCESS;
}





















int validateAbspath(char** cpp_string){
	ssize_t i = 0;
	ssize_t i_offset = 0;
	unsigned char* cp_decoded = NULL;
	
	cp_decoded = secCalloc(strlen(*cpp_string), sizeof(char));
	
	//Check every char of our string for correctness
	for(;i<strlen(*cpp_string); ++i){
		
		if(isNonEscapedChar(*cpp_string, i) == TRUE)
			return EXIT_FAILURE;
		if((*cpp_string)[i] == '%'){
		
			cp_decoded[i - i_offset] = (unsigned char) strDecodeHexToUInt(*cpp_string, i, 2);//decode(*cpp_string, i);
			//PROOF for NULL!!! TODO: bad request
			//We don't support 00 or ff
			if(cp_decoded[i - i_offset] == 0x00 || cp_decoded[i - i_offset] == 0xff)
				secExit(STATUS_BAD_REQUEST);
			i+=2;
			i_offset +=2;
		}
		//watch out if there is an + we make an space if we find one	
		else if((*cpp_string)[i] == '+')
			cp_decoded[i-i_offset] = ' ';
		else
			cp_decoded[i-i_offset] = (*cpp_string)[i];
	}
	secFree(*cpp_string);
	*cpp_string = cp_decoded;
	return EXIT_SUCCESS;
}

bool isNonEscapedChar(char* cp_input, int i_offset){
	const char* ccp_invalids = " ;?:@&=$,#";
	int i = 0;
	//Check if the char is one our blacklist
	for(; i<strlen(ccp_invalids); ++i)
		if(cp_input[i_offset] == ccp_invalids[i])
			return TRUE;
		
		if(cp_input[i_offset]=='%'){
			if(strlen(cp_input) - i_offset < 3)
				return TRUE;
			else if(isHexDigit(cp_input[i_offset + 1]) == FALSE || isHexDigit(cp_input[i_offset + 2]) == FALSE)
				return TRUE;
			else;
		}
		
		return FALSE;
}

// bool isHexDigit(char c){
// 	//check if the char is in a correct hexdigit range
// 	if(c>0x29 && c < 0x3A)
// 		return TRUE;
// 	else if(c>0x40 && c < 0x47)
// 		return TRUE;
// 	else if(c>0x60 && c < 0x67)
// 		return TRUE;
// 	else
// 		return FALSE;
// }

// void stringToUpperCase(char* input){
// 	//just convert every char from the string to an uppercase char
// 	for(int i=0; i<strlen(input);i++){
// 		input[i] = toupper(input[i]);
// 		if(input[i] == '-')
// 			input[i] = '_';
// 	}	
// }

void parsePrintStructures(){
	//if the structure is not NULL print it out
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
//	if(http_cgi_response_){
//		debugVerbose(PARSER, "http_cgi_response_->content_type = %s\n", http_cgi_response_->content_type);
//		debugVerbose(PARSER, "http_cgi_response_->status = %s\n", http_cgi_response_->status);
//	}
	debugVerbose(PARSER, "...shown\n");
}


char* parseExtension(const char* cp_filename){

	char* cp_extension = NULL;
	const char* ccp_file_type[] = {"png","html","txt", "css", NULL};
    char* cp_content_type[] = {"image/png","text/html","text/plain", "text/css", "application/octet-stream"};
	int i_str_end = strlen(cp_filename)-1;
	int i_str_beginn = strlen(cp_filename)-1;
	int i = 0;
	//find the beginning of our extension
	for(; i_str_beginn > 0 && cp_filename[i_str_beginn] != '.'; i_str_beginn--)
	cp_extension = secGetStringPart(cp_filename, i_str_beginn, i_str_end);
	
	//check if the extension is one we support else we set it to default
	for(;i<4;i++)
		if(strlen(cp_extension)==strlen(ccp_file_type[i]))
			if(strncasecmp(cp_extension,ccp_file_type[i],strlen(ccp_file_type[i]))==0)
				return cp_content_type[i];
	
	
	
	return cp_content_type[4];
	
}

char* parseFilename(const char* cp_filename){
	
	char* cp_name = NULL;
	int i_str_end = strlen(cp_filename)-1;
	int i_str_beginn = strlen(cp_filename)-1;
	
	//find the beginning of our filname and return the found string
	for(; i_str_beginn > 0 && cp_filename[i_str_beginn] != '/'; i_str_beginn--)
	cp_name = secGetStringPart(cp_filename, i_str_beginn, i_str_end);
	
	return cp_name;
	
}

