/** tiniweb
 * @file httpresponse.h
 * @author Christian Partl
 */

#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include <sys/types.h>
#include "typedef.h"

int sendCGIHTTPResponseHeader(http_cgi_response *header);

int sendHTTPResponseHeader(const char* cp_status, const char* cp_content_type);

#endif

