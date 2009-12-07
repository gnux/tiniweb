/** tiniweb
 * @file staticfile.h
 * @author Christian Partl
 */

#ifndef __STATIC_FILE_H__
#define __STATIC_FILE_H__

/**
 * Sends a HTTP response including the specified static file.
 *
 * @param cp_path Path (including filename) to the file
 */
void processStaticFile(const char* path);

/**
 * Writes the file of a source file descriptor to the destination file descriptor.
 *
 * @param i_src_fd File descriptor of the source
 * @param i_dest_fd File descriptor of the destination
 * @return EXIT_SUCCESS if no problem occurred, EXIT_FAILURE otherwise.
 */
int writeFileTo(int i_src_fd, int i_dest_fd);

#endif

