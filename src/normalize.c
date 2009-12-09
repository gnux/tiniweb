/** normalize.c
* Implementation of the normalizer
* \file normalize.c
* \author Patrick Plaschzug, Christian Partl, Georg Neubuaer, Dieter Ladenhauf
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "secmem.h"
#include "normalize.h"
#include "debug.h"
#include "httpresponse.h"
#include "secstring.h"
#include "pipe.h"
#include "typedef.h"

http_norm *normalizeHttp(const char* ccp_header, bool b_cgiresponse)
{
	http_norm *hnp_http_info;
	char *cp_current_line = NULL;
	size_t i_line_offset = 0;
	
	if(ccp_header[0] == '\0')
	{
		//TODO: STATUS_SCRIPT_ERROR
		debugVerbose(NORMALISE, "Empty input detected\n");
		if(b_cgiresponse == FALSE)
			secExit(STATUS_BAD_REQUEST);
		else
			secExit(STATUS_BAD_REQUEST);
	}
	hnp_http_info = secCalloc(1, sizeof(http_norm));
	hnp_http_info->i_num_fields = 0;
	hnp_http_info->cpp_header_field_name = NULL;
	hnp_http_info->cpp_header_field_body = NULL;
	hnp_http_info->cp_header = NULL;
	hnp_http_info->cp_body = NULL;
	
	// Proof the very first char
	if(isCharacter(ccp_header, 0) == EXIT_FAILURE)
	{
		if(b_cgiresponse == FALSE)
		{
			debugVerbose(NORMALISE, "Invalid HTTPRequest/Respone line detected\n");
			secExit(STATUS_BAD_REQUEST);
		}
		else
		{
			//TODO: STATUS_SCRIPT_ERROR
			debugVerbose(NORMALISE, "Invalid CGI Respone detected\n");
			secExit(STATUS_BAD_REQUEST);
		}
	}
	i_line_offset = getNextLineFromString(ccp_header, &cp_current_line, i_line_offset);
	// If we handle a CGI Response we don't need a first line (request line)
	if(b_cgiresponse == TRUE)
		hnp_http_info->cp_first_line = NULL;
	else
	{
		while(1)
		{
			strAppend(&hnp_http_info->cp_first_line, cp_current_line);
			i_line_offset = getNextLineFromString(ccp_header, &cp_current_line, i_line_offset);
			if(isNewLineChars(cp_current_line, 0)==EXIT_SUCCESS)
			{
				debugVerbose(NORMALISE, "No Header Fields detected\n");
				secExit(STATUS_BAD_REQUEST);
			}
			if(isCharacter(cp_current_line, 0) == EXIT_SUCCESS)
				break;
		}
	}
	
	// now hunt for header-fields
	while(1)
	{
		if(isValidHeaderFieldStart(cp_current_line, b_cgiresponse) == EXIT_FAILURE)
		{   
			//TODO: STATUS_SCRIPT_ERROR
			debugVerbose(NORMALISE, "Invalid Header Field detected: %s\n", cp_current_line);
			secExit(STATUS_BAD_REQUEST);
		}
		++hnp_http_info->i_num_fields;
		hnp_http_info->cpp_header_field_name = secRealloc(hnp_http_info->cpp_header_field_name, sizeof(char*) * hnp_http_info->i_num_fields);
		getHeaderFieldName(&hnp_http_info->cpp_header_field_name[hnp_http_info->i_num_fields - 1], cp_current_line);
		hnp_http_info->cpp_header_field_body = secRealloc(hnp_http_info->cpp_header_field_body, sizeof(char*) * hnp_http_info->i_num_fields);
		getHeaderFieldBody(&hnp_http_info->cpp_header_field_body[hnp_http_info->i_num_fields - 1], cp_current_line);
		// eat away multirow things
		do
		{
			i_line_offset = getNextLineFromString(ccp_header, &cp_current_line, i_line_offset);
			if(isBlank(cp_current_line, 0) == EXIT_SUCCESS)
				strAppend(&hnp_http_info->cpp_header_field_body[hnp_http_info->i_num_fields - 1], cp_current_line);
			else
				break;
		}
		while(1);
		// finished
		if(isNewLineChars(cp_current_line,0) == EXIT_SUCCESS)
			break;
	}
	normalizeHeaderFields(hnp_http_info);
	printHttpNorm(hnp_http_info);
	return hnp_http_info;
}

void restoreNormalizedHeader(http_norm* hnp_http_info)
{
	size_t i;
	hnp_http_info->cp_header = NULL;
	if(hnp_http_info->cp_first_line)
	{
		strAppend(&hnp_http_info->cp_header, hnp_http_info->cp_first_line);
		strAppend(&hnp_http_info->cp_header, "\n");
	}
	
	for(i=0; i<hnp_http_info->i_num_fields; ++i)
	{
		strAppend(&hnp_http_info->cp_header, hnp_http_info->cpp_header_field_name[i]);
		strAppend(&hnp_http_info->cp_header, ": ");
		strAppend(&hnp_http_info->cp_header, hnp_http_info->cpp_header_field_body[i]);
		strAppend(&hnp_http_info->cp_header, "\n");
	}
	
	strAppend(&hnp_http_info->cp_header, "\n");
}

void normalizeHeaderFields(http_norm* hnp_http_info)
{
	normalizeSingleLine(&(hnp_http_info->cp_first_line));
	for(size_t i = 0; i < hnp_http_info->i_num_fields; ++i)
	{
		normalizeSingleLine(&hnp_http_info->cpp_header_field_name[i]);
		if(&hnp_http_info->cpp_header_field_body[i])
			normalizeSingleLine(&hnp_http_info->cpp_header_field_body[i]);
	}
}

void normalizeSingleLine(char** cpp_input)
{
	size_t i_offset = 0;
	size_t i = 0;
	bool b_flag = 0;
	
	if(*cpp_input == NULL)
		return;
	
	while(isBlankNewLineChars(*cpp_input,i) == EXIT_SUCCESS)
	{
		++i;
	}
	i_offset = i;
	for(i=i;(*cpp_input)[i] != '\0'; ++i)
	{
		if(isBlankNewLineChars(*cpp_input, i) == EXIT_SUCCESS)
		{
			b_flag = 1;
			++i_offset;
		}
		else
		{
			if(b_flag == 1)
			{
				(*cpp_input)[i-i_offset] = ' ';
				--i_offset;
				b_flag = 0;
			}
			(*cpp_input)[i-i_offset] = (*cpp_input)[i];
		}
	}
	(*cpp_input)[i-i_offset] = '\0';
	*cpp_input = secRealloc(*cpp_input, (strlen(*cpp_input) + 1) * sizeof(char));
}

void printHttpNorm(http_norm* hnp_http_info)
{
	debugVerbose(NORMALISE, "-----START HEADER STRUCT PRINTING-----\n");
	
	if(hnp_http_info->cp_first_line)
		debugVerbose(NORMALISE, "first-line: %s\n", hnp_http_info->cp_first_line);
	
	for(size_t i = 0; i < hnp_http_info->i_num_fields; ++i)
		debugVerbose(NORMALISE, "%s: %s\n", hnp_http_info->cpp_header_field_name[i], hnp_http_info->cpp_header_field_body[i]);
	
	//TODO: remove cp_header
	if(hnp_http_info->cp_header)
		debugVerbose(NORMALISE, "complete request/response:\n%s", hnp_http_info->cp_header);
	
	debugVerbose(NORMALISE, "-----END HEADER STRUCT PRINTING-----\n");
}

void getHeaderFieldName(char** cpp_output, const char* ccp_input)
{
	size_t i_last_char_name;
	// There must be an blank or an ':'
	for(i_last_char_name = 0; isBlank(ccp_input, i_last_char_name) == EXIT_FAILURE && ccp_input[i_last_char_name] != ':'; ++i_last_char_name)
		*cpp_output = secCalloc(i_last_char_name + 2, sizeof(char));
	
	strncpy(*cpp_output, ccp_input, i_last_char_name);
	(*cpp_output)[i_last_char_name] = '\0';  
}

void getHeaderFieldBody(char** cpp_output, const char* ccp_input)
{
	size_t i_offset_token;
	size_t i_len_output;
	// search for ':' token, there must be one, we've already checked that
	for(i_offset_token = 0; ccp_input[i_offset_token] != ':'; ++i_offset_token);
	i_len_output = strlen(ccp_input) - i_offset_token;
	*cpp_output = secCalloc(i_len_output, sizeof(char));
	strncpy(*cpp_output, &ccp_input[i_offset_token + 1], i_len_output);
	(*cpp_output)[i_len_output - 1] = '\0';
}

int isBlank(const char* ccp_input, const size_t i_offset)
{
	return ((ccp_input[i_offset] == ' ' || ccp_input[i_offset] == '\t') ? EXIT_SUCCESS : EXIT_FAILURE);
}

int isNewLineChars(const char* ccp_input, const size_t i_offset)
{
	return (ccp_input[i_offset] == '\n' ? EXIT_SUCCESS : EXIT_FAILURE);
}

int isBlankNewLineChars(const char* ccp_input, const size_t i_offset)
{
	return ((isBlank(ccp_input, i_offset) == EXIT_SUCCESS) ? EXIT_SUCCESS : isNewLineChars(ccp_input, i_offset));
}

int isCharacter(const char* ccp_input, const size_t i_offset)
{
	return (ccp_input[i_offset] > 32 && ccp_input[i_offset] < 127) ? EXIT_SUCCESS : EXIT_FAILURE;
}

int isValid(const char* ccp_input, const size_t i_offset)
{
	return ((isCharacter(ccp_input, i_offset) == EXIT_SUCCESS) ? EXIT_SUCCESS : isBlankNewLineChars(ccp_input, i_offset));
}


// TODO: allow NL before : @ cgi!, just remove flag things
int isValidHeaderFieldStart(const char* ccp_input, bool b_cgiresponse)
{
	size_t i_offset_token;
	size_t i_last_char_name;
	size_t i;
	// search for ':' token
	for(i_offset_token = 0; i_offset_token < strlen(ccp_input); ++i_offset_token)
		if(ccp_input[i_offset_token] == ':')
			break;
		
		// no name given
		if(i_offset_token == strlen(ccp_input) || i_offset_token == 0)
			return EXIT_FAILURE;
		
		// search for last char in name
		for(i_last_char_name = 0; i_last_char_name < i_offset_token; ++i_last_char_name)
			if(isBlank(ccp_input, i_last_char_name) == EXIT_SUCCESS)
			{
				if(b_cgiresponse == FALSE)
					break; 
				else
					return EXIT_FAILURE;
			}
			//do this not in case of CGI
			// now proof if entries from i_last_char_name to i_offset_token is filled only by spaces
			for(i = i_last_char_name; i < i_offset_token; ++i)
				if(isBlank(ccp_input, i) != EXIT_SUCCESS)
					//		  if(b_skipfirstline == TRUE)
					return EXIT_FAILURE;
				return EXIT_SUCCESS;
}

