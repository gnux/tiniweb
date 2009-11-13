/** tiniweb
 * \file secmem.c
 * \author Christian Partl, Dieter Ladenhauf
 */

#include <libio.h>
#include <stdlib.h>
#include <stdio.h>

#include "envvar.h"
#include "secmem.h"
#include "typedef.h"

static environment_variable* evp_first_element = NULL;

int appendToEnvVarList(char* cp_name, char* cp_value)
{
    environment_variable* evp_var = evp_first_element;
    environment_variable* evp_new_var = NULL;
    
    if (evp_var == NULL || cp_name == NULL || cp_value == NULL)
        return -1;
    
    while (evp_var->evp_next)
    {
        evp_var = evp_var->evp_next;
    }
    
    evp_new_var = (environment_variable*)secMalloc(sizeof(environment_variable));
    evp_new_var->cp_name = cp_name;
    evp_new_var->cp_value = cp_value;
    evp_new_var->evp_next = NULL;
    evp_var->evp_next = evp_new_var;
    
    return 0;    
}

int deleteEnvVarList()
{
    environment_variable* evp_var = evp_first_element;
    environment_variable* evp_var_next = evp_var->evp_next;
    
    if (evp_first_element == NULL)
        return -1;
    
    while (evp_var)
    {
        evp_var_next = evp_var->evp_next;
        secFree((void*)evp_var->cp_name);
        secFree((void*)evp_var->cp_value);
        secFree((void*)evp_var);
        evp_var = evp_var_next;
    }
    
    evp_first_element = NULL;
    
    return 0;
}

int initEnvVarList(char* cp_name, char* cp_value)
{
    evp_first_element = (environment_variable*)secMalloc(sizeof(environment_variable));
    evp_first_element->cp_name = cp_name;
    evp_first_element->cp_value = cp_value;
    evp_first_element->evp_next = NULL;
    
    return 0;
}

int applyEnvVarList()
{
    environment_variable* evp_current_var = NULL;
    int success = 0;
    if(evp_first_element == NULL)
        return -1;
    
    evp_current_var = evp_first_element;
    
    while(evp_current_var)
    {
        success = setenv(evp_current_var->cp_name, evp_current_var->cp_value, 1);
        
        if(success == -1)
        {
            //TODO: safe exit
        }
        evp_current_var = evp_current_var->evp_next;
    }
    
    return 0;
}

void printEnvVarList()
{
    environment_variable* evp_current_var = NULL;
    if(evp_first_element == NULL) 
    {
        fprintf(stderr, "Envvar list is NULL\n");
        return;
    }
    
    evp_current_var = evp_first_element;
    
    while(evp_current_var)
    {
        fprintf(stderr, "Name: %s, Value: %s \n", evp_current_var->cp_name, evp_current_var->cp_value);
        evp_current_var = evp_current_var->evp_next;
    }
}

