/** tiniweb
 * @file staticfile.h
 * @author Christian Partl
 */

#ifndef __STATIC_FILE_H__
#define __STATIC_FILE_H__

void processStaticFile(const char* path);

int writeFileTo(FILE *file, int i_dest_fd);

#endif

