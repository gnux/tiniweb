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

/**
 * 
 *
 */
int appendToEnvVarList(char* cp_name, char* cp_value);

int deleteEnvVarList();

int initEnvVarList(char* cp_name, char* cp_value);

int applyEnvVarList();

void printEnvVarList();

#endif
