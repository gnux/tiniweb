#include <stdlib.h>
#include <unistd.h>

#include "cgi.h"
#include "envvar.h"
#include "secmem.h"

void processCGIScript(const char* cp_path) 
{
    int success = 0;
    pid_t child_pid = 0;
    char* cpa_cgi_args[] = {"cgi_name", NULL};
 
    printEnvVarList();

    initEnvVarList("TEST_VARIABLE1", "this is just a test");
    appendToEnvVarList("TEST_VARIABLE2", "this is just another test");
    appendToEnvVarList("TEST_VARIABLE3", "this is just a third test");
    
    printEnvVarList();

    /* Fork the child process */
    child_pid = fork();

    switch (child_pid) {
        case 0:
            /* We are the child process */
            
            success = clearenv();
            if(success == -1)
            {
                //TODO: safe exit
            }
            success = applyEnvVarList();
            if(success == -1)
            {
                //TODO: safe exit
            }
            
            secCleanup();
            
            success = chdir("path_to_cgi");
            if(success == -1)
            {
                //TODO: safe exit
            }
            
            execv(cp_path, cpa_cgi_args);

            /* Abort child "immediately" with _exit */
            _exit(EXIT_FAILURE);
            break;

        case -1:
            /* Error case */
            //TODO: close pipes

        default:
            /* Successful command execution */

            break;
    };
    
    
    
}

