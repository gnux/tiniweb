#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "secmem.h"
#include "secstring.h"
#include "typedef.h"
#include "debug.h"
#include "parser.h"
#include "pipe.h"
#include "normalize.h"
#include <poll.h>

char* secPrint2String(const char* ccp_format, ...)
{
  char* ucp_string = NULL;
  va_list va;
  va_start(va, ccp_format);
  if(vasprintf(&ucp_string, ccp_format, va) == -1 || ucp_string == NULL)
  {
	  secAbort();
  }
  secRegister(ucp_string);
  va_end(va);
  return ucp_string;
}

void *secGetStringPart(const char* ccp_string, ssize_t start, ssize_t end)
{
	if(end < start || end > strlen(ccp_string))
		//TODO: something went wrong we would return NULL
		return NULL;
		//secAbort();
	ssize_t len = end - start + 2;
	ssize_t i;
	char *cp_fragment = secCalloc(len, sizeof(char));
	cp_fragment[len - 1] = '\0';
	for(i=0; i < len - 1; ++i)
	cp_fragment[i] = ccp_string[start + i];
	
	return cp_fragment;
}

ssize_t getNextLineFromString(const char* ccp_string, char** cp_current_line, ssize_t i_offset)
{
	ssize_t i = 0;
	if(*cp_current_line != NULL)
		secFree(*cp_current_line);
	
	for(i = i_offset; i < strlen(ccp_string) && ccp_string[i] != '\n'; ++i);
	*cp_current_line = secGetStringPart(ccp_string, i_offset, i);
	return i + 1;
}

void strAppend(char** cpp_output, const char* ccp_input){
	if(ccp_input == NULL)
	  return;
	if(!*cpp_output){
		*cpp_output = secCalloc(1, sizeof(char));
		(*cpp_output)[0] = '\0';
	}
	
	size_t i_len_input = strlen(ccp_input);
	size_t i_len_output = strlen(*cpp_output);
	size_t i_len_new = i_len_input + i_len_output + 1;
	
	// prevent overflow
	// TODO: search for possible overflows!
	if(i_len_new < i_len_input || i_len_new < i_len_output){
		debugVerbose(NORMALISE, "Error in strAppend possible buffer overflow detected!\n");
		secAbort();
	}
	
	*cpp_output = secRealloc(*cpp_output, i_len_new);
	strncat(*cpp_output, ccp_input, i_len_input);
	
	// just be sure to delimit with '\0'
	(*cpp_output)[i_len_new - 1] = '\0';
}

void strAppendFormatString(char** cpp_output, const char* ccp_format, ...)
{
    char* cp_form = NULL;
	va_list va;
  va_start(va, ccp_format);
  if(vasprintf(&cp_form, ccp_format, va) == -1 || cp_form == NULL)
  {
	  secAbort();
  }
  strAppend(cpp_output, cp_form);
  free(cp_form);
  va_end(va);

}


void stringToUpperCase(char* cp_input){
	//just convert every char from the string to an uppercase char
	for(int i=0; i<strlen(cp_input);i++){
		cp_input[i] = toupper(cp_input[i]);
		if(cp_input[i] == '-')
			cp_input[i] = '_';
	}	
}



long strDecodeHexToUInt(char* cp_string, ssize_t i_offset, ssize_t i_len){
	//If we found an % and the next to chars were hexdigit we decode them
	long result = 0;

	ssize_t i;
	if(i_offset + i_len > strlen(cp_string))
		i_len = strlen(cp_string) - i_offset;
	for(i = 0; i < i_len; ++i)
	{
		result = result << 4;
		result += hextodec(cp_string[i_offset + i]);
	}
//	unsigned char a;
//	unsigned char b;
//	a = cp_string[i_offset + 1];
//	b = cp_string[i_offset + 2];
//	hextodec(&a);
//	hextodec(&b);
	return result;
	//return a*16 + b;
}

char hextodec(char c){
	//calculate the correct char
	if(c>47 && c<58)
		return c - 48;
	else if(c>64 && c<71)
		return c - 55;
	else if(c>96 && c<103)
		return c -= 87;
	return 0;
}

bool isHexDigit(char c){
	//check if the char is in a correct hexdigit range
	if(c>0x29 && c < 0x3A)
		return TRUE;
	else if(c>0x40 && c < 0x47)
		return TRUE;
	else if(c>0x60 && c < 0x67)
		return TRUE;
	else
		return FALSE;
}

// ssize_t secGetlineFromFDWithPollin(char** cpp_lineptr, int fd){
// 	size_t i_num_reads = 0;
// 	ssize_t i_ret = 0;
// 	ssize_t i = 0;
// 	char current;
// 	if(*cpp_lineptr)
// 		secFree(*cpp_lineptr);
// 	
// 	*cpp_lineptr = secCalloc(1, MAX_HEADER_SIZE+1);
// 		
// 	struct pollfd poll_fd;
// 	int i_poll_result;
// 	poll_fd.fd = fd;
// 	poll_fd.events = POLLIN;
// 	poll_fd.revents = 0;
// 	
// 	
// 	for(i=0; i<MAX_HEADER_SIZE + 1; ++i){
// 	 i_poll_result = poll(&poll_fd, 1, PIPE_TIMEOUT);
//      if (i_poll_result == -1) 
//      {
//          debugVerbose(PIPE, "I/O error during poll.\n");
//          //TODO: safe exit
//          return EXIT_FAILURE;
//      } else if (i_poll_result == 0) 
//      {
//          fprintf(stderr, "poll timed out\n");
//          //TODO: safe exit
//          return EXIT_FAILURE;
//      }
//  
// 	/* Evaluate poll */
//     if((poll_fd.revents & ~POLLIN && !(poll_fd.revents & POLLIN)))
// 		break;    
// 	if(!(poll_fd.revents & POLLIN))
// 		break;
// 	
// 	ssize_t in_size = read(fd, &current, 1);
//         if (in_size < 0) 
//         {
//             debugVerbose(PIPE, "I/O error on inbound file.\n");
//             secAbort();
//         }
// 		else if (in_size == 0) 
//         {
//             break;
//         }
// 		if(isValid(&current, 1) == EXIT_FAILURE)
// 			secAbort();
// 		(*cpp_lineptr)[i] = current;
// 		if(current == '\n' || current == '\0' || current == EOF)
// 		{
// 			break;
// 		}
// 	}
// 	
// 	(*cpp_lineptr)[i+1] = '\0';
// 	*cpp_lineptr = secRealloc(*cpp_lineptr, i+2);
// 	
// 		return i;
// }




	//if(read(fd, &(*cpp_lineptr)[i], 1))
	
	
	 
     
//   
//     for(i_index = i_in_idx + 1; i_index <= i_out_idx; i_index++)
//     {
//         int i_pipe_index = ia_io_idx_to_pipe_index_map[i_index];
//         pipes[i_pipe_index]->i_out_eof = pipes[i_pipe_index]->i_out_eof || (poll_fds[i_index].revents & ~POLLOUT);
//         pipes[i_pipe_index]->i_out_ready = pipes[i_pipe_index]->i_out_ready || (poll_fds[i_index].revents & POLLOUT);    
//     }
	
 	//if(fd==NULL || cp_header == NULL)
 	//	return EXIT_FAILURE;
 	//if(len == 0)
 	//	return 0;
	
	
	//if(pollPipes(pipe, PIPE_TIMEOUT, 1));
	
	
	//i_ret = getline(cpp_lineptr, &i_num_reads, stream);
	// proof our input, TODO: BAD request
	//if(i_ret == -1)
	//	secAbort();
	//for(; i < i_ret; ++i)
	//	if(isValid(*cpp_lineptr, i) == EXIT_FAILURE)
	//		secAbort();
	//	secProof(*cpp_lineptr);
	//secRegister(*cpp_lineptr);
	

/*
ssize_t secGetCompleteHeaderDataFromPipe(char** cpp_lineptr, io_pipe* pipe){
	size_t i_num_reads = 0;
	ssize_t i_ret = 0;
	ssize_t i = 0;
	char* cp_header = secCalloc(1, )
	if(*cpp_lineptr)
		secFree(*cpp_lineptr);
	
	*cpp_lineptr = NULL;
	
	
	struct pollfd poll_fd;
	int i_poll_result;
	poll_fd.fd = pipe->i_in_fd;
	poll_fd.events = POLLIN;
	poll_fd.revents = 0;
	
	 i_poll_result = poll(poll_fd, 1, PIPE_TIMEOUT);
     if (i_poll_result == -1) 
     {
         debugVerbose(PIPE, "I/O error during poll.\n");
         //TODO: safe exit
         return EXIT_FAILURE;
     } else if (i_poll_result == 0) 
     {
         fprintf(stderr, "poll timed out\n");
         //TODO: safe exit
         return EXIT_FAILURE;
     }/*
 
     /* Evaluate poll */
//      pipe->i_in_eof = pipe->i_in_eof || 
//                    (poll_fd.revents & ~POLLIN && !(poll_fd.revents & POLLIN));
//      pipe->i_in_ready = pipe->i_in_ready || (poll_fd.revents & POLLIN);
	 
// 	 */
     
//   
//     for(i_index = i_in_idx + 1; i_index <= i_out_idx; i_index++)
//     {
//         int i_pipe_index = ia_io_idx_to_pipe_index_map[i_index];
//         pipes[i_pipe_index]->i_out_eof = pipes[i_pipe_index]->i_out_eof || (poll_fds[i_index].revents & ~POLLOUT);
//         pipes[i_pipe_index]->i_out_ready = pipes[i_pipe_index]->i_out_ready || (poll_fds[i_index].revents & POLLOUT);    
//     }
	
 	//if(fd==NULL || cp_header == NULL)
 	//	return EXIT_FAILURE;
 	//if(len == 0)
 	//	return 0;
	
	
// 	if(pollPipes(pipe, PIPE_TIMEOUT, 1));
// 	
// 	
// 	i_ret = getline(cpp_lineptr, &i_num_reads, stream);
// 	// proof our input, TODO: BAD request
// 	if(i_ret == -1)
// 		secAbort();
// 	for(; i < i_ret; ++i)
// 		if(isValid(*cpp_lineptr, i) == EXIT_FAILURE)
// 			secAbort();
// 		secProof(*cpp_lineptr);
// 	secRegister(*cpp_lineptr);
// 	
// 	return i_ret;
// }*/

