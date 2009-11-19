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

extern int si_cgi_timeout_;

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
    int ia_cgi_response_pipe[2] = {-1, -1};
    int ia_cgi_post_body_pipe[2] = {-1, -1};
    pid_t pid_child = 0;
    char* cpa_cgi_args[] = {"testscript", NULL};
    /*
    char* cp_test_env_var_name = NULL;
    char* cp_test_env_var_value = NULL;
    
    cp_test_env_var_name = (char*)secMalloc(5);
    cp_test_env_var_value = (char*)secMalloc(5);
    cp_test_env_var_name[0] = 'A';
    cp_test_env_var_name[1] = 'B';
    cp_test_env_var_name[2] = 'C';
    cp_test_env_var_name[3] = 'D';
    cp_test_env_var_name[4] = '\0';
    
    cp_test_env_var_value[0] = 'g';
    cp_test_env_var_value[1] = 'u';
    cp_test_env_var_value[2] = 'g';
    cp_test_env_var_value[3] = 'u';
    cp_test_env_var_value[4] = '\0';
    */

     
    printEnvVarList();

    initEnvVarList("TEST_VARIABLE1", "this is just a test");
    appendToEnvVarList("TEST_VARIABLE2", "this is just another test");
    appendToEnvVarList("TEST_VARIABLE3", "this is just a third test");
    
    printEnvVarList();
    
    if (pipe(ia_cgi_response_pipe) || pipe(ia_cgi_post_body_pipe))
    {
        //TODO: safe exit
        debug(2, "Creating pipes to CGI script failed.\n");
    }
    if (setNonblocking(ia_cgi_response_pipe[0]))
    {
        //TODO safe exit
        debug(2, "Setting pipes non-blocking failed.\n");
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
                debug(2, "Clearing environment failed.\n");
            }
            i_success = applyEnvVarList();
            if(i_success == -1)
            {
                //TODO: safe exit
                debug(2, "Applying environment variables failed.\n");
            }
            
            secCleanup();
            
            i_success = chdir("../cgi-bin/");
            if(i_success == -1)
            {
                //TODO: safe exit
                debug(2, "Changing directory failed.\n");
            }
            fprintf(stderr, "before exec\n");
            
            
            // Duplicate the pipes to stdIN / stdOUT
            if (dup2(ia_cgi_post_body_pipe[0], STDIN_FILENO) < 0 ||
                dup2(ia_cgi_response_pipe[1], STDOUT_FILENO) < 0)
            {
                //TODO: safe exit
                debug(2, "Duplication pipes failed.\n");
            }

            // Close the pipes
            closePipes(ia_cgi_post_body_pipe);
            closePipes(ia_cgi_response_pipe);

            // Execute the cgi script
            execv(cp_path, cpa_cgi_args);

            debug(2, "Executing CGI script failed.\n");
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
            
            if(cp_http_body != NULL) {
            
                i_success = provideMessageBodyToCGIScript(ia_cgi_post_body_pipe[1], cp_http_body);
                if(i_success == -1)
                {
                    //TODO: safe exit
                    debug(2, "Providin message body to CGI script failed.\n");
                }
            }
            
            i_success = readFromCGIScript(ia_cgi_response_pipe[0], pid_child);
            if(i_success == -1)
            {
                //TODO: safe exit
                debug(2, "Reading response from CGI script failed.\n");
            }
            
            closePipes(ia_cgi_post_body_pipe);
            closePipes(ia_cgi_response_pipe);
            break;
    };  
    debug(2, "Finished processing CGI script.\n");
}

int readFromCGIScript(int i_cgi_response_pipe, pid_t pid_child)
{
    struct pollfd poll_fd[1];
    int i_poll_result = 0;
    int i_response_length = 0;
    char* cp_cgi_response = NULL;
    
    /* Setup poll_fds for child standard output and 
     * standard error stream */
    poll_fd[0].fd = i_cgi_response_pipe;
    poll_fd[0].events = POLLIN;
    poll_fd[0].revents = 0;
    debug(2, "Here.\n");
    while (!(poll_fd->revents & (POLLHUP | POLLERR)))
    {

debug(2, "There.\n");
        // Poll for more events
        i_poll_result = poll(poll_fd, sizeof(poll_fd)/sizeof(poll_fd[0]), si_cgi_timeout_);
        debug(2, "There2.\n");
        if (i_poll_result < 0)
        {
            debug(2, "Polling failed: %d\n", errno);
            // TODO debug output
            return -1;
        }

        // Timeout?
        if (i_poll_result == 0)
        {
            // Kill child
            debug(2, "Child process timed out, killing it.\n");
            kill(pid_child, 0);
            //TODO: Send 501 to http client
            
            return -1;
        }
    debug(2, "There3.\n");
        /* Drain the standard output pipe */
        if (poll_fd[0].revents & POLLIN)
        {   
        debug(2, "There4.\n");
            i_response_length = drainPipe(poll_fd[0].fd, &cp_cgi_response);
            if (i_response_length < 0)
            {
//                 fprintf(stderr, "failed to drain child standard output pipe");
//                 goto err_join_child;
                debug(2, "Could not read from pipe.\n");
                return -1;
            }
            
            debug(2, "Got following CGI response: %s\n", cp_cgi_response);
            //TODO: parse

            poll_fd[0].revents ^= POLLIN;
        }

    } 
    
    return 0;
    
}

int provideMessageBodyToCGIScript(int i_cgi_post_body_pipe, const char* cp_http_body)
{
    size_t written_bytes = 0;
    size_t body_size = strlen(cp_http_body);
    written_bytes = write(i_cgi_post_body_pipe, cp_http_body, body_size);
    if(written_bytes < 0)
        debug(2, "Error writing to CGI stdin: %d\n", errno);
    else
        debug(2, "Wrote %d bytes to CGI stdin.\n", written_bytes);
    return written_bytes;
}

int drainPipe(int i_source_fd, char** cpp_cgi_response) 
{
    int i_total_read_bytes = 0;
    char ca_buffer[SCI_BUF_SIZE];
    bool b_first_iteration = TRUE;
    bool b_eof_reached = FALSE;
 
    do 
    {
        /* Read data from input pipe */
        ssize_t read_bytes;

        read_bytes = read(i_source_fd, ca_buffer, sizeof(ca_buffer));
        debug(2, "There4.\n");
        
        if (read_bytes < 0) 
        {
            debug(2, "Error reading from pipe: %d\n", errno);
            return -1;        
        }

        if(read_bytes < SCI_BUF_SIZE)
        {
            ca_buffer[read_bytes] = '\0';
            read_bytes++;   
            b_eof_reached = TRUE;
        }
        
        i_total_read_bytes += read_bytes;
        
        if (b_first_iteration == TRUE)
        {
            (*cpp_cgi_response) = (char*)secMalloc(i_total_read_bytes);
            strncpy((*cpp_cgi_response), ca_buffer, i_total_read_bytes);
            debug(2, "Read from pipe: %s\n", (*cpp_cgi_response));
            b_first_iteration = FALSE;
        }
        else
        { 
            (*cpp_cgi_response) = (char*) secRealloc((*cpp_cgi_response), i_total_read_bytes);
            strncpy((*cpp_cgi_response) + (i_total_read_bytes - read_bytes), ca_buffer, read_bytes);
            debug(2, "Read from pipe after realloc: %s\n", (*cpp_cgi_response));
            //TODO: There should be a maximum size.
        }

    } while (!b_eof_reached);
    
    return i_total_read_bytes;
}

