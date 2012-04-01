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
 * \file parser.h
 * \author Patrick Plaschzug, Georg Neubauer
 */
 
#ifndef PARSER_H_
#define PARSER_H_
#include "typedef.h"

static const enum SCE_KNOWN_METHODS {
	GET = 0,
	POST,
	HEAD,
	UNKNOWN
} SCE_METHOD;

/**
 * Takes the Normalized Input, and calls the functions parseHttpRequestHeader and parseArguments
 * if something went wrong or the input is incorrect we call secExit
 * 
 * @param hnp_info Struct provided by the Normalizer
 */
void parse(http_norm *hnp_info);

/**
 * Takes the Normalized Input from an CGI-Response, parse for Content-Type and Status
 * check if they only appere once, set status if you couldn't find one
 * Return EXIT_SUCESS or EXIT_FAILURE
 * 
 * @param hnp_info Struct provided by the Normalizer
 * @return http_cgi_response Struct defined in typedef.h
 */
http_cgi_response* parseCgiResponseHeader(http_norm *hnp_info);

/**
 * Iterate to the hole Arguments providet from the Normalizer, check for
 * Host, Method, Autorization
 * 
 * @param hnp_info Struct provided by the Normalizer
 * @return EXIT_FAILURE if something went wrong, EXIT_SUCCESS if everything was right
 */
int parseArguments(http_norm *hnp_info);

/**
 * Calls parseRequestLine
 * 
 * @param input a char pointer to an string you want to get checked
 * @return EXIT_FAILURE if something went wrong, EXIT_SUCCESS if everything was right
 */
int parseHttpRequestHeader(char* input);

/**
 * Checks if the RequestLine is formated rigth and locks like:
 * request-Line = method SPACE request-uri [http-version]
 * 
 * @param input a char pointer to an string you want to get checked
 * @return EXIT_FAILURE if something went wrong, EXIT_SUCCESS if everything was right
 */
int parseRequestLine(char* input);


/**
 * Checks if  the Method is one out of our supported methods "GET","POST","HEAD"
 * 
 * @param input a char pointer to an string you want to get checked
 * @param offset point where you want to start to check
 * @return EXIT_FAILURE if something went wrong, pointer to the Offset how fare we have parsed the string
 */
int parseRequestMethod(char* input, int offset);

/**
 * Checks if the URI looks like:
 * request-uri = abspath ["?" query-string] ["#" fragment]
 * normal chars are alloud everything else has to be escaped, validateString function checks for that
 * 
 * @param input a char pointer to an string you want to get checked
 * @param offset point where you want to start to check
 * @return EXIT_FAILURE if something went wrong, pointer to the Offset how fare we have parsed the string
 */
int parseRequestURI(char* input, int offset);

/**
 * Checks if the HttpVersion is supported, if no HttpVersion is given secExit
 * 
 * @param input a char pointer to an string you want to get checked
 * @param offset point where you want to start to check
 * @return EXIT_SUCCESS if we found the rigth method, or it terminates with secEXIT
 */
int parseHttpVersion(char* input, int offset);

/**
 * Checks if the String is correct and Special characters are escaped with "%" hexdigit hexdigit
 * 
 * @param cp_string double pointer prove the correctness and decode, written back to cp_string
 * @return EXIT_SUCCESS if we found the rigth method, or it terminates with secEXIT
 */
int validateAbspath(char** cp_string);

/**
 * Tries to find non excaped chars, if one is found we abort
 * 
 * @param input a char pointer to an string you want to get checked
 * @param offset point where you want to start to check
 * @return TRUE if it is an non escaped char or FALSE
 */
bool isNonEscapedChar(char* cp_input, int i_offset);

/**
 * Takes the string you are searching for and returns the header fieldbody if it could
 * be found, or NULL if not
 * 
 * @param hnp_info pointer to http_norm which should contain
 * @param ccp_what pointer to string you want to find
 * @return char pointer to the found Header Field or NULL
 */
char* parseFindExplicitHeaderField(http_norm* hnp_info, const char* ccp_what);

/**
 * Takes the beginning char and the end char and returns everything between them, watch out
 * it only finds the first time where it occures
 * 
 * @param ccp_string const char pointer to string
 * @param ccp_stdelim const char pointer to beginning char
 * @param ccp_endelim const char pointer to end char
 * @return char pointer to the found String or NULL
 */
char* parseSubstringByDelimStrings(const char* ccp_string, const char* ccp_stdelim, const char* ccp_endelim);

/**
 * If we find an Autorization field with our function parseFindExplicitHeaderField, we start
 * to look for all necessary fields we need to check if the autorization could be correct
 * 
 * @param ccp_authstr const char pointer to the Authorization String
 * @return EXIT_SUCCESS if all Fields have been found or EXIT_FAILURE if not
 */
int parseAuthorizationInfos(const char* ccp_authstr);

/**
 * Prints out the found Structures so we can see if everything went right
 */
void parsePrintStructures();

 /**
 * Parse the last characters until it finds the first "." afterwoods it checks if we know
 * the extension and returns the defined values
 * 
 * @param cp_filename const char pointer to the Filepath
 * @return char* which points to the extension
 */
char* parseExtension(const char* cp_filename);

 /**
 * Parse the last characters until it finds the first "/" returns everything after "/" to 
 * end of File
 * 
 * @param cp_filename const char pointer to the Filepath
 * @return char* which points to the filepath
 */
char* parseFilename(const char* cp_filename);

 /**
 * Parse the last characters until it finds the first "/" returns everything before "/" to 
 * end of File
 * 
 * @param cp_filename const char pointer to the Filepath
 * @return char* which points to the filepath
 */
char* parseFilepath(const char* cp_filename);


 /**
 * Check if the first three Chars are be Digits followed with a space
 * 
 * @param cp_num const char pointer to the String 
 * @return TRUE if it is a Code with 3 Digits or FALSE
 */
bool isStatusCode(const char* cp_num);

#endif /*PARSER_H_*/
