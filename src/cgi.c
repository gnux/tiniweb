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
#include "typedef.h"

static const int MAX_HEADER_SIZE = 8192;
static const int SCI_BUF_SIZE = 256;

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
int setNonblocking(int i_fd)
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
        debugVerbose(CGICALL, "Creating pipes to CGI script failed.\n");
    }
    if (setNonblocking(ia_cgi_response_pipe[0]))
    {
        //TODO safe exit
        debugVerbose(CGICALL, "Setting pipes non-blocking failed.\n");
    }
    
    if(e_used_method == POST)
    {
        if (pipe(ia_cgi_post_body_pipe))
        {
            //TODO: safe exit
            debugVerbose(CGICALL, "Creating pipes to CGI script failed: %d\n", errno);
        }
        
        if (setNonblocking(ia_cgi_post_body_pipe[1]))
        {
            //TODO safe exit
            debugVerbose(CGICALL, "Setting pipes non-blocking failed: %d\n", errno);
        }
        if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        {
            //TODO safe exit
            debugVerbose(CGICALL, "Setting signal handler failed.\n");
        }
        
    }
    
    
    /* Fork the child process */
    pid_child = fork();  

    switch (pid_child) {
        case 0:
            /* We are the child process */
            
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
            
            secCleanup();
            
            i_success = chdir("../cgi-bin/");
            if(i_success == -1)
            {
                //TODO: safe exit
                debugVerbose(CGICALL, "Changing directory failed.\n");
            }
            
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

            // Close the pipes
            closePipes(ia_cgi_post_body_pipe);
            closePipes(ia_cgi_response_pipe);

            // Execute the cgi script
            execv(cp_path, cpa_cgi_args);

            debugVerbose(CGICALL, "Executing CGI script failed.\n");
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
    struct pollfd poll_fd[(e_used_method == POST) ? 2 : 1];
    int i_success = 0;
    int i_poll_result = 0;
    bool b_read_successful = FALSE;
    bool b_write_successful = TRUE;
    FILE* response_stream = NULL;
    
    /* Setup poll_fds for child standard output and 
     * standard error stream */
    poll_fd[0].fd = i_cgi_response_pipe;
    poll_fd[0].events = POLLIN;
    poll_fd[0].revents = 0;
    
    /*
    int fd_temp = 20;
    
    dup2(poll_fd[0].fd, fd_temp);
    
    response_stream = fdopen(fd_temp, "r");
    */
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
    
        if (i_poll_result < 0)
        {
            debugVerbose(CGICALL, "Polling failed: %d\n", errno);
            return -1;
        }
        
        // Timeout?
        if (i_poll_result == 0)
        {
            // Kill child
            debugVerbose(CGICALL, "Child process timed out, killing it.\n");
            kill(pid_child, 0);
            //TODO: Send 501 to http client
            
            return EXIT_FAILURE;
        }
        
        if(e_used_method == POST)
        {
            if(poll_fd[1].revents & (POLLERR))
            {
                debugVerbose(CGICALL, "A problem occured on the cgi post body pipe.\n");
                b_write_successful = TRUE;
            }
            if(poll_fd[1].revents & (POLLHUP))
            {
                debugVerbose(CGICALL, "The other end closed the cgi post body pipe.\n");
                b_write_successful = TRUE;
            }
            
            /* Drain the standard output pipe */
            if ((poll_fd[1].revents & POLLOUT) && (!b_write_successful))
            {   
                i_success = pipeThrough(STDIN_FILENO, poll_fd[1].fd, FALSE, TRUE);
                if (i_success < 0)
                {
                    debugVerbose(CGICALL, "An error occurred while writing.\n");
                    b_write_successful = TRUE;
                    //Writing failed, but that's not so bad. 
                    //Possibly the cgi script terminated without reading the whole body
                }
                else if(i_success == 0)
                {
                    debugVerbose(CGICALL, "Write completed.\n");
                    b_write_successful = TRUE;
                    poll_fd[1].events = 0;
                }
                else
                {   
                    debugVerbose(CGICALL, "Write not completed (yet).\n");
                    poll_fd[1].revents ^= POLLOUT;
                }
            }
        }

        if((poll_fd[0].revents & (POLLERR)) && (!b_read_successful))
        {
            debugVerbose(CGICALL, "A problem occured on the cgi response pipe.\n");
            //return -1;
        }
        
        if((poll_fd[0].revents & (POLLHUP)) && (!b_read_successful))
        {
            debugVerbose(CGICALL, "The other side closed the cgi response pipe.\n");
            //return -1;
        }

        /* Drain the standard output pipe */
        if ((poll_fd[0].revents & POLLIN) && (!b_read_successful))
        {   
            response_stream = getCGIHeaderResponseStream(poll_fd[0].fd);
            http_norm *hpn_info = normalizeHttp(response_stream, TRUE);
            http_cgi_response *http_cgi_response_header = parseCgiResponseHeader(hpn_info);
            
            
            i_success = sendCGIHTTPResponseHeader(http_cgi_response_header);
            if(i_success == EXIT_SUCCESS)
            {
                debugVerbose(CGICALL, "CGI header provided successfully to http client.\n");
            }
            else
            {
                fclose(response_stream);
                debugVerbose(CGICALL, "Error providing cgi response header.\n");
                return EXIT_FAILURE;
            }
            
            if(i_success == EXIT_SUCCESS)
            {                        
                if(e_used_method != HEAD)
                {
                    i_success = writeFileTo(response_stream, STDOUT_FILENO);
                    fclose(response_stream);
                    
                    if(i_success == EXIT_SUCCESS)
                    {
                        debugVerbose(CGICALL, "Body provided successfully to http client.\n");
                    }
                    else
                    {
                        debug(CGICALL, "Error providing cgi response body.\n");
                        return EXIT_FAILURE;
                    }
                    
                    i_success = pipeThrough(poll_fd[0].fd, STDOUT_FILENO, TRUE, FALSE); 
                    
                    
                    if(i_success == EXIT_SUCCESS)
                    {
                        debugVerbose(CGICALL, "Body provided successfully to http client.\n");
                    }
                    else
                    {
                        debugVerbose(CGICALL, "Error providing cgi response body.\n");
                        return EXIT_FAILURE;
                    }
                }
                else
                {
                    fclose(response_stream);
                }
            }
            else
            {
                debugVerbose(CGICALL, "Error while parsing cgi response header.\n");
                return EXIT_FAILURE;
            }
            
            /*
            response_length = drainPipe(poll_fd[0].fd, &cp_cgi_response);
            if (response_length < 0)
            {
                debug(CGICALL, "Could not read from pipe.\n");
                return -1;
            }
            
            debug(CGICALL, "Read %d bytes as CGI response.\n", response_length);
            */
            b_read_successful = TRUE;
            
            poll_fd[0].revents ^= POLLIN;
        } 
    } 
    
    debugVerbose(CGICALL, "Normal exit.\n");

    return EXIT_SUCCESS;
    
}

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

