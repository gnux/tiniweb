/** tiniweb
 * \file secmem.h
 * \author Christian Partl, Dieter Ladenhauf
 */

#ifndef __ENV_VAR_H__
#define __ENV_VAR_H__

typedef struct environment_variable_ {
    
    char* cp_name;
    char* cp_value;
    struct environment_variable_* evp_next;
    
} environment_variable;

static environment_variable* evp_first_element = NULL;

/**
 * 
 *
 */
int appendToEnvVarList(char* cp_name, char* cp_value);

int deleteEnvVarList();

int initEnvVarList(char* cp_name, char* cp_value);

#endif