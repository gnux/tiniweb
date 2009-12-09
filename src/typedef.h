/** tiniweb
 * \file typedef.h
 * \author Patrick Plaschzug, Christian Partl
 */

#ifndef TYPEDEF_
#define TYPEDEF_

#ifdef EXIT_FAILURE
#undef EXIT_FAILURE
#define EXIT_FAILURE -1
#endif


// define maximal header an buffer allocation sizes
#define MAX_HEADER_SIZE 8192
#define MAX_BUFFER_ALLOCATION_SIZE (MAX_HEADER_SIZE * 2)
#define PIPE_TIMEOUT 5000
#define STDIN_TIMEOUT 5000

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint;
typedef unsigned char bool;
#define TRUE 1
#define FALSE 0
//#define NULL 0

// min and max function macro, from http://tigcc.ticalc.org/doc/gnuexts.html#SEC69
#define min(a,b) \
  ({ typeof (a) _a = (a); \
      typeof (b) _b = (b); \
    _a < _b ? _a : _b; })

#define max(a,b) \
  ({ typeof (a) _a = (a); \
      typeof (b) _b = (b); \
    _a > _b ? _a : _b; })


//int min(int a, int b);

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
	int i_num_fields;
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

