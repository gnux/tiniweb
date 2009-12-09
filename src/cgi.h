/** tiniweb
 * \file cgi.h
 * \author Christian Partl, Dieter Ladenhauf
 */

#ifndef __CGI_H__
#define __CGI_H__

#include <sys/types.h>
#include "typedef.h"

/**
 * Executes all actions relevant for processing the specified cgi script: creation of pipes, 
 * environment setup, fork+exec, communication with the cgi script ...
 *
 * @param cp_path Path (including filename) to the cgi script
 */
void processCGIScript(const char* cp_path);

/**
 * Performs communication to the cgi script in form of providing the message body of a http POST 
 * request and response handling.
 *
 * @param i_cgi_response_pipe File descriptor of the reading end of the cgi response pipe
 * @param i_cgi_post_body_pipe File descriptor of the writing end of the pipe for providing 
 *        the message body of an http POST request.
 * @param pid_child Process id of the cgi program.
 * @return EXIT_SUCCESS if no problem occurred, EXIT_FAILURE otherwise.
 */
int processCGIIO(int i_cgi_response_pipe, int i_cgi_post_body_pipe, pid_t pid_child);

/** 
 * This function is supposed to be used in a poll loop that polls on the specified file descriptor.
 * In every call it reades 1 byte from the file descriptor, stores it in the specified string and 
 * returns 0 if the header has been completely read.
 * 
 *
 * @param cpp_header String where to write the header. The string will only be \0 terminated if the 
 *                   header has been completely read.
 * @param i_fd File descriptor where to read the header from.
 * @param i_max_size Maximum size of the header.
 * @return 0 if reading the header is completed, 1 if a byte has been successfully read and -1 on error.
 */
int getHeader(char** cpp_header, int i_fd, int i_max_size);

//bool isValidCharacter(char* c_character);

/*
int pipeThrough(int i_source_fd, int i_dest_fd, bool b_is_source_non_blocking, 
                bool b_is_dest_non_blocking);

FILE* getCGIHeaderResponseStream(int i_source_fd);
*/
#endif
