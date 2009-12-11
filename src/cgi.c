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
		//TODO: safe exit
		debugVerbose(CGICALL, "Creating pipes to CGI script failed.\n");
	}
	
//	if(e_used_method == POST)
//	{
		if (pipe(ia_cgi_post_body_pipe))
		{
			//TODO: safe exit
			debugVerbose(CGICALL, "Creating pipes to CGI script failed: %d\n", errno);
		}
		
		//TODO: an den anfang?
		/*
		if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		{
			//TODO safe exit, call signal earlier?
			debugVerbose(CGICALL, "Setting signal handler failed.\n");
		}
		*/
		
//	}
	
	
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
			
			//if(e_used_method == POST)
			//{
				if (dup2(ia_cgi_post_body_pipe[0], STDIN_FILENO) < 0)
				{
					//TODO: safe exit
					debugVerbose(CGICALL, "Duplication of pipes failed.\n");
				}
			//}
			/*
			else
			{
			    i_success = close(STDIN_FILENO);
			    //TODO: exit
			}*/
			
			// Close the pipes
			closePipes(ia_cgi_post_body_pipe);
			closePipes(ia_cgi_response_pipe);
			
			// Execute the cgi script
			execv(cpa_cgi_args[0], cpa_cgi_args);
			
			debugVerbose(CGICALL, "Executing CGI script failed.\n");
			/* Abort child "immediately" with _exit */
			//TODO: exit
			free(cpa_cgi_args[0]);
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
	struct timeval timeout_time;
	struct timeval current_time;
	
	cp_cgi_response_header = secCalloc((MAX_HEADER_SIZE + 1), sizeof(char));
	
	pipes = secMalloc(sizeof(io_pipe*) * (2));
	pipes[0] = secMalloc(sizeof(io_pipe));
	
	i_success = initPipe(pipes[0], i_cgi_response_pipe, STDOUT_FILENO);
	
	
	if(i_success == EXIT_FAILURE)
	{
	    debugVerbose(CGICALL, "Pipe init failed.\n");
		//TODO: safe exit? or in processcgiscript?
		return EXIT_FAILURE;
	}
	
//	if(e_used_method == POST)
//	{
		pipes[1] = secMalloc(sizeof(io_pipe));
		i_success = initPipe(pipes[1], STDIN_FILENO, i_cgi_post_body_pipe);
		
		if(i_success == EXIT_FAILURE)
		{
		    debugVerbose(CGICALL, "Pipe init failed.\n");
			//TODO: safe exit? or in processcgiscript?
			return EXIT_FAILURE;
		}
//	}

	if(gettimeofday(&timeout_time, NULL) != 0)
    {
        //TODO: exit?
    }
    
//    debug(CGICALL, "before calc: time: %i, micro: %i\n", timeout_time.tv_sec, timeout_time.tv_usec);
    //TODO: Overflow?
    timeout_time.tv_sec += si_cgi_timeout_/1000;
    timeout_time.tv_usec += (si_cgi_timeout_%1000) * 1000;
    if(timeout_time.tv_usec >= 1000000)
    {
        timeout_time.tv_sec++;
        timeout_time.tv_usec -= 1000000;
    }
    debug(CGICALL, "after calc: time: %i, micro: %i\n", timeout_time.tv_sec, timeout_time.tv_usec);
    
	while (1)
	{    
	    debug(CGICALL, "time: %i, micro: %i\n", timeout_time.tv_sec, timeout_time.tv_usec);
		i_success = pollPipes(pipes, si_cgi_timeout_, 2);
		
		//printPipe(pipes[0], "responsepipe after poll");
		
		if(i_success == 1)
		{
            if(b_header_provided == TRUE)
	            return EXIT_SUCCESS;
            else
                return EXIT_FAILURE;
		}
		if(i_success == 2)
		{
		    //TODO: SIGTERM und SIGKILL. 
		    if(b_header_provided)
		    {
		    //TODO: kill?
		        debugVerbose(CGICALL, "Polling timed out, but already sent header.\n");
		        return EXIT_SUCCESS;
		    }
		    
            kill(pid_child, SIGKILL);
			debugVerbose(CGICALL, "CGI script timed out.\n");
			return EXIT_FAILURE;
		}
		
		if(i_success == -1)
		{
		    //TODO: SIGTERM und SIGKILL. 
			debugVerbose(CGICALL, "Polling failed.\n");
			return EXIT_FAILURE;
		}
		
		
		//if(e_used_method == POST)
		//{
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
		//}

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
					//TODO: exit
					secExit(STATUS_INTERNAL_SERVER_ERROR);
				}          
			}
			
			if(gettimeofday(&current_time, NULL) != 0)
            {
                //TODO: exit?
            }
            
            if(timercmp(&current_time, &timeout_time, >))
            {
                debugVerbose(CGICALL, "No header was provided within cgi timeout.\n");
                //TODO: SIGTERM und SIGKILL. 
                kill(pid_child, SIGKILL);
                return EXIT_FAILURE;
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
		debugVerbose(CGICALL, "Invalid Header delimiter detected, or header too big.\n");
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
				debugVerbose(CGICALL, "Detected invalid char while retrieving header\n");
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

