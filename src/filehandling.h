/** tiniweb
* @file filehandling.h
* @author Sase Group 03: Plaschzug, Partl, Ladenhauf, Neubauer
*/

#ifndef __FILE_HANDLING_H__
#define __FILE_HANDLING_H__

/**
* This function retrieves an complete header
* @param fd filedescriptor of input file
* @param timeout per byte
*/
char* retrieveHeader(int fd, int timeout);

/**
* This function retrieves num bytes form file, it does polling for each byte
* @param fd filedescriptor of input file
* @param timeout per byte
* @param num number of bytes to read
*/
ssize_t pollAndRead(int fd, int timeout, char* cp_buffer, ssize_t num);

#endif
