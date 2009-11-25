#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string.h>

#include "debug.h"
#include "cgi.h"
#include "envvar.h"
#include "secmem.h"
#include "typedef.h"

static const int SCI_BUF_SIZE = 256;
static const int SCI_MAX_CGI_RESPONSE_LENGTH = 100000;

extern int si_cgi_timeout_;

static size_t http_body_length_ = 0;

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

void processCGIScript(const char* cp_path, const char* cp_http_body) 
{
    int i_success = 0;
    ssize_t written_bytes = 0;
    int ia_cgi_response_pipe[2] = {-1, -1};
    int ia_cgi_post_body_pipe[2] = {-1, -1};
    pid_t pid_child = 0;
    char* cpa_cgi_args[] = {"testscript", NULL};

     
    if(cp_http_body != NULL)
        http_body_length_ = strlen(cp_http_body);
    printEnvVarList();

    initEnvVarList("TEST_VARIABLE1", "this is just a test");
    appendToEnvVarList("TEST_VARIABLE2", "this is just another test");
    appendToEnvVarList("TEST_VARIABLE3", "this is just a third test");
    
    printEnvVarList();
    
    if (pipe(ia_cgi_response_pipe) || pipe(ia_cgi_post_body_pipe))
    {
        //TODO: safe exit
        debug(CGICALL, "Creating pipes to CGI script failed: %d\n", errno);
    }
    if (setNonblocking(ia_cgi_response_pipe[0])
        || setNonblocking(ia_cgi_post_body_pipe[1]))
    {
        //TODO safe exit
        debug(CGICALL, "Setting pipes non-blocking failed: %d\n", errno);
    }
    
    if((cp_http_body != NULL)) 
    {   
        http_body_length_ = strlen(cp_http_body);
        if(http_body_length_ > 0) 
        {
       // ret = poll(fds, 2, 1000);

            written_bytes = provideMessageBodyToCGIScript(ia_cgi_post_body_pipe[1], cp_http_body, 0);
            if(i_success < 0)
            {
                //TODO: safe exit
                debug(CGICALL, "Providing message body to CGI script failed.\n");
            }
            if(written_bytes < http_body_length_) 
            {
                debug(CGICALL, "Could not write whole http body at once.\n");
            }
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
            if (dup2(ia_cgi_post_body_pipe[0], STDIN_FILENO) < 0 ||
                dup2(ia_cgi_response_pipe[1], STDOUT_FILENO) < 0)
            {
                //TODO: safe exit
                debug(CGICALL, "Duplication of pipes failed.\n");
            }

            // Close the pipes
            closePipes(ia_cgi_post_body_pipe);
            closePipes(ia_cgi_response_pipe);

            // Execute the cgi script
            execv(cp_path, cpa_cgi_args);

            debug(CGICALL, "Executing CGI script failed.\n");
            /* Abort child "immediately" with _exit */
            _exit(EXIT_FAILURE);
            break;

        case -1:
            // Error case

            closePipes(ia_cgi_post_body_pipe);
            closePipes(ia_cgi_response_pipe);

        default:
            // Parent

            // We have no use for these pipe ends
            close(ia_cgi_post_body_pipe[0]);
            close(ia_cgi_response_pipe[1]);
            
            i_success = processCGIIO(ia_cgi_response_pipe[0], ia_cgi_post_body_pipe[1], 
                                     written_bytes, (written_bytes < http_body_length_), 
                                     pid_child, cp_http_body);
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


int processCGIIO(int i_cgi_response_pipe, int i_cgi_post_body_pipe, ssize_t written_bytes, 
                 bool b_provide_http_body, pid_t pid_child, const char* cp_http_body)
{
    struct pollfd poll_fd[(b_provide_http_body) ? 2 : 1];
    int i_poll_result = 0;
    ssize_t response_length = 0;
    ssize_t total_written_bytes = written_bytes;
    char* cp_cgi_response = NULL;
    bool b_read_successful = FALSE;
    bool b_write_successful = TRUE;
    
    /* Setup poll_fds for child standard output and 
     * standard error stream */
    poll_fd[0].fd = i_cgi_response_pipe;
    poll_fd[0].events = POLLIN;
    poll_fd[0].revents = 0;
    
    if(b_provide_http_body)
    {
        poll_fd[1].fd = i_cgi_post_body_pipe;
        poll_fd[1].events = POLLOUT;
        poll_fd[1].revents = 0;
        
        b_write_successful = FALSE;
    }

    while ((!b_read_successful) || (!b_write_successful))
    {
        // Poll for more events

        i_poll_result = poll(poll_fd, sizeof(poll_fd)/sizeof(poll_fd[0]), si_cgi_timeout_);
    
        if (i_poll_result < 0)
        {
            debug(CGICALL, "Polling for write failed: %d\n", errno);
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

        if(b_provide_http_body && !b_write_successful) 
        {
            if (poll_fd[1].revents & POLLOUT)
            {   
                debug(CGICALL, "Here.\n");
                written_bytes = provideMessageBodyToCGIScript(poll_fd[1].fd, cp_http_body, 
                                                          total_written_bytes);                                           
                if(written_bytes < 0)
                {
                    debug(CGICALL, "Providing message body to CGI script failed.\n");
                    return -1;
                }
                total_written_bytes += written_bytes;               
                if(total_written_bytes == http_body_length_) 
                {
                    debug(CGICALL, "Writing http_body completed.\n");
                    b_write_successful = TRUE;
                    poll_fd[1].events = 0;
                }
                
                poll_fd[1].revents ^= POLLOUT;
            }

            if((poll_fd[1].revents & (POLLHUP | POLLERR)) && (!b_write_successful))
            {
                debug(CGICALL, "A problem occured on the post body pipe.\n");
                return -1;
            }
        }

        /* Drain the standard output pipe */
        if ((poll_fd[0].revents & POLLIN) && (!b_read_successful))
        {   
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

        if((poll_fd[0].revents & (POLLHUP | POLLERR)) && (!b_read_successful))
        {
            debug(CGICALL, "A problem occured on the cgi response pipe.\n");
            return -1;
        }
    } 
    
    debug(CGICALL, "Normal exit.\n");

    return 0;
    
}


/*
int processCGIIO(int i_cgi_response_pipe, int i_cgi_post_body_pipe, ssize_t written_bytes, 
                 bool b_provide_http_body, pid_t pid_child, const char* cp_http_body)
{
    struct pollfd poll_fd_read[1];
    struct pollfd poll_fd_write[1];
    int i_poll_result_read = 0;
    int i_poll_result_write = 0;
    ssize_t response_length = 0;
    ssize_t total_written_bytes = written_bytes;
    char* cp_cgi_response = NULL;
    bool b_read_successful = FALSE;
    bool b_write_successful = TRUE;
    
    poll_fd_read[0].fd = i_cgi_response_pipe;
    poll_fd_read[0].events = POLLIN;
    poll_fd_read[0].revents = 0;
    
    if(b_provide_http_body)
    {
        poll_fd_write[0].fd = i_cgi_post_body_pipe;
        poll_fd_write[0].events = POLLOUT;
        poll_fd_write[0].revents = 0;
        
        b_write_successful = FALSE;
    }

    while ((!b_read_successful) || (!b_write_successful))
    {
        // Poll for more events
        if(b_provide_http_body && !b_write_successful)
        {
            i_poll_result_write = poll(poll_fd_write, sizeof(poll_fd_write)/sizeof(poll_fd_write[0]), si_cgi_timeout_);
        
            if (i_poll_result_write < 0)
            {
                debug(6, "Polling for write failed: %d\n", errno);
                return -1;
            }
            
            // Timeout?
            if (i_poll_result_write == 0)
            {
                // Kill child
                debug(6, "Child process timed out, killing it.\n");
                kill(pid_child, 0);
                //TODO: Send 501 to http client
                
                return -1;
            }
            
            if (poll_fd_write[0].revents & POLLOUT)
            {   
            debug(6, "Here.\n");
                written_bytes = provideMessageBodyToCGIScript(poll_fd_write[0].fd, cp_http_body, 
                                                          total_written_bytes);                                           
                if(written_bytes < 0)
                {
                    debug(6, "Providing message body to CGI script failed.\n");
                    return -1;
                }
                total_written_bytes += written_bytes;               
                if(total_written_bytes == http_body_length_) 
                {
                    debug(6, "Writing http_body completed.\n");
                    b_write_successful = TRUE;
                }
                
                poll_fd_write[0].revents ^= POLLOUT;
            }
            
            if((poll_fd_write[0].revents & (POLLHUP | POLLERR)) && (!b_write_successful))
            {
                debug(6, "A problem occured on the post body pipe.\n");
                return -1;
            }
        }
        
        i_poll_result_read = poll(poll_fd_read, sizeof(poll_fd_read)/sizeof(poll_fd_read[0]), si_cgi_timeout_);

        if (i_poll_result_read < 0)
        {
            debug(6, "Polling for read failed: %d\n", errno);
            return -1;
        }

        // Timeout?
        if (i_poll_result_read == 0)
        {
            // Kill child
            debug(6, "Child process timed out, killing it.\n");
            kill(pid_child, 0);
            //TODO: Send 501 to http client
            
            return -1;
        }
        if ((poll_fd_read[0].revents & POLLIN) && (!b_read_successful))
        {   
            response_length = drainPipe(poll_fd_read[0].fd, &cp_cgi_response);
            if (response_length < 0)
            {
                debug(6, "Could not read from pipe.\n");
                return -1;
            }
            
            debug(6, "Read %d bytes as CGI response.\n", response_length);
            //TODO: parse
            b_read_successful = TRUE;
            
            poll_fd_read[0].revents ^= POLLIN;
        }

        if((poll_fd_read[0].revents & (POLLHUP | POLLERR)) && (!b_read_successful))
        {
            debug(6, "A problem occured on the cgi response pipe.\n");
            return -1;
        }
    } 
    
    debug(6, "Normal exit.\n");

    return 0;
    
}
*/


ssize_t provideMessageBodyToCGIScript(int i_cgi_post_body_pipe, const char* cp_http_body, 
                                  size_t start_index)
{
    ssize_t written_bytes = 0;
    debug(CGICALL, "Starting write at: %d\n", start_index);
    written_bytes = write(i_cgi_post_body_pipe, cp_http_body + start_index, 
                          http_body_length_ - start_index);

    if(written_bytes < 0)
    {
        debug(CGICALL, "Error writing to CGI stdin: %d\n", errno);
        return -1;
    }
    
    debug(CGICALL, "Wrote %d bytes to CGI stdin.\n", written_bytes);
    
    return written_bytes;
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

