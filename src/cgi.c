#include <stdlib.h>

#include "cgi.h"
#include "envvar.h"

void processCGIScript(char* cp_filename) 
{

    initEnvVarList("TEST_VARIABLE1", "this is just a test");
    appendToEnvVarList("TEST_VARIABLE2", "this is just another test");
    appendToEnvVarList("TEST_VARIABLE3", "this is just a third test");
}

