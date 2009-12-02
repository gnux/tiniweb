/** tiniweb
 * @file httpresponse.c
 * @author Christian Partl
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "httpresponse.h"
#include "typedef.h"
#include "normalize.h"


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

int sendHTTPResponseHeaderExplicit(const char* cp_status, const char* cp_content_type, int i_content_length)
{
    if(cp_content_type == NULL || cp_status == NULL)
        return EXIT_FAILURE;
        
    fprintf(stdout, "HTTP/1.1 %s\n", cp_status);
    fprintf(stdout, "Server: tiniweb/1.0\n");
    fprintf(stdout, "Connection: close\n");
    fprintf(stdout, "Content-Type: %s\n", cp_content_type);
    
    if(i_content_length >= 0)
    {
        fprintf(stdout, "Content-Length: %i\n", i_content_length);
    }
    
    fprintf(stdout, "\n");
        
    return EXIT_SUCCESS;
}

void sendHTTPResponseHeader(int i_status, int i_content_type, int i_content_length)
{       
    fprintf(stdout, "HTTP/1.1 %s\n", getStatusCode(i_status));
    fprintf(stdout, "Server: tiniweb/1.0\n");
    fprintf(stdout, "Connection: close\n");
    fprintf(stdout, "Content-Type: %s\n", getContentType(i_content_type));
   
    if(i_content_length >= 0)
    {
        fprintf(stdout, "Content-Length: %i\n", i_content_length);
    }
    
    fprintf(stdout, "\n");
}

void sendHTTPAuthorizationResponse(const char* realm, const char* nonce)
{   
    char *cp_body = "<html><body>Access Denied!</body></html>";
    fprintf(stdout, "HTTP/1.1 %s\n", getStatusCode(STATUS_UNAUTHORIZED));
    fprintf(stdout, "Server: tiniweb/1.0\n");
    fprintf(stdout, "Connection: close\n");
    fprintf(stdout, "Authorization: Digest realm=\"%s\", nonce=\"%s\"\n", realm, nonce);
    
    fprintf(stdout, "Content-Type: text/html\n");
    fprintf(stdout, "Content-Length: %i\n",strlen(cp_body));
    
    fprintf(stdout, "\n");
}

void sendHTTPResponse(int i_status, int i_content_type, const char* ccp_body)
{
    int i_content_length = 0;
    
    if(ccp_body == NULL)
    {   
    
        i_content_length = strlen(ccp_body);
    }
       
    sendHTTPResponseHeader(i_status, i_content_type, i_content_length);
    
    fprintf(stdout, "%s", ccp_body);
    
}



int sendHTTPErrorMessage(int i_status)
{
    char* cp_body = NULL;
    
    if(i_status > STATUS_OK && i_status <= STATUS_HTTP_VERSION_NOT_SUPPORTED)
    {
        strAppend(&cp_body, "<html><body>");
        strAppend(&cp_body, getStatusCode(i_status));
        strAppend(&cp_body, "</body></html>");
        
        sendHTTPResponse(i_status, TEXT_HTML, cp_body);
    
        return EXIT_SUCCESS;
    }
    
    return EXIT_FAILURE;
}

char* getStatusCode(int status)
{
    switch(status)
    {
        case STATUS_OK:
            return "200 OK";
            break;
        case STATUS_BAD_REQUEST:
            return "400 Bad Request";
            break;
        case STATUS_UNAUTHORIZED:
            return "401 Unauthorized";
            break;
        case STATUS_FORBIDDEN:
            return "403 Forbidden";
            break;
        case STATUS_NOT_FOUND:
            return "404 Not Found";
            break;
        case STATUS_INTERNAL_SERVER_ERROR:
            return "500 Internal Server Error";
            break;
        case STATUS_HTTP_VERSION_NOT_SUPPORTED:
            return "505 HTTP Version Not Supported";
            break;
        default:
            return "200 OK";
    };  
}

char* getContentType(int i_content_type)
{
    switch(i_content_type)
    {
        case TEXT_HTML:
            return "text/html";
            break;
        case TEXT_PLAIN:
            return "text/plain";
            break;
        case TEXT_CSS:
            return "text/css";
            break;
        case IMAGE_PNG:
            return "image/png";
            break;
        case DEFAULT:
            return "application/octet-stream";
            break;
        default:
            return "application/octet-stream";
    };  
}


