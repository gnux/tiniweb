/** tiniweb
 * \file secmem.c
 * \author Christian Partl, Dieter Ladenhauf
 */

#include <libio.h>

#include "envvar.h"
#include "secmem.h"
#include "typedef.h"


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
    
    evp_new_var = (environment_variable*)sec_malloc(sizeof(environment_variable));
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
    
    while (evp_var->evp_next)
    {
        evp_var_next = evp_var->evp_next;
        sec_free((void*)evp_var->cp_name);
        sec_free((void*)evp_var->cp_value);
        sec_free((void*)evp_var);
        evp_var = evp_var_next;
    }
    
    evp_first_element = NULL;
    
    return 0;
}

int initEnvVarList(char* cp_name, char* cp_value)
{
    evp_first_element = (environment_variable*)sec_malloc(sizeof(environment_variable));
    evp_first_element->cp_name = cp_name;
    evp_first_element->cp_value = cp_value;
    evp_first_element->evp_next = NULL;
    
    return 0;
}

