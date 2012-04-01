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
 * typedef.h
 * Common definitions used globaly in whole program
 * 
 * \file typedef.h
 * \author Patrick Plaschzug, Christian Partl, Georg Neubauer, Dieter Ladenhauf
 */

#ifndef TYPEDEF_
#define TYPEDEF_

#include <stdlib.h>

// define EXIT_FAILURE and EXIT_SUCCESS
#ifdef EXIT_FAILURE
#undef EXIT_FAILURE
#endif
#define EXIT_FAILURE -1

#ifdef EXIT_SUCCESS
#undef EXIT_SUCCESS
#endif
#define EXIT_SUCCESS 0

// define maximal header an buffer allocation sizes
#define MAX_HEADER_SIZE 8192
#define MAX_BUFFER_ALLOCATION_SIZE (MAX_HEADER_SIZE * 2)
// define timeouts
#define PIPE_TIMEOUT 5000
#define STDIN_TIMEOUT 5000
#define CGI_TIME_OUT_MIN 1000
#define CGI_TIME_OUT_MAX 50000

// define some std variable types
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint;
typedef unsigned char bool;

// define true and false
#define TRUE 1
#define FALSE 0

// min and max function macro, from http://tigcc.ticalc.org/doc/gnuexts.html#SEC69
#define min(a,b) \
({ typeof (a) _a = (a); \
typeof (b) _b = (b); \
_a < _b ? _a : _b; })

#define max(a,b) \
({ typeof (a) _a = (a); \
typeof (b) _b = (b); \
_a > _b ? _a : _b; })


/**
* Structure for normalised and splittet header
*/
typedef struct http_norm {
	char *cp_first_line; /**< first line of the header (just in case of request) */
	ssize_t i_num_fields; /**< number of available header fields */
	char **cpp_header_field_name; /**< names of the header fields */
	char **cpp_header_field_body; /**< header field bodys */
} http_norm;

/**
* Structure for parsed authorization information
*/
typedef struct http_autorization{
	char *cp_username; /**< username */
	char *cp_realm; /**< realm */
	char *cp_nonce; /**< nonce */
	char *cp_uri; /**< uri */
	char *cp_response; /**< response */
}http_autorization;

/**
* Structure for parsed cgi response
*/
typedef struct http_cgi_response{
	int i_num_fields; /**< number of available header fields */
	char *content_type; /**< response content type */
	char *status; /**< response status */
	char *connection; /**< response connection information */
	char *server; /**< which server */
	char **cpp_header_field_name; /**< names of other header fields */
	char **cpp_header_field_body; /**< header field bodys */
}http_cgi_response;

/**
* Structure for parsed cgi response
*/
typedef struct http_request{
	char *cp_method; /**< used request method */
	char *cp_uri; /**< requested uri */
	char *cp_query; /**< query string from uri */
	char *cp_fragment; /**< fragment from uri */
	char *cp_path; /**< requested file path */
}http_request;

#endif /*TYPEDEF_*/
