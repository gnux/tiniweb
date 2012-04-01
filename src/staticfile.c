/** 
 * Copyright 2009-2012
 * 
 * Dieter Ladenhauf
 * Georg Neubauer
 * Christian Partl
 * Patrick Plaschzug
 * 
 * This file is part of Wunderwuzzi.
 * 
 * Wunderwuzzi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wunderwuzzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wunderwuzzi. If not, see <http://www.gnu.org/licenses/>.
 * 
 * -------------------------------------------------------------------
 * 
 * tiniweb
 * 
 * @file httpresponse.c
 * @author Christian Partl
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "staticfile.h"
#include "parser.h"
#include "httpresponse.h"
#include "pipe.h"
#include "secmem.h"
#include "debug.h"
#include "typedef.h"

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
        secExit(STATUS_INTERNAL_SERVER_ERROR);
    }
    
    if(lstat(ccp_path, &stat_buffer) < 0)
    {
        debugVerbose(STATIC_FILE, "Opening file %s failed: %d\n", ccp_path, errno);
        secExit(STATUS_INTERNAL_SERVER_ERROR);
    }
    
    i_content_length = stat_buffer.st_size;
    
    file = fopen(ccp_path, "r");
        
    if(file == NULL)
    {
        debugVerbose(STATIC_FILE, "Opening file %s failed: %d\n", ccp_path, errno);
        secExit(STATUS_INTERNAL_SERVER_ERROR);
    }
    
    i_fd = fileno(file);
    if(i_fd < 0)
    {
        debugVerbose(STATIC_FILE, "Retreiving a file descriptor from file pointer failed.\n");
        secExit(STATUS_INTERNAL_SERVER_ERROR);
    }
    
    cp_content_type = parseExtension(ccp_path);


    i_success = sendHTTPResponseHeaderExplicit("200 OK", cp_content_type, i_content_length);
    
    if(i_success == EXIT_FAILURE)
    {
        debugVerbose(STATIC_FILE, "Sending header failed.\n");
        secExit(STATUS_CANCEL);
    }
    
    debugVerbose(STATIC_FILE, "Sent HTTP response header to client.\n");
    
    if(e_used_method != HEAD)
    { 
        i_success = writeFileTo(i_fd, STDOUT_FILENO);
        
        if(i_success == EXIT_FAILURE)
        {
            debugVerbose(STATIC_FILE, "Sending file failed.\n");
            fclose(file);
            secExit(STATUS_CANCEL);
        }
        
        debugVerbose(STATIC_FILE, "Sent file to client.\n");
    }
    
    if(fclose(file) != 0)
    {
        //Everything is finished, here, no need to do something special
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
        if(i_success == 2)
        {
            return EXIT_FAILURE;
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

