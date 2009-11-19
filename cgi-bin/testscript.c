#include <libio.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char** argv) 
{
  //  char* cp_env_var1 = "TEST_VARIABLE1";
  //  char* cp_env_var2 = "TEST_VARIABLE2";
    char buf[256];
  //  char* cp_env_var3 = "TEST_VARIABLE3";
    int ret = 0;
 //   char* cp_env_var4 = "ABCD";

    fprintf(stdout, "I say: \n"); 
    
    
   // fprintf(stderr, "TestCGI: %s: %s\n", cp_env_var1, getenv(cp_env_var1));
   // fprintf(stderr, "TestCGI: %s: %s\n", cp_env_var2, getenv(cp_env_var2));
   // fprintf(stderr, "TestCGI: %s: %s\n", cp_env_var3, getenv(cp_env_var3));  

   ret = fscanf(stdin, "%256s", buf);
   if (ret < 0) {
        fprintf(stderr, "TestCGI: Error reading from stdin: %d\n", errno);
    }
    else
        fprintf(stderr, "TestCGI: Read from stdin: %s\n", buf);

     if (!fgets(buf, sizeof(buf), stdin)) {
        fprintf(stderr, "TestCGI: Error reading from stdin: %d\n", errno);
    }
    else
        fprintf(stderr, "TestCGI: Read from stdin: %s\n", buf);
        
 
   // fprintf(stderr, "%s: %s\n", cp_env_var4, getenv(cp_env_var4));   
    
    fprintf(stdout, "Hello World!\n");     
}

