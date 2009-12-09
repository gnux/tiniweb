/**
 *
 */

#ifndef __FILE_HANDLING_H__
#define __FILE_HANDLING_H__

//#include <types.h>

char* retrieveHeader(int fd, int timeout);
ssize_t pollAndRead(int fd, int timeout, char* cp_buffer, ssize_t num);

#endif