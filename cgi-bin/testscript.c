#include <libio.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) 
{
    char* cp_env_var1 = "TEST_VARIABLE1";
    char* cp_env_var2 = "TEST_VARIABLE2";
    char* cp_env_var3 = "TEST_VARIABLE3";
 //   char* cp_env_var4 = "ABCD";
    
    fprintf(stderr, "%s: %s\n", cp_env_var1, getenv(cp_env_var1));
    fprintf(stderr, "%s: %s\n", cp_env_var2, getenv(cp_env_var2));
    fprintf(stderr, "%s: %s\n", cp_env_var3, getenv(cp_env_var3));  
   // fprintf(stderr, "%s: %s\n", cp_env_var4, getenv(cp_env_var4));   
    
    fprintf(stdout, "Hello World!\n");     
}

