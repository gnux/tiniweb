/** tiniweb
 * \file typedef.h
 * \author Patrick Plaschzug
 */

#ifndef TYPEDEF_
#define TYPEDEF_

#ifdef EXIT_FAILURE
#undef EXIT_FAILURE
#endif
#define EXIT_FAILURE -1

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint;
typedef unsigned char bool;
#define TRUE 1
#define FALSE 0
//#define NULL 0

int min(int a, int b);

typedef struct http_norm {
  char *cp_first_line;
  int i_num_fields;
  char **cpp_header_field_name;
  char **cpp_header_field_body;
  char *cp_header;
  char *cp_body;
} http_norm;

typedef struct http_autorization{
	char *cp_username;
	char *cp_realm;
	char *cp_nonce;
	char *cp_uri;
	char *cp_response;
}http_autorization;

typedef struct http_cgi_response{
	int i_num_header_fields;
	char *content_type;
	char *status;
	char *connection;
	char *server;
	char **cpp_header_field_name;
  	char **cpp_header_field_body;
	
}http_cgi_response;

typedef struct http_request{
	char *cp_method;
	char *cp_uri;
	char *cp_query;
	char *cp_fragment;
	char *cp_path;
}http_request;

#endif /*TYPEDEF_*/
