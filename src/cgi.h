/** tiniweb
 * \file cgi.h
 * \author Christian Partl, Dieter Ladenhauf
 */

#ifndef __CGI_H__
#define __CGI_H__

#include <sys/types.h>

void processCGIScript(const char* cp_path);

int processCGIIO(int i_cgi_response_pipe, int i_cgi_post_body_pipe, pid_t pid_child);

int provideBodyToCGIScript(int source_fd, int dest_fd);

int provideCGIBodyToHTTPClient(int i_source_fd, int i_dest_fd);

int provideResponseStreamToHttpClient(FILE *stream, int i_dest_fd);

FILE* getCGIHeaderResponseStream(int i_source_fd);

#endif
