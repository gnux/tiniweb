/** tiniweb
 * @file httpresponse.c
 * @author Christian Partl
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "staticfile.h"
#include "typedef.h"
#include "debug.h"
#include "parser.h"
#include "httpresponse.h"
#include "pipe.h"
#include "secmem.h"

extern int si_cgi_timeout_;
 
extern const enum SCE_KNOWN_METHODS e_used_method;

void processStaticFile(const char* ccp_path)
{
    FILE* file = NULL;
    int i_fd = -1;
    struct stat stat_buffer;
    char* cp_content_type = NULL;
    int i_content_length = 0;
    int i_success = 0;
    
    if(ccp_path == NULL)
    {
        debugVerbose(STATIC_FILE, "Error, no file path specified.\n");
        //TODO: safe exit
        return;
    }
    
    if(lstat(ccp_path, &stat_buffer) < 0)
    {
        debugVerbose(STATIC_FILE, "Opening file %s failed: %d\n", ccp_path, errno);
        //TODO: safe exit
        return;
    }
    
    i_content_length = stat_buffer.st_size;
    
    file = fopen(ccp_path, "r");
        
    if(file == NULL)
    {
        debugVerbose(STATIC_FILE, "Opening file %s failed: %d\n", ccp_path, errno);
        //TODO: safe exit
        return;
    }
    
    i_fd = fileno(file);
    if(i_fd < 0)
    {
        //TODO: safe exit
        return;
    }
    
    cp_content_type = parseExtension(ccp_path);


    //i_success = sendHTTPResponseHeaderExplicit("200 OK", cp_content_type, i_content_length);
    
    if(i_success == EXIT_FAILURE)
    {
        //TODO: safe exit
    }
    
    debugVerbose(STATIC_FILE, "Sent HTTP response header to client.\n");
    
    if(e_used_method != HEAD)
    { 
        i_success = writeFileTo(i_fd, STDOUT_FILENO);
        
        if(i_success == EXIT_FAILURE)
        {
            debugVerbose(STATIC_FILE, "Sending file failed.\n");
            fclose(file);
            //TODO: safe exit
            return;
        }
        
        debugVerbose(STATIC_FILE, "Sent file to client.\n");
    }
    
    if(fclose(file) != 0)
    {
        //TODO: safe exit?
        debugVerbose(STATIC_FILE, "Could not close file.\n");
    }
      
}

int writeFileTo(int i_src_fd, int i_dest_fd)
{
    
    int i_success = -1;
    io_pipe *my_pipe = NULL;
    
    my_pipe = secMalloc(sizeof(io_pipe));

    i_success = initPipe(my_pipe, i_src_fd, STDOUT_FILENO);

    if(i_success == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }
    
    while (1)
    {
    
        i_success = pollPipes(&my_pipe, si_cgi_timeout_, 1);
        
        if(i_success == 1)
        {
            return EXIT_SUCCESS;
        }
        if(i_success == -1)
        {
            return EXIT_FAILURE;
        }
        
        i_success = servePipe(my_pipe);
        
        if(i_success == EXIT_FAILURE)
        {
            return EXIT_FAILURE;
        }

        if((my_pipe->i_in_eof) && (my_pipe->i_out_eof))
        {
            return EXIT_SUCCESS;
        }     
    }
}

/* 
int writeFileTo(FILE *file, int i_dest_fd)
{
    char c_char[2] = {'\0', '\0'};
    int i_result = 0;
    int i_success = 0;
 
    do 
    {
        // Read data from input pipe
        i_result = fgetc(file);
        if(i_result == EOF)
        {
            debugVerbose(STATIC_FILE, "Finished writing file.\n");
            return EXIT_SUCCESS;
        }
            
        c_char[0] = (char)(i_result);
        //debug(STATIC_FILE, "writing %c %d\n", c_char[0], c_char[0]);
        i_success = writeToOutputStream(i_dest_fd, c_char);
        
        if(i_success == EXIT_FAILURE)
        {
            return EXIT_FAILURE;
        }
        
        
        //debug(CGICALL, "Before write.\n");
        written_bytes = write(i_dest_fd, &c_char, 1);
        total_written_bytes += written_bytes;
        //debug(CGICALL, "Wrote %d bytes to http client.\n", written_bytes);
        if (written_bytes < 0) 
        {       
            return EXIT_FAILURE;
        } 
        
    } while (1);

}
*/
