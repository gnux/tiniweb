/** 
 * Copyright 2009-2012
 * 
 * Dieter Ladenhauf
 * Georg Neubauer
 * Christian Partl
 * Patrick Plaschzug
 * 
 * This file is part of Wunderwuzzi.
 * 
 * Wunderwuzzi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wunderwuzzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wunderwuzzi. If not, see <http://www.gnu.org/licenses/>.
 * 
 * -------------------------------------------------------------------
 * 
 * tiniweb
 * 
 * \file parser.c
 * \author Patrick Plaschzug, Georg Neubauer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "parser.h"
#include "secmem.h"
#include "normalize.h"
#include "debug.h"
#include "envvar.h"
#include "httpresponse.h"
#include "secstring.h"
#include "typedef.h"

static const char* SCCA_KNOWN_METHODS[] = {"GET", "POST", "HEAD"};
static const int SCI_NUM_KNOWN_METHODS = 3;
static const char* SCCP_KNOWN_HTTPVERSION = "HTTP/1.1";

enum SCE_KNOWN_METHODS e_used_method = UNKNOWN;

bool B_AUTORIZATION_FOUND = FALSE;
http_autorization* http_autorization_ = NULL;
http_request *http_request_ = NULL;


void parse(http_norm *hnp_info){
 	
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
		secExit(STATUS_BAD_REQUEST);
	
	//Check if first field is realy content-type set the value to our structure
	if(strncasecmp("Content-Type",hnp_info->cpp_header_field_name[0],min(strlen(hnp_info->cpp_header_field_name[0]), 12))==0){
		http_cgi_response_->content_type = hnp_info->cpp_header_field_body[0];				
	}
	else
		secExit(STATUS_BAD_REQUEST);

	//second can be Status, or no status is provided 
	//if no Status is provided we have to set it to "200 OK"
	if(hnp_info->i_num_fields > 1){
		if(strlen(hnp_info->cpp_header_field_name[1]) == 6)
			if(strncasecmp("Status",hnp_info->cpp_header_field_name[1],6)==0){
				http_cgi_response_->status = hnp_info->cpp_header_field_body[1];
				if(isStatusCode(http_cgi_response_->status)==FALSE)
					secExit(STATUS_INTERNAL_SERVER_ERROR);
				i++;
			}			
	}
	
	if(http_cgi_response_->status == NULL)
		strAppend(&http_cgi_response_->status, "200 OK");
	
	// DEFAULTS
	strAppend(&http_cgi_response_->connection, "close");
	strAppend(&http_cgi_response_->server, "tiniweb/1.0");
	
	// add defaults
	http_cgi_response_->i_num_fields = 4;
	http_cgi_response_->cpp_header_field_name = secRealloc(http_cgi_response_->cpp_header_field_name, 
                                                           sizeof(char*) * http_cgi_response_->i_num_fields);
	http_cgi_response_->cpp_header_field_body = secRealloc(http_cgi_response_->cpp_header_field_body, 
                                                           sizeof(char*) * http_cgi_response_->i_num_fields);
	
	http_cgi_response_->cpp_header_field_name[0] = secPrint2String("Server");
	http_cgi_response_->cpp_header_field_body[0] = http_cgi_response_->server;
	
	http_cgi_response_->cpp_header_field_name[1] = secPrint2String("Connection");
	http_cgi_response_->cpp_header_field_body[1] = http_cgi_response_->connection;
	
	http_cgi_response_->cpp_header_field_name[2] = secPrint2String("Content-Type");
	http_cgi_response_->cpp_header_field_body[2] = http_cgi_response_->content_type;
	
	http_cgi_response_->cpp_header_field_name[3] = secPrint2String("Content-Lenght");
	http_cgi_response_->cpp_header_field_body[3] = secPrint2String("0");
	
	
	//Find all other fields, but if we find server or connection do nothing
	//If there are more content-type or status fields secExit
	for(; i < hnp_info->i_num_fields; ++i){

		i_len = strlen(hnp_info->cpp_header_field_name[i]);
		if(i_len == 12 && strncasecmp("Content-Type",hnp_info->cpp_header_field_name[i],12)==0)
			secExit(STATUS_INTERNAL_SERVER_ERROR);
		if(i_len == 6 && strncasecmp("Status",hnp_info->cpp_header_field_name[i],6)==0)
			secExit(STATUS_INTERNAL_SERVER_ERROR);
		if(i_len == 6 && strncasecmp("Server",hnp_info->cpp_header_field_name[i],6)==0)
			continue;
		if(i_len == 10 && strncasecmp("Connection",hnp_info->cpp_header_field_name[i],10)==0)
			continue;
		if(i_len == 14 && strncasecmp("Content-Lenght",hnp_info->cpp_header_field_name[i],14)==0)
			continue;
			
		++http_cgi_response_->i_num_fields;
		
		http_cgi_response_->cpp_header_field_name = secRealloc(http_cgi_response_->cpp_header_field_name, 
                                                               sizeof(char*) * http_cgi_response_->i_num_fields);
		http_cgi_response_->cpp_header_field_body = secRealloc(http_cgi_response_->cpp_header_field_body, 
                                                               sizeof(char*) * http_cgi_response_->i_num_fields);
		
		http_cgi_response_->cpp_header_field_name[http_cgi_response_->i_num_fields - 1] = hnp_info->cpp_header_field_name[i];
		http_cgi_response_->cpp_header_field_body[http_cgi_response_->i_num_fields - 1] = hnp_info->cpp_header_field_body[i];
	}

	//Create Debug output
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
		return EXIT_FAILURE;
	//Check Request Method if we don't now the Method we secExit
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
	
	//search for authorization field, if we find one, we now this could
	//be an Authorization Request, so let us check if everything is here
	//we call parseAuthorizationInfos
	cp_name = parseFindExplicitHeaderField(hnp_info, "Authorization");
	if(cp_name != NULL){
		if(parseAuthorizationInfos(cp_name) == EXIT_FAILURE)
			return EXIT_FAILURE;
		else
			B_AUTORIZATION_FOUND = TRUE;
	}
	
	return EXIT_SUCCESS;
}
	

char* parseFindExplicitHeaderField(http_norm* hnp_info, const char* ccp_what){
	ssize_t i = 0;
	//iterate over all header fields and try to find the one we search and return it
	//if we can't find it return NULL
	for(; i < hnp_info->i_num_fields; ++i){
        if(strlen(ccp_what)==strlen(hnp_info->cpp_header_field_name[i]))
            if(strncasecmp(hnp_info->cpp_header_field_name[i], ccp_what, min(strlen(ccp_what), strlen(hnp_info->cpp_header_field_name[i]))) == 0)
                return hnp_info->cpp_header_field_body[i];
    }
	return NULL;
}

int parseAuthorizationInfos(const char* ccp_authstr){
	char* cp_helper = NULL;
	//Must start with Digest
	if(strncasecmp("Digest ", ccp_authstr, min(strlen(ccp_authstr), strlen("Digest "))) != 0)
		return EXIT_FAILURE;
		
	//If it is an autorization request all this fields must be available
	//we check for them and save them in our structur, if something went wrong
	//exit with EXIT_FAILURE else EXIT_SUCCESS
	http_autorization_ = secCalloc(1, sizeof(http_autorization));
	//Check for username
	cp_helper = parseSubstringByDelimStrings(ccp_authstr, "username=\"", "\"");
	if(cp_helper == NULL){
		cp_helper = parseSubstringByDelimStrings(ccp_authstr, "username=", ",");
        if(cp_helper == NULL){
            cp_helper = parseSubstringByDelimStrings(ccp_authstr, "username=", "\n");
            if(cp_helper == NULL)
                return EXIT_FAILURE;
        }
    }
	http_autorization_->cp_username = cp_helper;
	//Check for realm
	cp_helper = parseSubstringByDelimStrings(ccp_authstr, "realm=\"", "\"");
	if(cp_helper == NULL){
		cp_helper = parseSubstringByDelimStrings(ccp_authstr, "realm=", ",");
		if(cp_helper == NULL){
            cp_helper = parseSubstringByDelimStrings(ccp_authstr, "realm=", "\n");
            if(cp_helper == NULL)
                return EXIT_FAILURE;
        }
	}
	http_autorization_->cp_realm = cp_helper;
	//Check for nonce
	cp_helper = parseSubstringByDelimStrings(ccp_authstr, "nonce=\"", "\"");
	if(cp_helper == NULL){
		cp_helper = parseSubstringByDelimStrings(ccp_authstr, "nonce=", ",");
		if(cp_helper == NULL){
            cp_helper = parseSubstringByDelimStrings(ccp_authstr, "nonce=", "\n");
            if(cp_helper == NULL)
                return EXIT_FAILURE;
        }
	}
	http_autorization_->cp_nonce = cp_helper;
	//Check for uri
	cp_helper = parseSubstringByDelimStrings(ccp_authstr, "uri=\"", "\"");
	if(cp_helper == NULL){
		cp_helper = parseSubstringByDelimStrings(ccp_authstr, "uri=", ",");
		if(cp_helper == NULL){
            cp_helper = parseSubstringByDelimStrings(ccp_authstr, "uri=", "\n");
            if(cp_helper == NULL)
                return EXIT_FAILURE;
        }
	}
	http_autorization_->cp_uri = cp_helper;
	//Check for response
	cp_helper = parseSubstringByDelimStrings(ccp_authstr, "response=\"", "\"");
	if(cp_helper == NULL){
		cp_helper = parseSubstringByDelimStrings(ccp_authstr, "response=", ",");
		if(cp_helper == NULL){
            cp_helper = parseSubstringByDelimStrings(ccp_authstr, "response=", "\n");
            if(cp_helper == NULL)
                return EXIT_FAILURE;
        }
	}
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
	//Iterate to the string as long as we find the cpp_stdelim
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
	//Iterate to the string as long as we find the cpp_endelim
	for(i_en = i_st + i_len_st; i_en < i_len_in; ++i_en){
		cp_helper = secGetStringPart(ccp_string, i_en, i_en + i_len_en - 1);
		if(cp_helper == NULL)
			return NULL;
		if(strncasecmp(cp_helper, ccp_endelim, i_len_en) == 0)
			break;
		secFree(cp_helper);
	}
	secFree(cp_helper);
	//now we know the start and the end of the first apperiance of this strings/chars in our
	//big string. Just call secGetStringPart with this information
	cp_helper = secGetStringPart(ccp_string, i_st + i_len_st, i_en - 1);
	debugVerbose(PARSER, "parserSubstringByDelimStrings: we found %s\n", cp_helper);
	return cp_helper;
}

int parseHttpRequestHeader(char* input){
	//RequestHeader starts with RequestLine, rekursiv Parsing
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
	//If there is a http-version set it must be our supported one HTTP/1.1 if not
	//Bad Request if there isn't one or Wrong HTTP Version if it is not the supported one
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
	for(;i < SCI_NUM_KNOWN_METHODS; ++i){
        if(strlen(cp_method)==strlen(SCCA_KNOWN_METHODS[i])){
            if(strncmp(SCCA_KNOWN_METHODS[i], cp_method, min(strlen(SCCA_KNOWN_METHODS[i]), strlen(cp_method))) == 0){
                e_used_method = i;
                //We found a correct Version, return the offset so everyone knows were to go on
                return i_offset_en + 1;
            }
        }
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
	for(i_offset_st = offset; i_offset_st < strlen(input) && input[i_offset_st] != '/'; ++i_offset_st){
        if(isBlank(input,i_offset_st)==EXIT_FAILURE)
               return EXIT_FAILURE;
    }
	if(i_offset_st >= strlen(input)){
		debugVerbose(PARSER, "No URI found\n");
		return EXIT_FAILURE;
	}
	
	//First find the end of the uri
	for(i_offset_en = i_offset_st; i_offset_en < strlen(input) && input[i_offset_en] != ' '; ++i_offset_en);
	cp_uri = secGetStringPart(input, i_offset_st, i_offset_en - 1);
	
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
	
	//if we found an possible correct path check if the encoding is correct and decode everything
	if(cp_path){
		if(validateAbspath(&cp_path) == EXIT_FAILURE){
			debugVerbose(PARSER, "Invalid encoding detected in URI Path\n");
			return EXIT_FAILURE;
		}
	}
	
	// if we try do get an directory, we will force to get index.html
	if(cp_path[strlen(cp_path) - 1] == '/')
		strAppend(&cp_path, "index.html");
	
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
		secExit(STATUS_BAD_REQUEST);
		//cp_http_version = secGetStringPart(SCCP_KNOWN_HTTPVERSION, 0, strlen(SCCP_KNOWN_HTTPVERSION));
	else
		cp_http_version = secGetStringPart(input, offset, strlen(input));
	
	//Check if the Version is correct
    if(strlen(cp_http_version)!=strlen(SCCP_KNOWN_HTTPVERSION))
        secExit(STATUS_HTTP_VERSION_NOT_SUPPORTED);
	if(strncmp(cp_http_version, SCCP_KNOWN_HTTPVERSION, min(strlen(SCCP_KNOWN_HTTPVERSION), strlen(cp_http_version))) != 0)
		secExit(STATUS_HTTP_VERSION_NOT_SUPPORTED);
	return EXIT_SUCCESS;
}

int validateAbspath(char** cpp_string){
	ssize_t i = 0;
	ssize_t i_offset = 0;
	char* cp_decoded = NULL;
	
	cp_decoded = secCalloc(strlen(*cpp_string) + 1, sizeof(char));
	
	//Check every char of our string for correctness
	for(;i<strlen(*cpp_string); ++i){
		
		if(isNonEscapedChar(*cpp_string, i) == TRUE)
			return EXIT_FAILURE;
		if((*cpp_string)[i] == '%'){
		
			cp_decoded[i - i_offset] = (unsigned char) strDecodeHexToULong(*cpp_string, i+1, 2);
			//We don't support 00
			if(cp_decoded[i - i_offset] == 0x00 )
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
    cp_decoded[i-i_offset] = '\0';
    cp_decoded = secRealloc(cp_decoded, (strlen(cp_decoded) + 1) * sizeof(char));
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
	for(; i_str_beginn > 0 && cp_filename[i_str_beginn] != '.'; i_str_beginn--);
	cp_extension = secGetStringPart(cp_filename, i_str_beginn + 1, i_str_end);
	
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
	int i_str_begin = strlen(cp_filename)-1;
	
	//find the beginning of our filname and return the found string
	for(; i_str_begin >= 0 && cp_filename[i_str_begin] != '/'; i_str_begin--);	
	cp_name = secGetStringPart(cp_filename, i_str_begin + 1, i_str_end);
	
	return cp_name;
	
}

char* parseFilepath(const char* cp_filename){
	
	char* cp_path = NULL;
	int i_str_end = strlen(cp_filename)-1;
	int i_str_begin = 0;
	
	//find the end of the path and return the found string
	while(i_str_end > 0)
	{
	    i_str_end--;
	    if(cp_filename[i_str_end] == '/')
	        break;
	}
	cp_path = secGetStringPart(cp_filename, i_str_begin, i_str_end);
	
	return cp_path;
	
}

bool isStatusCode(const char* cp_num){
	
	int i=0;
	//If the string is short then 4 it can't be right
	if(strlen(cp_num)<4)
		return FALSE;
	
	//Check if the first 3 chars are Digits
	for(;i<3;i++){
		if(cp_num[i]<47 || cp_num[i]>57)
			return FALSE;
	}
	
	if(cp_num[3] != ' ')
		return FALSE;
	
	//First three chars were digits fourth char is an space
	return TRUE;
	
}

