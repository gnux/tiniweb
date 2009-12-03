/** tiniweb
 * @file httpresponse.h
 * @author Christian Partl
 */

#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include <sys/types.h>
#include "typedef.h"

static const enum SCE_STATUS_CODE_
{
    STATUS_OK,
    STATUS_BAD_REQUEST,
    STATUS_UNAUTHORIZED,
    STATUS_FORBIDDEN,
    STATUS_NOT_FOUND,
    STATUS_INTERNAL_SERVER_ERROR,
    STATUS_HTTP_VERSION_NOT_SUPPORTED,
    STATUS_LOGIN_FAILED
    
} SCE_STATUS_CODE;

static const enum SCE_CONTENT_TYPE_
{
    TEXT_HTML,
    TEXT_PLAIN,
    TEXT_CSS,
    IMAGE_PNG,
    DEFAULT
    
} SCE_CONTENT_TYPE;

int writeToOutputStream(int i_fd, const char* ccp_text);

int sendHTTPErrorMessage(int i_status);

int sendCGIHTTPResponseHeader(http_cgi_response *header);

int sendHTTPAuthorizationResponse(const char* ccp_realm, const char* ccp_nonce);

int sendHTTPResponseHeader(int i_status, int i_content_type, int i_content_length);

int sendHTTPResponse(int i_status, int i_content_type, const char* ccp_body);

int sendHTTPResponseHeaderExplicit(const char* cp_status, const char* cp_content_type, int i_content_length);

char* getStatusCode(int i_status);

char* getContentType(int i_content_type);

#endif

