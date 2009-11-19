/** tiniweb
 * \file cgi.h
 * \author Christian Partl, Dieter Ladenhauf
 */

#ifndef __CGI_H__
#define __CGI_H__

#include <sys/types.h>

void processCGIScript(const char* cp_path, const char* cp_http_body);

int readFromCGIScript(int i_cgi_response_pipe, pid_t pid_child);

int provideMessageBodyToCGIScript(int i_cgi_post_body_pipe, const char* cp_http_body);

int drainPipe(int i_source_fd, char** i_destination_fd);

#endif
