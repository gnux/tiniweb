#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "cgi.h"
#include "envvar.h"
#include "secmem.h"


void processCGIScript(const char* cp_path) 
{
    int i_success = 0;
    int ia_cgi_response_pipe[2] = {-1, -1};
    int ia_cgi_post_body_pipe[2] = {-1, -1};
    pid_t pid_child = 0;
    char* cpa_cgi_args[] = {"testscript", NULL};
    /*
    char* cp_test_env_var_name = NULL;
    char* cp_test_env_var_value = NULL;
    
    cp_test_env_var_name = (char*)secMalloc(5);
    cp_test_env_var_value = (char*)secMalloc(5);
    cp_test_env_var_name[0] = 'A';
    cp_test_env_var_name[1] = 'B';
    cp_test_env_var_name[2] = 'C';
    cp_test_env_var_name[3] = 'D';
    cp_test_env_var_name[4] = '\0';
    
    cp_test_env_var_value[0] = 'g';
    cp_test_env_var_value[1] = 'u';
    cp_test_env_var_value[2] = 'g';
    cp_test_env_var_value[3] = 'u';
    cp_test_env_var_value[4] = '\0';
    */

     
    printEnvVarList();

    initEnvVarList("TEST_VARIABLE1", "this is just a test");
    appendToEnvVarList("TEST_VARIABLE2", "this is just another test");
    appendToEnvVarList("TEST_VARIABLE3", "this is just a third test");
    
    printEnvVarList();
    
    if (pipe(ia_cgi_response_pipe) || pipe(ia_cgi_post_body_pipe))
    {
        //TODO: safe exit
    }

    /* Fork the child process */
    pid_child = fork();  

    switch (pid_child) {
        case 0:
            /* We are the child process */
            fprintf(stderr, "we are the child\n");
            
            i_success = clearenv();
            if(i_success == -1)
            {
                //TODO: safe exit
            }
            i_success = applyEnvVarList();
            if(i_success == -1)
            {
                //TODO: safe exit
            }
            
            secCleanup();
            
            i_success = chdir("../cgi-bin/");
            if(i_success == -1)
            {
                //TODO: safe exit
            }
            fprintf(stderr, "before exec\n");
            
            execv(cp_path, cpa_cgi_args);

            fprintf(stderr, "exec failed, %d, %d\n", errno, ENOENT);
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

