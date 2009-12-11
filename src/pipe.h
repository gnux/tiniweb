/** tiniweb
 * @file pipe.h
 * @author Christian Partl
 */

#ifndef __PIPE_H__
#define __PIPE_H__

#define PIPE_IOBUF_SIZE 16

typedef struct io_pipe {
    char ca_buffer[PIPE_IOBUF_SIZE];
    size_t valid;
    int i_in_fd;
    int i_in_ready;
    int i_in_eof;
    int i_out_fd;
    int i_out_ready;
    int i_out_eof;
} io_pipe;

/**
 * Sets the specified file descriptor non-blocking.
 *
 * @param i_fd File descriptor that shll be set non-blocking.
 * @return The return value of fcntl on success, -1 on error.
 */
int setNonblocking(int i_fd);

/**
 * Initializes an io_pipe.
 *
 * @param io_pipe io_pipe that shall be initialized.
 * @param i_in_fd File despcriptor of the incoming end for the io_pipe.
 * @param i_out_fd ip_pipe File despcriptor of the outgoing end for the io_pipe.
 * @return EXIT_SUCCESS if no problem occurred, EXIT_FAILURE otherwise.
 */
int initPipe(io_pipe *pi, int i_in_fd, int i_out_fd);

/**
 * Performs polling on an array of specified pipes and modifies the pipes accordingly.
 *
 * @param pipes array of pipes that shall be used for polling.
 * @param i_timeout Polling timeout.
 * @param i_num_pipes Number of pipes stored in the pipe array.
 * @return 0, if everything went ok, 1, if no pipe could be polled on, 
 *         2, on timeout and -1 on error.
 */
int pollPipes(io_pipe **pipes, int i_timeout, unsigned int i_num_pipes);

/**
 * Reads PIPE_IOBUF_SIZE bytes from the read end of a pipe (if possible) and writes it 
 * to its write end (if possible).
 *
 * @param pi Pipe that shall be served.
 * @return EXIT_SUCCESS if no problem occurred, EXIT_FAILURE otherwise.
 */
int servePipe(io_pipe *pi);

/**
 * Helper method that prints all properties of the secified pipe.
 *
 * @param pi Pipe whose properties shall be printed.
 * @param ccp_caption Arbitrary string for identifying the pipe.
 */
void printPipe(io_pipe *pi, const char* ccp_caption);

#endif

