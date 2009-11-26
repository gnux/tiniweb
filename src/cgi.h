/** tiniweb
 * \file cgi.h
 * \author Christian Partl, Dieter Ladenhauf
 */

#ifndef __CGI_H__
#define __CGI_H__

#include <sys/types.h>

void processCGIScript(const char* cp_path);

int processCGIIO(int i_cgi_response_pipe, int i_cgi_post_body_pipe, pid_t pid_child);

int drainPipeTo(int source_fd, int dest_fd);

ssize_t drainPipe(int i_source_fd, char** i_destination_fd);

#endif
