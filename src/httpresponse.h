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
    STATUS_OK = 0,
    STATUS_CANCEL,
    STATUS_BAD_REQUEST,
    STATUS_UNAUTHORIZED,
    STATUS_FORBIDDEN,
    STATUS_NOT_FOUND,
    STATUS_INTERNAL_SERVER_ERROR,
    STATUS_LOGIN_FAILED,
    STATUS_HTTP_VERSION_NOT_SUPPORTED    
    
} SCE_STATUS_CODE;

static const enum SCE_CONTENT_TYPE_
{
    TEXT_HTML,
    TEXT_PLAIN,
    TEXT_CSS,
    IMAGE_PNG,
    DEFAULT
    
} SCE_CONTENT_TYPE;

/**
 * Writes a string to the specified file using the cgi timeout as timeout for writing.
 *
 * @param i_fd File descripter that shall be written to.
 * @param ccp_text String that shall be written to the file.
 * @return EXIT_SUCCESS if no problem occurred, EXIT_FAILURE otherwise.
 */
int writeStringToFile(int i_fd, const char* ccp_text);

/**
 * Sends a standard http message for the specified status code.
 *
 * @param i_status Status code (from SCE_STATUS_CODE).
 * @return EXIT_SUCCESS if no problem occurred, EXIT_FAILURE otherwise.
 */
int sendHTTPErrorMessage(int i_status);

/**
 * Sends the response header of a cgi response.
 *
 * @param http_cgi_response Structure that contains all necessary header information.
 * @return EXIT_SUCCESS if no problem occurred, EXIT_FAILURE otherwise.
 */
int sendCGIHTTPResponseHeader(http_cgi_response *header);

/**
 * Sends the an authorization response to the http client
 *
 * @param ccp_realm Realm that shall be used for authentication.
 * @param ccp_nonce Nonce that shall be used for authentication.
 * @return EXIT_SUCCESS if no problem occurred, EXIT_FAILURE otherwise.
 */
int sendHTTPAuthorizationResponse(const char* ccp_realm, const char* ccp_nonce);

/**
 * Sends a http response header to the client.
 *
 * @param i_status Status code (from SCE_STATUS_CODE).
 * @param i_content_type Content type (from SCE_CONTENT_TYPE).
 * @param i_content_lenth Content length.
 * @return EXIT_SUCCESS if no problem occurred, EXIT_FAILURE otherwise.
 */
int sendHTTPResponseHeader(int i_status, int i_content_type, int i_content_length);

/**
 * Sends a http response to the client.
 *
 * @param i_status Status code (from SCE_STATUS_CODE).
 * @param i_content_type Content type (from SCE_CONTENT_TYPE).
 * @param ccp_body Message body.
 * @return EXIT_SUCCESS if no problem occurred, EXIT_FAILURE otherwise.
 */
int sendHTTPResponse(int i_status, int i_content_type, const char* ccp_body);

/**
 * Sends a http response header to the client.
 *
 * @param cp_status Status code.
 * @param cp_content_type Content type.
 * @param i_content_lenth Content length.
 * @return EXIT_SUCCESS if no problem occurred, EXIT_FAILURE otherwise.
 */
int sendHTTPResponseHeaderExplicit(const char* cp_status, const char* cp_content_type, int i_content_length);

/**
 * Get the status code as string from its SCE_STATUS_CODE identifier.
 *
 * @param i_status Status code (from SCE_STATUS_CODE).
 * @return Text of the status code.
 */
char* getStatusCode(int i_status);

/**
 * Get the content type as string from its SCE_STATUS_CODE identifier.
 *
 * @param i_status Status code (from SCE_STATUS_CODE).
 * @return Content type as text.
 */
char* getContentType(int i_content_type);

#endif

