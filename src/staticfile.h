/** tiniweb
 * @file staticfile.h
 * @author Christian Partl
 */

#ifndef __STATIC_FILE_H__
#define __STATIC_FILE_H__

void processStaticFile(const char* path);

int writeFileTo(int i_fd, int i_dest_fd);

#endif

