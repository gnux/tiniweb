/** tiniweb
 * \file typedef.h
 * \author Patrick Plaschzug
 */

#ifndef TYPEDEF_
#define TYPEDEF_

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint;
typedef unsigned char bool;
#define TRUE 1
#define FALSE 0
//#define NULL 0

typedef struct http_norm {
  char *cp_first_line;
  int i_num_fields;
  char **cpp_header_field_name;
  char **cpp_header_field_body;
  char *cp_header;
  char *cp_body;
} http_norm;


#endif /*TYPEDEF_*/
