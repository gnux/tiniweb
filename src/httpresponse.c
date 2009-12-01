/** tiniweb
 * @file httpresponse.c
 * @author Christian Partl
 */

#include <stdlib.h>
#include <stdio.h>

#include "httpresponse.h"
#include "typedef.h"

int sendCGIHTTPResponseHeader(http_cgi_response *header)
{
    int i_index = 0;

    if(header == NULL)
        return EXIT_FAILURE;
        
    fprintf(stdout, "HTTP/1.1 %s\n", header->status);
    
    for(i_index = 0; i_index < header->i_num_fields; i_index++)
    {
        fprintf(stdout, "%s: %s\n", header->cpp_header_field_name[i_index], 
                header->cpp_header_field_body[i_index]);
    }
    
    fprintf(stdout, "\n");
    return EXIT_SUCCESS;
}

int sendHTTPResponseHeader(const char* cp_status, const char* cp_content_type)
{
    if(cp_content_type == NULL || cp_status == NULL)
        return EXIT_FAILURE;
        
    fprintf(stdout, "HTTP/1.1 %s\n", cp_status);
    fprintf(stdout, "Server: tiniweb/1.0\n");
    fprintf(stdout, "Connection: close\n");
    fprintf(stdout, "Content-Type: %s\n\n", cp_content_type);
        
    return EXIT_SUCCESS;
}

