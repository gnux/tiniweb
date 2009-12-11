/** tiniweb
 * @file secmem.h
 * @author Christian Partl, Dieter Ladenhauf
 */

#ifndef __ENV_VAR_H__
#define __ENV_VAR_H__

#include "typedef.h"

typedef struct environment_variable_ {
    
    const char* cp_name;
    const char* cp_value;
    struct environment_variable_* evp_next;
    
} environment_variable;


/**
 * Creates all environment variables for the cgi script and puts them into the static env-var list
 *
 * @param ccp_webroot_path Absolute filesystem path to webroot
 * @param ccp_cgi_filename Absolute filesystem path to CGI script
 * @param hnp_info Normalized header struct containing all http header fields
 * @param b_authenticated_user Determines whether a user has been authenticated or not
 */
void setupEnvVarList(const char* ccp_webroot_path, const char* ccp_cgi_filename, 
                     http_norm* hnp_info, bool b_authenticated_user);

/**
 * Puts a new environment variable into the static env-var list
 *
 * @param cp_name the name of the environment variable
 * @param cp_value the specified calue of the environment value
 * @return EXIT_FAILURE in case of an error
 *         EXIT_SUCCESS in case if it worked
 */
int appendToEnvVarList(const char* cp_name, const char* cp_value);

/**
 * Deletes evry element inside of the evn-var list and frees the used memory.
 *
 * @return EXIT_FAILURE in case of an error
 *         EXIT_SUCCESS in case if it worked
 */
int deleteEnvVarList();

/**
 * Initializes the first element of the env-var list
 *
 * @param cp_name the name of the environment variable
 * @param cp_value the specified value of the environment value
 */
void initEnvVarList(const char* cp_name, const char* cp_value);

/**
 * Sets the environment variables of the current process with the elements inside
 * of the static env-var list.
 *
 * @return EXIT_FAILURE in case of an error
 *         EXIT_SUCCESS in case if it worked
 */
int applyEnvVarList();

/**
 * Debug method that prints the content inside of the static env-var list or
 * if the list is empty a message is printed.
 */
void printEnvVarList();

#endif
