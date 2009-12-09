/**
 *
 */

#include <poll.h>
#include <unistd.h>
#include "filehandling.h"
#include "secmem.h"
#include "debug.h"
#include "normalize.h"
#include "httpresponse.h"
#include "typedef.h"

char* retrieveHeader(int fd, int timeout)
{
	char* cp_header = NULL;
	ssize_t i = 0;
	ssize_t in_size = 0;
	char c_current = 0;
	bool b_gotnl = FALSE;
	bool b_gotcr = FALSE;
	
	// first of all allocated our big buffer
	cp_header = secCalloc(MAX_HEADER_SIZE + 1, sizeof(char));
	
	for(i=0; i<MAX_HEADER_SIZE; ++i)
	{
		in_size = pollAndRead(fd, timeout, &cp_header[i], 1);
		// Error from file is never good
		if(in_size == EXIT_FAILURE)
		{
			debugVerbose(FILEHANDLING, "Error reading from file\n");
			secAbort();
		}
		// Header has to end with \n\n
		if(in_size == 0)
		{
			debugVerbose(NORMALISE, "Invalid Header delimiter detected\n");
			secExit(STATUS_BAD_REQUEST);
		}
		// register a \r
		if(cp_header[i] == '\r')
		{
			b_gotcr = TRUE;
			continue;
		}
		// special things to do on \n
		if(cp_header[i] == '\n')
		{
			// replace previous \r by \n
			if(b_gotcr == TRUE)
			{
				cp_header[i-1] = '\n';
				cp_header[i] = '\0';
				--i;
				b_gotcr = FALSE;
			}
			// break up if we had found a previous \n, thats the only right way to get out here
			if(b_gotnl == TRUE)
			{
				++i;
				break;
			}
			b_gotnl = TRUE;
			continue;
		}
		// if we found a non character or a single \r we will break
		if(isValid(cp_header, i) == EXIT_FAILURE || b_gotcr == TRUE)
		{
			debugVerbose(FILEHANDLING, "Detected invalid char while retrieving header\n");
			secAbort();
		}
		b_gotnl = FALSE;
		b_gotcr = FALSE;
	}
	
	// Header is too large
	if(i == MAX_HEADER_SIZE)
		secAbort();
	
	cp_header[i] = '\0';
	return secRealloc(cp_header, i + 1);
}

ssize_t pollAndRead(int fd, int timeout, char* cp_buffer, ssize_t num)
{
	struct pollfd poll_fd;
	ssize_t i = 0;
	ssize_t in_size = 0;
	int i_poll_result = 0;
	poll_fd.fd = fd;
	poll_fd.events = POLLIN;
	poll_fd.revents = 0;
	
	// Poll for every byte
	for(i=0; i<num; ++i)
	{
		i_poll_result = poll(&poll_fd, 1, timeout);
		if (i_poll_result == -1) 
		{
			debugVerbose(FILEHANDLING, "I/O error during poll.\n");
			secAbort();
		} else if (i_poll_result == 0) 
		{
			debugVerbose(FILEHANDLING, "poll timed out\n");
			return EXIT_FAILURE;
		}
		// Evaluate poll
		if((poll_fd.revents & ~POLLIN && !(poll_fd.revents & POLLIN)))
			break;    
		if(!(poll_fd.revents & POLLIN))
			break;
		
		// Read 1 Byte
		in_size= read(fd, &cp_buffer[i], 1);
		if (in_size < 0)
		{
			debugVerbose(FILEHANDLING, "I/O error on inbound file.\n");
			secAbort();
		}
		else if (in_size == 0) 
		{
			break;
		}
	}
	return i;
}
