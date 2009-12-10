#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>


#include "cgi.h"
#include "httpresponse.h"
#include "parser.h"
#include "staticfile.h"
#include "normalize.h"
#include "debug.h"
#include "envvar.h"
#include "secmem.h"
#include "pipe.h"
#include "typedef.h"

//static const int MAX_HEADER_SIZE = 8192;
static const int SCI_BUF_SIZE = 256;

extern int si_cgi_timeout_;
extern enum SCE_KNOWN_METHODS e_used_method;

/**
* Helper function for safely closing a pipe pair
*/
static void closePipes(int ia_pipe_fds[2])
{
	if (ia_pipe_fds[0] != -1)
	{
		close(ia_pipe_fds[0]);
	}
	
	if (ia_pipe_fds[1] != -1)
	{
		close(ia_pipe_fds[1]);
	}
}

void processCGIScript(const char* cp_path) 
{
	int i_success = 0;
	int ia_cgi_response_pipe[2] = {-1, -1};
	int ia_cgi_post_body_pipe[2] = {-1, -1};
	pid_t pid_child = 0;
	char* cpa_cgi_args[2];
	char* cp_path_to_file = parseFilepath(cp_path);   
	char* cp_file_name = parseFilename(cp_path);
	
	debugVerbose(CGICALL, "filename: %s.\n", cp_file_name);
	
	/*
	printEnvVarList();
	
	initEnvVarList("TEST_VARIABLE1", "this is just a test");
	appendToEnvVarList("TEST_VARIABLE2", "this is just another test");
	appendToEnvVarList("TEST_VARIABLE3", "this is just a third test");
	
	printEnvVarList();
	*/ 
	
	if (pipe(ia_cgi_response_pipe))
	{
		//TODO: safe exit
		debugVerbose(CGICALL, "Creating pipes to CGI script failed.\n");
	}
	
	if(e_used_method == POST)
	{
		if (pipe(ia_cgi_post_body_pipe))
		{
			//TODO: safe exit
			debugVerbose(CGICALL, "Creating pipes to CGI script failed: %d\n", errno);
		}
	//	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	//	{
	//		//TODO safe exit, call signal earlier?
	//		debugVerbose(CGICALL, "Setting signal handler failed.\n");
	//	}
		
	}
	
	
	/* Fork the child process */
	pid_child = fork();  
	
	switch (pid_child) {
		case 0:
			/* We are the child process */
			//TODO: exit im child process, was passiert mit parent?
			
			cpa_cgi_args[0] = malloc(sizeof(char)*(strlen(cp_file_name) + 1));
			secProof(cpa_cgi_args[0]);
			strncpy(cpa_cgi_args[0], cp_file_name, strlen(cp_file_name) + 1);   
			cpa_cgi_args[1] = NULL;
			
			i_success = clearenv();
			if(i_success == -1)
			{
				//TODO: safe exit
				debugVerbose(CGICALL, "Clearing environment failed.\n");
			}
			i_success = applyEnvVarList();
			if(i_success == -1)
			{
				//TODO: safe exit
				debugVerbose(CGICALL, "Applying environment variables failed.\n");
			}            
			
			i_success = chdir(cp_path_to_file);
			if(i_success == -1)
			{
				//TODO: safe exit
				debugVerbose(CGICALL, "Changing directory failed.\n");
			}
			printEnvVarList();
			secCleanup();
			
			// Duplicate the pipes to stdIN / stdOUT
			if (dup2(ia_cgi_response_pipe[1], STDOUT_FILENO) < 0)
			{
				//TODO: safe exit
				debugVerbose(CGICALL, "Duplication of pipes failed.\n");
			}
			
			if(e_used_method == POST)
			{
				if (dup2(ia_cgi_post_body_pipe[0], STDIN_FILENO) < 0)
				{
					//TODO: safe exit
					debugVerbose(CGICALL, "Duplication of pipes failed.\n");
				}
			}
			else
			{
			    i_success = close(STDIN_FILENO);
			    //TODO: exit
			}
			
			// Close the pipes
			closePipes(ia_cgi_post_body_pipe);
			closePipes(ia_cgi_response_pipe);
			
			// Execute the cgi script
			execv(cpa_cgi_args[0], cpa_cgi_args);
			
			debugVerbose(CGICALL, "Executing CGI script failed.\n");
			/* Abort child "immediately" with _exit */
			//TODO: exit
			exit(-1);
			
			case -1:
				// Error case
				
				closePipes(ia_cgi_post_body_pipe);
				closePipes(ia_cgi_response_pipe);
				//TODO: safe exit
				
			default:
				// Parent
				
				// We have no use for these pipe ends
				close(ia_cgi_response_pipe[1]);
				close(ia_cgi_post_body_pipe[0]);
				/*       
				if(e_used_method == POST)
				{
					if(dup2(STDIN_FILENO, ia_cgi_post_body_pipe[1]) < 0)
					{
						//TODO: safe exit
						debug(CGICALL, "Duplication of pipes failed.\n");
	}
	
	}
	*/  
				i_success = processCGIIO(ia_cgi_response_pipe[0], ia_cgi_post_body_pipe[1], pid_child);
				if(i_success == EXIT_FAILURE)
				{
					//TODO: safe exit
					debug(CGICALL, "CGI IO processing failed.\n");
				}
				
				closePipes(ia_cgi_post_body_pipe);
				closePipes(ia_cgi_response_pipe);
				break;
	};
	
	debug(CGICALL, "Finished processing CGI script.\n");
}


int processCGIIO(int i_cgi_response_pipe, int i_cgi_post_body_pipe, pid_t pid_child)
{
	io_pipe **pipes = NULL;
	int i_success = 0;
	//TODO: if header provided and error afterwards, what to do?
	bool b_header_provided = FALSE;
	bool b_first_get_header_call = TRUE;
	char* cp_cgi_response_header = NULL;
	
	cp_cgi_response_header = secCalloc((MAX_HEADER_SIZE + 1), sizeof(char));
	
	pipes = secMalloc(sizeof(io_pipe*) * ((e_used_method == POST) ? 2 : 1));
	pipes[0] = secMalloc(sizeof(io_pipe));
	
	i_success = initPipe(pipes[0], i_cgi_response_pipe, STDOUT_FILENO);
	
	
	if(i_success == EXIT_FAILURE)
	{
		//TODO: safe exit? or in processcgiscript?
		return EXIT_FAILURE;
	}
	
	if(e_used_method == POST)
	{
		pipes[1] = secMalloc(sizeof(io_pipe));
		i_success = initPipe(pipes[1], STDIN_FILENO, i_cgi_post_body_pipe);
		
		if(i_success == EXIT_FAILURE)
		{
			//TODO: safe exit? or in processcgiscript?
			return EXIT_FAILURE;
		}
	}
	
	while (1)
	{    
		i_success = pollPipes(pipes, si_cgi_timeout_, (e_used_method == POST) ? 2 : 1);
		
		//printPipe(pipes[0], "responsepipe after poll");
		
		if(i_success == 1)
		{
            if(b_header_provided == TRUE)
	            return EXIT_SUCCESS;
            else
                return EXIT_FAILURE;
		}
		if(i_success == -1)
		{
			debugVerbose(CGICALL, "Polling failed.\n");
			return EXIT_FAILURE;
		}
		
		
		if(e_used_method == POST)
		{
			//printPipe(pipes[1], "postbodypipe after poll");
			i_success = servePipe(pipes[1]);
			// printPipe(pipes[1], "postbodypipe after serve");
			
			if(i_success == EXIT_FAILURE)
			{
				debugVerbose(CGICALL, "An error occurred while providing body to cgi client.\n");
				//Writing failed, but that's not so bad. 
				//Possibly the cgi script terminated without reading the whole body
				pipes[1]->i_in_eof = 1;
				pipes[1]->i_out_eof = 1;
			}
		}
		
		//TODO: call parser, he somehow has to notify that he is finished
		
/*		if(!b_header_provided)
		{
			http_norm *hpn_info = normalizeHttp(i_cgi_response_pipe, TRUE);
			http_cgi_response *http_cgi_response_header = parseCgiResponseHeader(hpn_info);
			
			
			i_success = sendCGIHTTPResponseHeader(http_cgi_response_header);
			if(i_success == EXIT_SUCCESS)
			{
				debugVerbose(CGICALL, "CGI header provided successfully to http client.\n");
				b_header_provided = TRUE;
			}
			else
			{
				debugVerbose(CGICALL, "Providing cgi response header failed.\n");
				return EXIT_FAILURE;
			}
		}*/
		
		if(!b_header_provided)
		{
			if(pipes[0]->i_in_ready)
			{
				i_success = getHeader(&cp_cgi_response_header, pipes[0]->i_in_fd, MAX_HEADER_SIZE + 1);
				b_first_get_header_call = FALSE;
				pipes[0]->i_in_ready = 0;
				if(i_success == 0)
				{
					
					http_norm *hpn_info = normalizeHttp(cp_cgi_response_header, TRUE);
					http_cgi_response *http_cgi_response_header = parseCgiResponseHeader(hpn_info);
					
					
					i_success = sendCGIHTTPResponseHeader(http_cgi_response_header);
					if(i_success == EXIT_SUCCESS)
					{
						debugVerbose(CGICALL, "CGI header provided successfully to http client.\n");
						b_header_provided = TRUE;
						if(e_used_method == HEAD)
						{
						    pipes[0]->i_in_eof = 1;
						}
					}
					else
					{
						debugVerbose(CGICALL, "Providing cgi response header failed.\n");
						return EXIT_FAILURE;
					}
					
					/*          i_success = writeStringToFile(STDOUT_FILENO, cp_cgi_response_header);
					if(i_success == EXIT_FAILURE)
					{
						//TODO: exit
						secAbort();
				}
				*/          
					b_header_provided = TRUE;
					
				} 
				else if(i_success == -1)
				{
					//TODO: exit
					secAbort();
				}          
			}
		}
		else
		{
			i_success = servePipe(pipes[0]);
			
			if(i_success == EXIT_FAILURE)
			{
				debugVerbose(CGICALL, "An error occurred while reading cgi response.\n");
				return EXIT_FAILURE;
			}  
		}
	}
	
}


int getHeader(char** cpp_header, int i_fd, int i_max_size)
{
	char c_character;
	static int i_position = 0;
	static bool b_gotcr = FALSE;
	static bool b_gotnl = FALSE;
	
	if(cpp_header == NULL)
	{
	    return -1;
	}
	
	if(i_position >= i_max_size - 1)
	{
		//TODO: header ohne newline am ende?
		return EXIT_FAILURE;
	}
	
	ssize_t in_size = read(i_fd, &c_character, 1);
	
	if (in_size < 0) 
	{
		debugVerbose(CGICALL, "I/O error on inbound file.\n");
		secAbort();
	}
	else if (in_size == 0) 
	{
		//TODO: STATUS_SCRIPT_ERROR
		debugVerbose(CGICALL, "Invalid Header delimiter detected\n");
		secExit(STATUS_BAD_REQUEST);
	}
	if(c_character == '\r')
	{
		b_gotcr = TRUE;
	}
	else
	{
		if(c_character == '\n')
		{
			// replace previous \r by \n
			if(b_gotcr == TRUE)
			{
				(*cpp_header)[i_position-1] = '\n';
				(*cpp_header)[i_position] = '\0';
				--i_position;
				b_gotcr = FALSE;
			}
			// break up if we had found a previous \n, thats the only right way to get out here
			if(b_gotnl == TRUE)
			{
				//++i_position;
				(*cpp_header)[i_position] = '\n';
				(*cpp_header)[i_position+1] = '\0';
				(*cpp_header) = secRealloc((*cpp_header), (i_position + 2) * sizeof(char));
				return 0;
			}
			b_gotnl = TRUE;
		}
		else{
			if(isValid(&c_character, 0) == EXIT_FAILURE || b_gotcr == TRUE)
			{
				// TODO: Script error
				debugVerbose(FILEHANDLING, "Detected invalid char while retrieving header\n");
				secExit(STATUS_BAD_REQUEST);
			}
			else{
				b_gotcr = FALSE;
				b_gotnl = FALSE;
				
			}
		}
	}
	(*cpp_header)[i_position] = c_character;
	i_position++;
	
	return 1;
	}
	/*
	bool isValidCharacter(char* c_character)
	{
		if(*c_character == '\n' || *c_character == '\r')
			return TRUE;
		if(isCharacter(c_character, 0) == EXIT_SUCCESS)
			return TRUE;
		if(isBlank(c_character, 0) == EXIT_SUCCESS)
			return TRUE;
		return FALSE;
	}*/
	/*
	FILE* getCGIHeaderResponseStream(int i_source_fd)
		{
			ssize_t max_bytes_left_to_read = 0;
			ssize_t total_read_bytes = 0;
			char *cp_stream_memory = NULL;
			char ca_buffer[SCI_BUF_SIZE];
			bool b_first_iteration = TRUE;
			bool b_eof_reached = FALSE;
			
			//TODO: neu mit getc()
			//cp_stream_memory = (char*)secCalloc(total_read_bytes+1, sizeof(char));
			do 
			{
				// Read data from input pipe
				ssize_t read_bytes = 0;
				
				max_bytes_left_to_read = MAX_HEADER_SIZE - total_read_bytes;
				
				read_bytes = read(i_source_fd, ca_buffer, 
					 (sizeof(ca_buffer) < max_bytes_left_to_read) ? 
					 sizeof(ca_buffer) : max_bytes_left_to_read);
					 
					 if (read_bytes < 0) 
					 {
						 debugVerbose(6, "Error reading from pipe: %d\n", errno);
						 return NULL;        
		}
		
		if(read_bytes < SCI_BUF_SIZE)
		{ 
			b_eof_reached = TRUE;
		}
		
		total_read_bytes += read_bytes;
		
		if (b_first_iteration == TRUE)
		{
			//TODO: find solution to valgrind problem
			cp_stream_memory = (char*)secCalloc(total_read_bytes, sizeof(char));
			memcpy(cp_stream_memory, ca_buffer, total_read_bytes);
			debugVerbose(CGICALL, "Read %d bytes from pipe.\n", total_read_bytes);
			b_first_iteration = FALSE;
		}
		else
		{ 
			cp_stream_memory = (char*) secRealloc(cp_stream_memory, total_read_bytes);
			memcpy(cp_stream_memory + (total_read_bytes - read_bytes), ca_buffer, read_bytes);
			debugVerbose(CGICALL, "Read totally %d from pipe after realloc.\n", total_read_bytes);
			
			//Check maximum size.
			if(total_read_bytes >= MAX_HEADER_SIZE)
			{
				b_eof_reached = TRUE;
		}
		
		}
		
		} while (!b_eof_reached);
		
		cp_stream_memory[total_read_bytes] = '\0';
		return fmemopen((void*)cp_stream_memory, (size_t)total_read_bytes, "rb");
		}
		
		int pipeThrough(int i_source_fd, int i_dest_fd, bool b_is_source_non_blocking, 
					 bool b_is_dest_non_blocking)
					 {
						 char ca_buffer[SCI_BUF_SIZE];
						 ssize_t written_bytes = 0;
						 ssize_t read_bytes = 0;
						 ssize_t bytes_left_to_write = 0;
						 bool b_short_write = FALSE;
						 
						 do
						 {
							 if(!b_short_write)
							 {
								 read_bytes = read(i_source_fd, ca_buffer, sizeof(ca_buffer));
								 
								 if (read_bytes <= 0) 
								 {
									 debugVerbose(CGICALL, "Read nothing from source.\n");
									 if (read_bytes == 0 || ((b_is_source_non_blocking) && (errno == EAGAIN)))
									 {            
										 return 0;
						 }
						 
						 debugVerbose(CGICALL, "Error while reading.\n"); 
						 return -1;      
						 }    
						 written_bytes = write(i_dest_fd, ca_buffer, read_bytes);
						 bytes_left_to_write = read_bytes - written_bytes;
						 } 
						 else
						 {
							 written_bytes = write(i_dest_fd, ca_buffer + (read_bytes - bytes_left_to_write),
					 bytes_left_to_write);
					 
					 if(written_bytes > 0)
					 {
						 bytes_left_to_write -= written_bytes;
						 }
						 }
						 
						 
						 if (written_bytes < 0) 
						 {       
							 if((b_is_dest_non_blocking) && (errno == EAGAIN))
							 return 1;  
							 debugVerbose(CGICALL, "Error while writing.\n");
							 return -1;
							 
						 } else if (bytes_left_to_write > 0) 
						 {
							 b_short_write = TRUE;
							 debugVerbose(CGICALL, "Short write.\n");
						 }
						 else
						 {
							 debugVerbose(CGICALL, "Wrote %d bytes to pipe.\n", written_bytes);
							 b_short_write = FALSE;
						 }
						 } while(1);
						 }
						 
						 */
	/*
	int provideCGIBodyToHTTPClient(int i_source_fd, int i_dest_fd) 
	{
		char ca_buffer[SCI_BUF_SIZE];
		ssize_t read_bytes = 0;
		ssize_t written_bytes = 0;
		ssize_t bytes_left_to_write = 0;
		bool b_short_write = FALSE;
		
		do 
		{
			if(!b_short_write)
			{
				read_bytes = read(i_source_fd, ca_buffer, sizeof(ca_buffer));
				
				if (read_bytes <= 0) 
				{
					
					if (read_bytes == 0 || errno == EAGAIN) 
					{            
						debugVerbose(CGICALL, "Read nothing from cgi: %d\n", errno);
						return EXIT_SUCCESS;
	}
	
	debugVerbose(CGICALL, "Error while reading.\n"); 
	return EXIT_FAILURE;      
	}
	written_bytes = write(i_dest_fd, ca_buffer, read_bytes);
	bytes_left_to_write = read_bytes - written_bytes;
	}
	else
	{
		written_bytes = write(i_dest_fd, ca_buffer + (read_bytes - bytes_left_to_write), bytes_left_to_write);
		
		if(written_bytes > 0)
		{
			bytes_left_to_write -= written_bytes;
	}
	}
	
	if (written_bytes < 0) 
	{       
		debugVerbose(CGICALL, "Error while writing.\n");
		return EXIT_FAILURE;
		
	} else if (bytes_left_to_write > 0) 
	{
		b_short_write = TRUE;
		debugVerbose(CGICALL, "Short write.\n");
	}
	else
	{
		b_short_write = FALSE;
		debugVerbose(CGICALL, "Wrote %d bytes to http client.\n", written_bytes);
	}
	} while (1);
	}
	
	ssize_t provideCGIBodyToHTTPClient(int i_source_fd, int i_dest_fd) 
	{
		ssize_t total_read_bytes = 0;
		char ca_buffer[SCI_BUF_SIZE];
		bool b_first_iteration = TRUE;
		bool b_eof_reached = FALSE;
		
		do 
		{
			// Read data from input pipe
			ssize_t read_bytes;
			
			read_bytes = read(i_source_fd, ca_buffer, sizeof(ca_buffer));
			
			if (read_bytes < 0) 
			{
				debug(6, "Error reading from pipe: %d\n", errno);
				return -1;        
	}
	
	if(read_bytes < SCI_BUF_SIZE)
	{
		ca_buffer[read_bytes] = '\0';
		read_bytes++;   
		b_eof_reached = TRUE;
	}
	
	total_read_bytes += read_bytes;
	
	if (b_first_iteration == TRUE)
	{
		(*cpp_cgi_response) = (char*)secMalloc(total_read_bytes);
		strncpy((*cpp_cgi_response), ca_buffer, total_read_bytes);
		debug(CGICALL, "Read from pipe: %s\n", (*cpp_cgi_response));
		b_first_iteration = FALSE;
	}
	else
	{ 
		(*cpp_cgi_response) = (char*) secRealloc((*cpp_cgi_response), total_read_bytes);
		strncpy((*cpp_cgi_response) + (total_read_bytes - read_bytes), ca_buffer, read_bytes);
		debug(CGICALL, "Read from pipe after realloc: %s\n", (*cpp_cgi_response));
		
		//Check maximum size.
		if(total_read_bytes > SCI_MAX_CGI_RESPONSE_LENGTH)
		{
			debug(CGICALL, "CGI response exceeds maximum size.\n");
			return -1;
	}
	
	}
	
	} while (!b_eof_reached);
	
	return total_read_bytes;
	}
	*/
	
	
