#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <signal.h>

#include "parser.h"
#include "normalize.h"
#include "debug.h"
#include "cgi.h"
#include "envvar.h"
#include "secmem.h"
#include "typedef.h"

static const int SCI_BUF_SIZE = 256;
static const int SCI_MAX_CGI_RESPONSE_LENGTH = 100000;

extern int si_cgi_timeout_;
extern const enum SCE_KNOWN_METHODS e_used_method;

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

/**
 * Put a file descriptor into non-blocking mode.
 */
static int setNonblocking(int i_fd)
{
    int old_mode = fcntl(i_fd, F_GETFL);
    if (old_mode < 0) 
    {
        return -1;
    }     

    return fcntl(i_fd, F_SETFL, old_mode | O_NONBLOCK);
}

void processCGIScript(const char* cp_path) 
{
    int i_success = 0;
    int ia_cgi_response_pipe[2] = {-1, -1};
    int ia_cgi_post_body_pipe[2] = {-1, -1};
    pid_t pid_child = 0;
    char* cpa_cgi_args[] = {"testscript", NULL};

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
        debug(CGICALL, "Creating pipes to CGI script failed: %d\n", errno);
    }
    if (setNonblocking(ia_cgi_response_pipe[0]))
    {
        //TODO safe exit
        debug(CGICALL, "Setting pipes non-blocking failed: %d\n", errno);
    }

    debug(CGICALL, "Http-Method: %d\n", e_used_method);
    
    if(e_used_method == POST)
    {
        if (pipe(ia_cgi_post_body_pipe))
        {
            //TODO: safe exit
            debug(CGICALL, "Creating pipes to CGI script failed: %d\n", errno);
        }
        
        if (setNonblocking(ia_cgi_post_body_pipe[1]))
        {
            //TODO safe exit
            debug(CGICALL, "Setting pipes non-blocking failed: %d\n", errno);
        }
        if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        {
            //TODO safe exit
            debug(CGICALL, "Setting signal handler failed.\n");
        }
        
    }
    
    
    /* Fork the child process */
    pid_child = fork();  

    switch (pid_child) {
        case 0:
            /* We are the child process */
            fprintf(stderr, "we are the child\n");
            
            i_success = clearenv();
            if(i_success == -1)
            {
                //TODO: safe exit
                debug(CGICALL, "Clearing environment failed.\n");
            }
            i_success = applyEnvVarList();
            if(i_success == -1)
            {
                //TODO: safe exit
                debug(CGICALL, "Applying environment variables failed.\n");
            }
            
            secCleanup();
            
            i_success = chdir("../cgi-bin/");
            if(i_success == -1)
            {
                //TODO: safe exit
                debug(CGICALL, "Changing directory failed.\n");
            }
            fprintf(stderr, "before exec\n");
            
            
            // Duplicate the pipes to stdIN / stdOUT
            if (dup2(ia_cgi_response_pipe[1], STDOUT_FILENO) < 0)
            {
                //TODO: safe exit
                debug(CGICALL, "Duplication of pipes failed.\n");
            }
            
            if(e_used_method == POST)
            {
                if (dup2(ia_cgi_post_body_pipe[0], STDIN_FILENO) < 0)
                {
                    //TODO: safe exit
                    debug(CGICALL, "Duplication of pipes failed.\n");
                }
            }

            // Close the pipes
            closePipes(ia_cgi_post_body_pipe);
            closePipes(ia_cgi_response_pipe);

            // Execute the cgi script
            execv(cp_path, cpa_cgi_args);

            debug(CGICALL, "Executing CGI script failed.\n");
            /* Abort child "immediately" with _exit */
            //TODO: safe exit
            break;

        case -1:
            // Error case

            closePipes(ia_cgi_post_body_pipe);
            closePipes(ia_cgi_response_pipe);

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
            if(i_success == -1)
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
    struct pollfd poll_fd[(e_used_method == POST) ? 2 : 1];
    int i_success = 0;
    int i_poll_result = 0;
    ssize_t response_length = 0;
    char* cp_cgi_response = NULL;
    bool b_read_successful = FALSE;
    bool b_write_successful = TRUE;
    FILE* response_stream = NULL;
    
    /* Setup poll_fds for child standard output and 
     * standard error stream */
    poll_fd[0].fd = i_cgi_response_pipe;
    poll_fd[0].events = POLLIN;
    poll_fd[0].revents = 0;
    
    response_stream = fdopen(poll_fd[0].fd, "re");
    
    if(e_used_method == POST)
    {
        poll_fd[1].fd = i_cgi_post_body_pipe;
        poll_fd[1].events = POLLOUT;
        poll_fd[1].revents = 0;
        b_write_successful = FALSE;
    }


    while (!b_read_successful || !b_write_successful)
    {
        // Poll for more events

        i_poll_result = poll(poll_fd, sizeof(poll_fd)/sizeof(poll_fd[0]), si_cgi_timeout_);
    
        debug(CGICALL, "Iteration: %d, %d\n", poll_fd[0].revents, poll_fd[1].revents);
        if (i_poll_result < 0)
        {
            debug(CGICALL, "Polling failed: %d\n", errno);
            return -1;
        }
        
        // Timeout?
        if (i_poll_result == 0)
        {
            // Kill child
            debug(CGICALL, "Child process timed out, killing it.\n");
            kill(pid_child, 0);
            //TODO: Send 501 to http client
            
            return -1;
        }
        
        if(e_used_method == POST)
        {
            if(poll_fd[1].revents & (POLLERR))
            {
                debug(CGICALL, "A problem occured on the cgi post body pipe.\n");
                b_write_successful = TRUE;
            }
            if(poll_fd[1].revents & (POLLHUP))
            {
                debug(CGICALL, "The other end closed the cgi post body pipe.\n");
                b_write_successful = TRUE;
            }
            
            /* Drain the standard output pipe */
            if ((poll_fd[1].revents & POLLOUT) && (!b_write_successful))
            {   
                i_success = drainPipeTo(STDIN_FILENO, poll_fd[1].fd);
                if (i_success < 0)
                {
                    debug(CGICALL, "An error occurred while writing.\n");
                    b_write_successful = TRUE;
                    //Writing failed, but that's not so bad. 
                    //Possibly the cgi script terminated without reading the whole body
                }
                else if(i_success == 0)
                {
                    debug(CGICALL, "Write completed.\n");
                    b_write_successful = TRUE;
                    poll_fd[1].events = 0;
                }
                else
                {   
                    debug(CGICALL, "Write not completed (yet).\n");
                    poll_fd[1].revents ^= POLLOUT;
                }
            }
        }

        if((poll_fd[0].revents & (POLLERR)) && (!b_read_successful))
        {
            debug(CGICALL, "A problem occured on the cgi response pipe.\n");
            //return -1;
        }
        
        if((poll_fd[0].revents & (POLLHUP)) && (!b_read_successful))
        {
            debug(CGICALL, "The other side closed the cgi response pipe.\n");
            //return -1;
        }

        /* Drain the standard output pipe */
        if ((poll_fd[0].revents & POLLIN) && (!b_read_successful))
        {   
        
            http_norm *hpn_info = normalizeHttp(response_stream);
            i_success = parseCgiResponseHeader(hpn_info);
            
            if(i_success == EXIT_SUCCESS)
            {
                //TODO: send http_response header
                //TODO: send body    
            }
            else
            {
            
            }
            
            response_length = drainPipe(poll_fd[0].fd, &cp_cgi_response);
            if (response_length < 0)
            {
                debug(CGICALL, "Could not read from pipe.\n");
                return -1;
            }
            
            debug(CGICALL, "Read %d bytes as CGI response.\n", response_length);
            b_read_successful = TRUE;
            
            poll_fd[0].revents ^= POLLIN;
        } 
    } 
    
    debug(CGICALL, "Normal exit.\n");

    return 0;
    
}

int drainPipeTo(int i_source_fd, int i_dest_fd)
{
    char ca_buffer[2048];
 
    do
    {
        ssize_t written_bytes = 0;
        ssize_t read_bytes = 0;

        read_bytes = read(i_source_fd, ca_buffer, sizeof(ca_buffer));
        
        if (read_bytes <= 0) {
             debug(CGICALL, "Read nothing from source.\n");
            if (read_bytes == 0) {            
                return 0;
            }

            debug(CGICALL, "Error while reading.\n"); 
            return -1;      
        }     


        written_bytes = write(i_dest_fd, ca_buffer, strlen(ca_buffer));
        debug(CGICALL, "Wrote %d bytes to cgi pipe.\n", written_bytes);
        if (written_bytes < 0) 
        {       
            if(errno == EAGAIN)
                return 1;  
            debug(CGICALL, "Error while writing.\n");
            return -1;

        } else if (written_bytes < read_bytes) 
        {
            debug(CGICALL, "Short write.\n");
        }
    } while(1);
}

ssize_t drainPipe(int i_source_fd, char** cpp_cgi_response) 
{
    ssize_t total_read_bytes = 0;
    char ca_buffer[SCI_BUF_SIZE];
    bool b_first_iteration = TRUE;
    bool b_eof_reached = FALSE;
 
    do 
    {
        /* Read data from input pipe */
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

