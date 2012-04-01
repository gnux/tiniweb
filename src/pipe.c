/** 
 * Copyright 2009-2012
 * 
 * Dieter Ladenhauf
 * Georg Neubauer
 * Christian Partl
 * Patrick Plaschzug
 * 
 * This file is part of Wunderwuzzi.
 * 
 * Wunderwuzzi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wunderwuzzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wunderwuzzi. If not, see <http://www.gnu.org/licenses/>.
 * 
 * -------------------------------------------------------------------
 * 
 * tiniweb
 * 
 * @file pipe.c
 * @author Christian Partl
 */
#include <stdlib.h> 
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
 
#include "pipe.h"
#include "typedef.h"
#include "debug.h"

int setNonblocking(int i_fd)
{
    int old_mode = fcntl(i_fd, F_GETFL);
    if (old_mode < 0) 
    {
        return -1;
    }     

    return fcntl(i_fd, F_SETFL, old_mode | O_NONBLOCK);
}

 
int initPipe(io_pipe *pi, int i_in_fd, int i_out_fd)
{
    pi->i_in_fd = i_in_fd;
    pi->i_out_fd = i_out_fd;
    pi->i_in_ready = 0;
    pi->i_in_eof = 0;
    pi->i_out_ready = 0;
    pi->i_out_eof = 0;
    pi->valid = 0;
    
    if (setNonblocking(i_in_fd) != 0 || setNonblocking(i_out_fd) != 0) 
    {
        debugVerbose(PIPE, "Setting pipes non-blocking failed.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int servePipe(io_pipe *pi)
{
    size_t free_buffer_size = sizeof(pi->ca_buffer) - pi->valid;

    /* Refill the pipe buffer from input end */
    if (pi->i_in_ready && free_buffer_size > 0) 
    {
        ssize_t in_size = read(pi->i_in_fd, pi->ca_buffer + pi->valid, free_buffer_size);
        pi->i_in_ready = 0;   

        if (in_size < 0) 
        {
            debugVerbose(PIPE, "I/O error on inbound file.\n");
            return EXIT_FAILURE;

        } else if (in_size == 0) 
        {
            /* Input end EOF */
            pi->i_in_eof = 1;
            
        } else 
        {
            /* Got data */
            pi->valid += in_size;
        }
    }

    /* Drain the pipe buffer to output end */
    if (pi->i_out_ready && pi->valid > 0) 
    {
        ssize_t out_size = write(pi->i_out_fd, pi->ca_buffer, pi->valid);
        pi->i_out_ready = 0;

        if (out_size < 0) 
        {
            if(errno != EAGAIN)
            {
                debugVerbose(PIPE, "I/O error on outbound file.\n");
                return EXIT_FAILURE;
            }

        } else if (out_size == 0) 
        {
            /* EOF on output pipe */
            pi->i_out_eof = 1;

        } else 
        {
            /* Mark cleared */
            pi->valid -= out_size;

            /* Shift the pipe buffer (when there is remaining data) */
            if (pi->valid > 0) 
            {
                memmove(pi->ca_buffer, pi->ca_buffer + out_size, pi->valid); 
            }
        }
    }
    
    return EXIT_SUCCESS;
}

int pollPipes(io_pipe *pipes[], int i_timeout, unsigned int i_num_pipes)
{
    struct pollfd poll_fds[i_num_pipes * 2];
    int i_in_idx = -1;
    int i_out_idx = -1;
    int i_num_polls = 0;
    int i_poll_result = 0;
    int i_index = 0;
    int ia_io_idx_to_pipe_index_map[i_num_pipes * 2];
    
    if(pipes == NULL)
    {
        return 0;
    }
    
    for(i_index = 0; i_index < i_num_pipes; i_index++)
    {
            /* Setup for poll */
        if ((!pipes[i_index]->i_in_eof) && (!pipes[i_index]->i_in_ready))
        {
            i_in_idx = i_num_polls++;
            poll_fds[i_in_idx].fd = pipes[i_index]->i_in_fd;
            poll_fds[i_in_idx].events = POLLIN;
            poll_fds[i_in_idx].revents = 0;
            ia_io_idx_to_pipe_index_map[i_in_idx] = i_index;
        }      
    }
    
    for(i_index = 0; i_index < i_num_pipes; i_index++)
    {
        if (!pipes[i_index]->i_out_eof && !pipes[i_index]->i_out_ready) 
        {
            i_out_idx = i_num_polls++;
            poll_fds[i_out_idx].fd = pipes[i_index]->i_out_fd;
            poll_fds[i_out_idx].events = POLLOUT;
            poll_fds[i_out_idx].revents = 0;
            ia_io_idx_to_pipe_index_map[i_out_idx] = i_index;
        } 
    } 

    /* Nothing to poll? */
    if (!i_num_polls) 
    {
        debugVerbose(PIPE, "nothing to poll.\n");
        return 1;
    }

    /* poll */
    i_poll_result = poll(poll_fds, i_num_polls, i_timeout);
    if (i_poll_result == -1) 
    {
        debugVerbose(PIPE, "I/O error during poll.\n");
        return -1;

    } else if (i_poll_result == 0) 
    {
        debugVerbose(PIPE, "poll timed out: %d\n", i_num_polls);
        return 2;
    }

    /* Evaluate poll */
    for(i_index = 0; i_index <= i_in_idx; i_index++)
    {
        int i_pipe_index = ia_io_idx_to_pipe_index_map[i_index];
        //debugVerbose(PIPE, "%i POLLIN: %d, POLLHUP: %d, POLLERR: %d\n", i_index, poll_fds[i_index].revents & POLLIN, 
        //             poll_fds[i_index].revents & POLLHUP, poll_fds[i_index].revents & POLLERR);
        pipes[i_pipe_index]->i_in_eof = pipes[i_pipe_index]->i_in_eof || 
                                        (poll_fds[i_index].revents & ~POLLIN && !(poll_fds[i_index].revents & POLLIN));
        pipes[i_pipe_index]->i_in_ready = pipes[i_pipe_index]->i_in_ready || (poll_fds[i_index].revents & POLLIN);
    }
  
    for(i_index = i_in_idx + 1; i_index <= i_out_idx; i_index++)
    {
        int i_pipe_index = ia_io_idx_to_pipe_index_map[i_index];
        pipes[i_pipe_index]->i_out_eof = pipes[i_pipe_index]->i_out_eof || (poll_fds[i_index].revents & ~POLLOUT);
        pipes[i_pipe_index]->i_out_ready = pipes[i_pipe_index]->i_out_ready || (poll_fds[i_index].revents & POLLOUT);        
        if(pipes[i_pipe_index]->i_out_eof)
        {
            pipes[i_pipe_index]->i_out_ready = 0;
        }
    }

    return 0;
}

void printPipe(io_pipe *pi, const char* ccp_caption)
{
    debugVerbose(PIPE, "%s\n:", ccp_caption);
    debugVerbose(PIPE, "IN: fd: %d, ready: %d, eof: %d\n", pi->i_in_fd, pi->i_in_ready, pi->i_in_eof);
    debugVerbose(PIPE, "OUT: fd: %d, ready: %d, eof: %d\n", pi->i_out_fd, pi->i_out_ready, pi->i_out_eof);
    debugVerbose(PIPE, "valid: %d\n\n", pi->valid);
    
}


