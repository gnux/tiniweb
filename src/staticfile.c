/** tiniweb
 * @file httpresponse.c
 * @author Christian Partl
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "staticfile.h"
#include "typedef.h"
#include "debug.h"
#include "parser.h"
#include "httpresponse.h"
 
extern const enum SCE_KNOWN_METHODS e_used_method;

void processStaticFile(const char* ccp_path)
{
    FILE* file = NULL;
    file = fopen(ccp_path, "r");
    char* cp_content_type = NULL;
    
    if(file == NULL)
    {
        debugVerbose(STATIC_FILE, "Opening file %s failed: %d\n", ccp_path, errno);
        //TODO: safe exit
        return;
    }
    
    cp_content_type = parseExtention(ccp_path);

    sendHTTPResponseHeaderExplicit("200 OK", cp_content_type);
    
    if(e_used_method != HEAD)
    {
        writeFileTo(file, STDOUT_FILENO);
    }
    
    fclose(file);
}
 
int writeFileTo(FILE *file, int i_dest_fd)
{
    unsigned char c_char = 0;
    int i_result = 0;
    ssize_t written_bytes = 0;
 
    do 
    {
        // Read data from input pipe
        i_result = fgetc(file);
        if(i_result == EOF)
            return EXIT_SUCCESS;
            
        c_char = (unsigned char)(i_result);
        
        //debug(CGICALL, "Before write.\n");
        written_bytes = write(i_dest_fd, &c_char, 1);
        //debug(CGICALL, "Wrote %d bytes to http client.\n", written_bytes);
        if (written_bytes < 0) 
        {       
            return EXIT_FAILURE;
        } 
    } while (1);

}

