#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/time.h>


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
	
	if (pipe(ia_cgi_response_pipe))
	{
		debugVerbose(CGICALL, "Creating pipes to CGI script failed.\n");
		secAbort();
	}

	if (pipe(ia_cgi_post_body_pipe))
	{
		debugVerbose(CGICALL, "Creating pipes to CGI script failed.\n");
		closePipes(ia_cgi_response_pipe);
		secAbort();
	}
	
	
	/* Fork the child process */
	pid_child = fork();  
	
	switch (pid_child) {
		case 0:
			/* We are the child process */
			
			cpa_cgi_args[0] = malloc(sizeof(char)*(strlen(cp_file_name) + 1));
			secProof(cpa_cgi_args[0]);
			strncpy(cpa_cgi_args[0], cp_file_name, strlen(cp_file_name) + 1);   
			cpa_cgi_args[1] = NULL;
			
			i_success = clearenv();
			
			if(i_success == -1)
			{
			//TODO: exit or abort, abort sends something, but i dont want that as child	
				debugVerbose(CGICALL, "Clearing environment failed.\n");
				closePipes(ia_cgi_post_body_pipe);
			    closePipes(ia_cgi_response_pipe);
				secExit(STATUS_CANCEL);
			}
			i_success = applyEnvVarList();
			if(i_success == -1)
			{
				//TODO: exit or abort, abort sends something, but i dont want that as child
				debugVerbose(CGICALL, "Applying environment variables failed.\n");
				closePipes(ia_cgi_post_body_pipe);
			    closePipes(ia_cgi_response_pipe);
				secExit(STATUS_CANCEL);
			}            
			
			i_success = chdir(cp_path_to_file);
			if(i_success == -1)
			{
				//TODO: exit or abort, abort sends something, but i dont want that as child
				debugVerbose(CGICALL, "Changing directory failed.\n");
				closePipes(ia_cgi_post_body_pipe);
			    closePipes(ia_cgi_response_pipe);
				secExit(STATUS_CANCEL);
			}
			printEnvVarList();
			secCleanup();
			
			// Duplicate the pipes to stdIN / stdOUT
			if ((dup2(ia_cgi_response_pipe[1], STDOUT_FILENO) < 0) || (dup2(ia_cgi_post_body_pipe[0], STDIN_FILENO) < 0))
			{
				//TODO: exit or abort, abort sends something, but i dont want that as child
				debugVerbose(CGICALL, "Duplication of pipes failed.\n");
				closePipes(ia_cgi_post_body_pipe);
			    closePipes(ia_cgi_response_pipe);
				secExit(STATUS_CANCEL);
			}
/*
			if (dup2(ia_cgi_post_body_pipe[0], STDIN_FILENO) < 0)
			{
				//TODO: exit or abort, abort sends something, but i dont want that as child
				debugVerbose(CGICALL, "Duplication of pipes failed.\n");
				closePipes(ia_cgi_post_body_pipe);
		        closePipes(ia_cgi_response_pipe);
				secExit(STATUS_CANCEL);
			}
	*/		
			// Close the pipes
			closePipes(ia_cgi_post_body_pipe);
			closePipes(ia_cgi_response_pipe);
			
			// Execute the cgi script
			execv(cpa_cgi_args[0], cpa_cgi_args);
			
			debugVerbose(CGICALL, "Executing CGI script failed.\n");
			free(cpa_cgi_args[0]);
			exit(-1);
			
			case -1:
				// Error case
				
				closePipes(ia_cgi_post_body_pipe);
				closePipes(ia_cgi_response_pipe);
				secAbort();
				
			default:
				// Parent
				
				// We have no use for these pipe ends
				close(ia_cgi_response_pipe[1]);
				close(ia_cgi_post_body_pipe[0]);

				i_success = processCGIIO(ia_cgi_response_pipe[0], ia_cgi_post_body_pipe[1], pid_child);
				if(i_success == EXIT_FAILURE)
				{
					debug(CGICALL, "CGI IO processing failed.\n");
					closePipes(ia_cgi_post_body_pipe);
				    closePipes(ia_cgi_response_pipe);
				    kill(pid_child, SIGTERM);
					secExit(STATUS_INTERNAL_SERVER_ERROR);
				}
				
				closePipes(ia_cgi_post_body_pipe);
				closePipes(ia_cgi_response_pipe);
				break;
	};
	
	debug(CGICALL, "Finished processing CGI script.\n");
	kill(pid_child, SIGTERM);
}


int processCGIIO(int i_cgi_response_pipe, int i_cgi_post_body_pipe, pid_t pid_child)
{
	io_pipe **pipes = NULL;
	int i_success = 0;
	bool b_header_provided = FALSE;
	bool b_first_get_header_call = TRUE;
	char* cp_cgi_response_header = NULL;
	struct timeval timeout_time;
	struct timeval current_time;
	
	cp_cgi_response_header = secCalloc((MAX_HEADER_SIZE + 1), sizeof(char));
	
	pipes = secMalloc(sizeof(io_pipe*) * (2));
	pipes[0] = secMalloc(sizeof(io_pipe));
	
	i_success = initPipe(pipes[0], i_cgi_response_pipe, STDOUT_FILENO);
		
	if(i_success == EXIT_FAILURE)
	{
	    debugVerbose(CGICALL, "Pipe init failed.\n");
		return EXIT_FAILURE;
	}
	
	pipes[1] = secMalloc(sizeof(io_pipe));
	i_success = initPipe(pipes[1], STDIN_FILENO, i_cgi_post_body_pipe);
	
	if(i_success == EXIT_FAILURE)
	{
	    debugVerbose(CGICALL, "Pipe init failed.\n");
		return EXIT_FAILURE;
	}

	if(gettimeofday(&timeout_time, NULL) != 0)
    {
        debugVerbose(CGICALL, "Making timestamp failed.\n");
		return EXIT_FAILURE;
    }
    
    timeout_time.tv_sec += si_cgi_timeout_/1000;
    timeout_time.tv_usec += (si_cgi_timeout_%1000) * 1000;
    if(timeout_time.tv_usec >= 1000000)
    {
        timeout_time.tv_sec++;
        timeout_time.tv_usec -= 1000000;
    }
    
	while (1)
	{    
		i_success = pollPipes(pipes, si_cgi_timeout_, 2);
		
		if(i_success == 1)
		{
            if(b_header_provided == TRUE)
	            return EXIT_SUCCESS;
            else
                return EXIT_FAILURE;
		}
		if(i_success == 2)
		{
		    if(b_header_provided)
		    {
		        debugVerbose(CGICALL, "Polling timed out, but already sent header.\n");
		        return EXIT_SUCCESS;
		    }
		    
			debugVerbose(CGICALL, "CGI script timed out.\n");
			return EXIT_FAILURE;
		}
		
		if(i_success == -1)
		{
		    if(b_header_provided)
		    {
		        debugVerbose(CGICALL, "Polling failed, but header was provided successfully.\n");
		        //Don't send error messages
		        close(i_cgi_response_pipe);
		        close(i_cgi_post_body_pipe);
		        kill(pid_child, SIGTERM);
		        secExit(STATUS_CANCEL);
		    }
			debugVerbose(CGICALL, "Polling failed.\n");
			return EXIT_FAILURE;
		}		
		
		i_success = servePipe(pipes[1]);
		
		if(i_success == EXIT_FAILURE)
		{
			debugVerbose(CGICALL, "An error occurred while providing body to cgi client.\n");
			//Writing failed, but that's not so bad. 
			//Possibly the cgi script terminated without reading the whole body
			pipes[1]->i_in_eof = 1;
			pipes[1]->i_out_eof = 1;
		}

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
						if(e_used_method == HEAD)
						{
						    pipes[0]->i_in_eof = 1;
						}
						b_header_provided = TRUE;
					}
					else
					{
						debugVerbose(CGICALL, "Providing cgi response header failed.\n");
						return EXIT_FAILURE;
					}    			
					
				} 
				else if(i_success == -1)
				{
					return EXIT_FAILURE;
				}          
			}
			
			if(gettimeofday(&current_time, NULL) != 0)
            {
                debugVerbose(CGICALL, "Making timestamp failed.\n");
		        return EXIT_FAILURE;
            }
            
            if(timercmp(&current_time, &timeout_time, >))
            {
                debugVerbose(CGICALL, "No header was provided within cgi timeout.\n");
                return EXIT_FAILURE;
            }
		}
		else
		{
			i_success = servePipe(pipes[0]);
			
			if(i_success == EXIT_FAILURE)
			{
		        debugVerbose(CGICALL, "Serving cgi body failed, but header was provided successfully.\n");
		        //Don't send error messages
		        close(i_cgi_response_pipe);
		        close(i_cgi_post_body_pipe);
		        kill(pid_child, SIGTERM);
		        secExit(STATUS_CANCEL);
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
		debugVerbose(CGICALL, "Invalid Header delimiter detected, or header too big.\n");
		return -1;
	}
	
	ssize_t in_size = read(i_fd, &c_character, 1);
	
	if (in_size < 0) 
	{
		debugVerbose(CGICALL, "I/O error on inbound file.\n");
		secAbort();
	}
	else if (in_size == 0) 
	{
		debugVerbose(CGICALL, "Invalid Header delimiter detected\n");
		return -1;
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
				debugVerbose(CGICALL, "Detected invalid char while retrieving header\n");
				return -1;
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

