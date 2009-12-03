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
 
extern const enum SCE_KNOWN_METHODS e_used_method;

void processStaticFile(const char* ccp_path)
{
    FILE* file = NULL;
    struct stat stat_buffer;
    char* cp_content_type = NULL;
    int i_content_length = 0;
    
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
    
    cp_content_type = parseExtension(ccp_path);

    sendHTTPResponseHeaderExplicit("200 OK", cp_content_type, i_content_length);
    debugVerbose(STATIC_FILE, "Sent HTTP response to client.\n");
    
    if(e_used_method != HEAD)
    {
        writeFileTo(file, STDOUT_FILENO);
    }
    
    fclose(file);
}
 
int writeFileTo(FILE *file, int i_dest_fd)
{
    //TODO: make non-blocking
    unsigned char c_char = 0;
    int i_result = 0;
    ssize_t written_bytes = 0;
    ssize_t total_written_bytes = 0;
 
    do 
    {
        // Read data from input pipe
        i_result = fgetc(file);
        if(i_result == EOF)
        {
            debugVerbose(STATIC_FILE, "Finished writing file.\n");
            return EXIT_SUCCESS;
        }
            
        c_char = (unsigned char)(i_result);
        
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

