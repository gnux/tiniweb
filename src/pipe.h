/** tiniweb
 * @file pipe.h
 * @author Christian Partl
 */

#ifndef __PIPE_H__
#define __PIPE_H__

#define MY_PIPE_IOBUF_SIZE 16

typedef struct io_pipe {
    char ca_buffer[MY_PIPE_IOBUF_SIZE];
    size_t valid;
    int i_in_fd;
    int i_in_ready;
    int i_in_eof;
    int i_out_fd;
    int i_out_ready;
    int i_out_eof;
} io_pipe;

int setNonblocking(int i_fd);

int initPipe(io_pipe *pi, int i_in_fd, int i_out_fd);

int pollPipes(io_pipe **pipes, int i_timeout, unsigned int i_num_pipes);

int servePipe(io_pipe *pi);

#endif

