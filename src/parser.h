/** tiniweb
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
 * if something went wrong or the input is incorrect we abort
 */
void parse(http_norm *hnp_info);

/**
 * Takes the Normalized Input from an CGI-Respons, Parser for Contetn-Type and Status
 * Return EXIT_SUCESS or EXIT_FAILURE
 */
http_cgi_response* parseCgiResponseHeader(http_norm *hnp_info);
/**
 * Iterate to the hole Arguments providet from the Normalizer, add to all Headerfieldnames an HTTP_
 * an make them to upper Case by calling the stringToUpperCase funktion
 */
int parseArguments(http_norm *hnp_info);

/**
 * Calls parseRequestLine
 */
int parseHttpRequestHeader(char* input);

/**
 * Checks if the RequestLine is formated rigth and locks like:
 * request-Line = method SPACE request-uri [http-version]
 */
int parseRequestLine(char* input);


/**
 * Checks if  the Method is one out of our supported methods "GET","POST","HEAD"
 */
int parseRequestMethod(char* input, int offset);

/**
 * Checks if the URI looks like:
 * request-uri = abspath ["?" query-string] ["#" fragment]
 * normal chars are alloud everything else has to be escaped, validateString function checks for that
 */
int parseRequestURI(char* input, int offset);

/**
 * Checks if the HttpVersion is supported, if no HttpVersion is given we set the following "HTTP/1.1"
 */
int parseHttpVersion(char* input, int offset);

/**
 * Checks if the String is correct and Special characters are escaped with "%" hexdigit hexdigit
 */
int validateAbspath(char** cp_string);

/**
 * Tries to find non excaped chars, if one is found we abort
 */
bool isNonEscapedChar(char* input, int i_offset);

/**
 * Tries to check if the escaped chars hexdigit is correct
 */
bool isHexDigit(char input);

/**
 * Transforms an String to an Upper Case String
 */
void stringToUpperCase(char* input);

/**
 *
 */
char* parseFindExplicitHeaderField(http_norm* hnp_info, const char* ccp_what);

/**
 *
 */
char* parseSubstringByDelimStrings(const char* ccp_string, const char* ccp_stdelim, const char* ccp_endelim);

/**
 *
 */
int parseAuthorizationInfos(const char* ccp_authstr);

/**
 *
 */
void parsePrintStructures();

 /**
 *
 */
char* parseExtention(char* filepath);

 /**
 *
 */
char* parseFilename(char* filepath);


//void normalizeHeader(char input[]);
//bool isChar(char input);
//bool isWhiteSpace(char input);
//bool isNewLine(char input);
//bool isEOF(char input);
//int parseHttpRequest(char* input, char** outputline, int offset);
//int offsetPP(int offset, int count);

#endif /*PARSER_H_*/
