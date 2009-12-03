/** tiniweb
 * @file httpresponse.c
 * @author Christian Partl
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include "httpresponse.h"
#include "typedef.h"
#include "debug.h"
#include "normalize.h"
#include "cgi.h"
#include "secstring.h"

extern int si_cgi_timeout_;

int writeToOutputStream(int i_fd, const char* ccp_text)
{
    struct pollfd poll_fd[1];
    int i_text_length = 0;
    int i_poll_result = 0;
    int i_num_polls = 1;
    ssize_t written_bytes = 0;
    ssize_t total_written_bytes = 0;
    
    if(ccp_text == NULL)
    {
        return EXIT_FAILURE;
    }
    
    if (setNonblocking(i_fd))
    {
        debug(HTTP_RESPONSE, "Setting output stream non-blocking failed.\n");
        return EXIT_FAILURE;
    }
    
    i_text_length = strlen(ccp_text);
    
    poll_fd[0].fd = i_fd;
    poll_fd[0].events = POLLOUT;
    poll_fd[0].revents = 0;
    
    while(total_written_bytes < i_text_length)
    {
        i_poll_result = poll(poll_fd, i_num_polls, si_cgi_timeout_);
        
        if(i_poll_result < 0)
        {
            debug(HTTP_RESPONSE, "Polling on output stream failed.");
            return EXIT_FAILURE;
        }
        
        if(i_poll_result == 0)
        {
            debug(HTTP_RESPONSE, "Output stream timed out.");
            return EXIT_FAILURE;
        }
        
        if((poll_fd[0].revents & (POLLERR)) || ((!poll_fd[0].revents) & (POLLHUP)))
        {
            debug(HTTP_RESPONSE, "A problem occurred on the output stream.");
            return EXIT_FAILURE;
        }
        
        written_bytes = write(poll_fd[0].fd, ccp_text + (total_written_bytes), i_text_length - total_written_bytes);
        total_written_bytes += written_bytes;
        if (written_bytes < 0) 
        {       
            if(errno != EAGAIN)
            {
                debug(HTTP_RESPONSE, "Error while writing on output stream.\n");
                return EXIT_FAILURE;
            }

        }
    }    
    
    return EXIT_SUCCESS;
}

int sendCGIHTTPResponseHeader(http_cgi_response *header)
{
    int i_index = 0;
    int i_success = 0;
    char* cp_cgi_http_response_header = NULL;
    

    if(header == NULL)
        return EXIT_FAILURE;
        
    strAppend(&cp_cgi_http_response_header, "HTTP/1.1 ");
    strAppend(&cp_cgi_http_response_header, header->status);
    strAppend(&cp_cgi_http_response_header, "\n");
    
    for(i_index = 0; i_index < header->i_num_fields; i_index++)
    {
        strAppend(&cp_cgi_http_response_header, header->cpp_header_field_name[i_index]);
        strAppend(&cp_cgi_http_response_header, ": ");
        strAppend(&cp_cgi_http_response_header, header->cpp_header_field_body[i_index]);
        strAppend(&cp_cgi_http_response_header, "\n");
    }
    
    strAppend(&cp_cgi_http_response_header, "\n");
    
    i_success = writeToOutputStream(STDOUT_FILENO, cp_cgi_http_response_header);
    
    /*    
    fprintf(stdout, "HTTP/1.1 %s\n", header->status);
    
    for(i_index = 0; i_index < header->i_num_fields; i_index++)
    {
        fprintf(stdout, "%s: %s\n", header->cpp_header_field_name[i_index], 
                header->cpp_header_field_body[i_index]);
    }
    
    fprintf(stdout, "\n");
    */
    return i_success;
}

int sendHTTPResponseHeaderExplicit(const char* ccp_status, const char* ccp_content_type, int i_content_length)
{
    int i_success = 0;
    char* cp_http_response_header = NULL;

    if(ccp_content_type == NULL || ccp_status == NULL)
        return EXIT_FAILURE;
        
    cp_http_response_header = secPrint2String("HTTP/1.1 %s\n", ccp_status);
    strAppend(&cp_http_response_header, "Server: tiniweb/1.0\n");
    strAppend(&cp_http_response_header, "Connection: close\n");
    strAppendFormatString(&cp_http_response_header, "Content-Type: %s\n", ccp_content_type);
    
    if(i_content_length >= 0)
    {
        strAppendFormatString(&cp_http_response_header, "Content-Length: %i\n", i_content_length);
    }
    
    strAppend(&cp_http_response_header, "\n");
    
    /*
    fprintf(stdout, "HTTP/1.1 %s\n", ccp_status);
    fprintf(stdout, "Server: tiniweb/1.0\n");
    fprintf(stdout, "Connection: close\n");
    fprintf(stdout, "Content-Type: %s\n", ccp_content_type);
    
    if(i_content_length >= 0)
    {
        fprintf(stdout, "Content-Length: %i\n", i_content_length);
    }
    */
    i_success = writeToOutputStream(STDOUT_FILENO, cp_http_response_header);
        
    return i_success;
}

int sendHTTPAuthorizationResponse(const char* ccp_realm, const char* ccp_nonce)
{
    int i_success = 0;
    char* cp_http_auth_response = NULL;
    char* cp_body = "<html><body>Access Denied!</body></html>";

    if(ccp_realm == NULL || ccp_nonce == NULL)
        return EXIT_FAILURE;
        
    cp_http_auth_response = secPrint2String("HTTP/1.1 %s\n", getStatusCode(STATUS_UNAUTHORIZED));
    strAppend(&cp_http_auth_response, "Server: tiniweb/1.0\n");
    strAppend(&cp_http_auth_response, "Connection: close\n");
    strAppendFormatString(&cp_http_auth_response, 
                          "WWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\"\n", ccp_realm, ccp_nonce);
    strAppendFormatString(&cp_http_auth_response, "Content-Type: %s\n", getContentType(TEXT_HTML));
    strAppendFormatString(&cp_http_auth_response, "Content-Length: %d\n\n", strlen(cp_body));
    strAppendFormatString(&cp_http_auth_response, "%s", cp_body);
        
    /*    
    fprintf(stdout, "HTTP/1.1 %s\n", getStatusCode(STATUS_UNAUTHORIZED));
    fprintf(stdout, "Server: tiniweb/1.0\n");
    fprintf(stdout, "Connection: close\n");
    fprintf(stdout, "WWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\"\n", ccp_realm, ccp_nonce);
    fprintf(stdout, "Content-Type: %s\n", getContentType(TEXT_HTML));
    fprintf(stdout, "Content-Length: %i\n\n", strlen(cp_body));
    
    fprintf(stdout, "%s", cp_body);
    */
    
    i_success = writeToOutputStream(STDOUT_FILENO, cp_http_auth_response);
        
    return i_success;
}

int sendHTTPResponseHeader(int i_status, int i_content_type, int i_content_length)
{     
    int i_success = 0;
    char* cp_http_response_header = NULL;
    
    cp_http_response_header = secPrint2String("HTTP/1.1 %s\n", getStatusCode(i_status));
    strAppend(&cp_http_response_header, "Server: tiniweb/1.0\n");
    strAppend(&cp_http_response_header, "Connection: close\n");
    strAppendFormatString(&cp_http_response_header, "Content-Type: %s\n", getContentType(i_content_type));
    
    if(i_content_length >= 0)
    {
        strAppendFormatString(&cp_http_response_header, "Content-Length: %i\n", i_content_length);
    }
    
    strAppend(&cp_http_response_header, "\n");
    
    i_success = writeToOutputStream(STDOUT_FILENO, cp_http_response_header);
        
    return i_success;
    
    /*
    fprintf(stdout, "HTTP/1.1 %s\n", getStatusCode(i_status));
    fprintf(stdout, "Server: tiniweb/1.0\n");
    fprintf(stdout, "Connection: close\n");
    fprintf(stdout, "Content-Type: %s\n", getContentType(i_content_type));
   
    if(i_content_length >= 0)
    {
        fprintf(stdout, "Content-Length: %i\n", i_content_length);
    }
    
    fprintf(stdout, "\n");
    */
}

int sendHTTPResponse(int i_status, int i_content_type, const char* ccp_body)
{
    int i_content_length = 0;
    int i_success = 0;
    
    if(ccp_body == NULL)
    {   
    
        i_content_length = strlen(ccp_body);
    }
       
    i_success = sendHTTPResponseHeader(i_status, i_content_type, i_content_length);
    
    if(i_success == EXIT_FAILURE)
        return EXIT_FAILURE;
    
    i_success = writeToOutputStream(STDOUT_FILENO, ccp_body);
        
    return i_success;
}



int sendHTTPErrorMessage(int i_status)
{
    char* cp_body = NULL;
    int i_success = 0;
    
    if(i_status > STATUS_OK && i_status <= STATUS_HTTP_VERSION_NOT_SUPPORTED)
    {
        strAppend(&cp_body, "<html><body>");
        strAppend(&cp_body, getStatusCode(i_status));
        strAppend(&cp_body, "</body></html>");
        
        i_success = sendHTTPResponse(i_status, TEXT_HTML, cp_body);
    
        return i_success;
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
        case STATUS_LOGIN_FAILED:
            return "450 Login Failed";
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


