/** tiniweb
 * \file cgi.h
 * \author Christian Partl, Dieter Ladenhauf
 */

#ifndef __CGI_H__
#define __CGI_H__

#include <sys/types.h>

void processCGIScript(const char* cp_path, const char* cp_http_body);

int processCGIIO(int i_cgi_response_pipe, int i_cgi_post_body_pipe, ssize_t written_bytes, 
                 bool b_provide_http_body, pid_t pid_child, const char* cp_http_body);

ssize_t provideMessageBodyToCGIScript(int i_cgi_post_body_pipe, const char* cp_http_body, 
                                  size_t i_start_index);

ssize_t drainPipe(int i_source_fd, char** i_destination_fd);

#endif
