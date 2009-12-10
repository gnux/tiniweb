/** tiniweb
 * \file secmem.c
 * \author Christian Partl, Dieter Ladenhauf
 */

#include <libio.h>
#include <stdlib.h>
#include <stdio.h>

#include "envvar.h"
#include "parser.h"
#include "secmem.h"
#include "secstring.h"
#include "typedef.h"
#include "debug.h"

static const char* SCCP_HTTP_HEADER_FIELD_MARKER = "HTTP_";

static environment_variable* evp_first_element = NULL;

extern http_autorization* http_autorization_;
extern enum SCE_KNOWN_METHODS e_used_method;
extern http_request* http_request_;

void setupEnvVarList(const char* ccp_webroot_path, const char* ccp_cgi_filename, 
                     http_norm* hnp_info, bool b_authenticated_user)
{
    int i_index = 0;
    char* cp_name = NULL;

    initEnvVarList("GATEWAY_INTERFACE", "CGI/1.1");
    appendToEnvVarList("SERVER_PROTOCOL", "HTTP/1.1");
    appendToEnvVarList("REQUEST_METHOD", (e_used_method == GET) ? "GET" : 
                       ((e_used_method == POST) ? "POST" : "HEAD"));
    appendToEnvVarList("SERVER_SOFTWARE", "tiniweb/1.0");
    appendToEnvVarList("CONTENT_LENGTH", "0");
    appendToEnvVarList("QUERY_STRING", (http_request_->cp_query == NULL) ? "" : http_request_->cp_query);
    
    if(b_authenticated_user == TRUE)
    {
        appendToEnvVarList("REMOTE_USER", http_autorization_->cp_username);
    }
    
    appendToEnvVarList("REQUEST_URI", http_request_->cp_uri);
    appendToEnvVarList("SCRIPT_FILENAME", ccp_cgi_filename);
	appendToEnvVarList("DOCUMENT_ROOT", ccp_webroot_path);
	
	for(i_index = 0; i_index < hnp_info->i_num_fields; ++i_index){
		cp_name = NULL;
		strAppend(&cp_name, SCCP_HTTP_HEADER_FIELD_MARKER);
		strAppend(&cp_name, hnp_info->cpp_header_field_name[i_index]);
		stringToUpperCase(cp_name);
		appendToEnvVarList(cp_name,hnp_info->cpp_header_field_body[i_index]);
	}
}

int appendToEnvVarList(const char* cp_name, const char* cp_value)
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

int initEnvVarList(const char* cp_name, const char* cp_value)
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
        debugVerbose(ENVVAR, "Name: %s, Value: %s \n", evp_current_var->cp_name, evp_current_var->cp_value);
        evp_current_var = evp_current_var->evp_next;
    }
}

