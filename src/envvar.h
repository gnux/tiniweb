/** tiniweb
 * @file secmem.h
 * @author Christian Partl, Dieter Ladenhauf
 */

#ifndef __ENV_VAR_H__
#define __ENV_VAR_H__

typedef struct environment_variable_ {
    
    const char* cp_name;
    const char* cp_value;
    struct environment_variable_* evp_next;
    
} environment_variable;

void setupEnvVarList(const char* ccp_webroot_path, const char* ccp_cgi_filename);

/**
 * Puts a new environment variable into the static env-var list
 *
 * @param cp_name the name of the environment variable
 * @param cp_value the specified calue of the environment value
 * @return -1 in case of an error
 *          0 in case if it worked
 */
int appendToEnvVarList(const char* cp_name, const char* cp_value);

/**
 * Deletes evry element inside of the evn-var list and frees the used memory.
 *
 * @return -1 in case of an error
 *          0 in case if it worked
 */
int deleteEnvVarList();

/**
 * Initializes the first element of the env-var list
 *
 * @param cp_name the name of the environment variable
 * @param cp_value the specified calue of the environment value
 * @return -1 in case of an error
 *          0 in case if it worked
 */
int initEnvVarList(const char* cp_name, const char* cp_value);

/**
 * Sets the environment variables of the current process with the elements inside
 * of the static env-var list.
 *
 * @return -1 in case of an error
 *          0 in case if it worked
 */
int applyEnvVarList();

/**
 * Debug method that prints the content inside of the static env-var list or
 * if the list is empty a message is printed.
 */
void printEnvVarList();

#endif
