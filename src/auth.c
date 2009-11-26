/** tiniweb
 * @file auth.c
 * @author Dieter Ladenhauf
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "auth.h"
#include "typedef.h"
#include "md5.h"
#include "debug.h"
#include "secmem.h"
#include "normalize.h"

extern char *scp_web_dir_;
extern char *scp_cgi_dir_;
extern char *scp_secret_;

static const int NONCE_LEN = 16;
static const int MAX_ERROR_NUM = 5;
static int i_current_error_num = 0;
static bool sb_unauthorized_message_sent = FALSE;
static unsigned char suca_sent_nonce[16];

void authenticate()
{
    if (checkPath(scp_cgi_dir_) == FALSE ||
        checkPath(scp_web_dir_) == FALSE)
    {
        debug(AUTH, "Something is wrong with the given SGI-BIN- or the WEB-Directory. Terminate Connection!\n");
        // TODO Safe Shutdown
    }
    
    bool b_error = FALSE;
    // TODO check for authentication field
    bool b_auth_field_available = FALSE;
    
    // If no authentication field is available and no unauthorized message (401) was
    // sent, create nonce and send unauthorized message (401)
    if (sb_unauthorized_message_sent == FALSE)
    {
        debugVerbose(AUTH, "No 401-Unauthorized message waas sent before.\n");
        createNonce((unsigned char*)scp_secret_, suca_sent_nonce);
        
        // TODO send 401
    }
    else
    {       
        if (b_auth_field_available == TRUE)
        {
            // If authentication field is available and 401 was already sent -> check if response
            // is valid
        
            debugVerbose(AUTH, "The Request contains an authentication field.\n");
            
            bool b_response_valid = FALSE;
            // TODO: b_response_valid = verifyResponse(...);
        
            if (b_response_valid == FALSE)
            {
                //TODO Send Login Error
                b_error = TRUE;
                i_current_error_num++;
                debug(AUTH, "The 'response' from the auth field is NOT valid!\n");
            }
            else
            {
                debugVerbose(AUTH, "The 'response' from the auth field is valid!\n");
            }
        }
        else
        {
            debugVerbose(AUTH, "The Request contains no authentication field.\n");
            createNonce((unsigned char*)scp_secret_, suca_sent_nonce);
        
            // TODO send 401
            
            b_error = TRUE;
            i_current_error_num++;
        }
    }
    
    if (b_error == TRUE)
    {
        // TODO Free Memory
    }    
    if (i_current_error_num >= MAX_ERROR_NUM)
    {
        debug(AUTH, "The maximum number of errors (%i) within this session was reached. Shutdown of Connection!\n", MAX_ERROR_NUM);
        // TODO Safe Shutdown
    }
}

bool verifyResponse(unsigned char* uca_ha1, unsigned char* uca_nonce, int i_nonce_len,
                    unsigned char* uca_http_request_method, int i_http_request_method_len,
                    unsigned char* uca_uri, int i_uri_len, unsigned char* uca_response)
{
    md5_state_t ha2_state;
    md5_state_t expected_response_state;
    unsigned char uca_ha2[16];
    unsigned char uca_expected_response[16];
    int i_result = -1;
    
    // Calculate HA2:
    md5_init(&ha2_state);
    md5_append(&ha2_state, uca_http_request_method, i_http_request_method_len);
    md5_append(&ha2_state, uca_uri, i_uri_len);
    md5_finish(&ha2_state, uca_ha2);
    
    // Calculate expected response:
    md5_init(&expected_response_state);
    md5_append(&expected_response_state, uca_ha1, 16);
    md5_append(&expected_response_state, uca_nonce, i_nonce_len);
    md5_append(&expected_response_state, uca_ha2, 16);
    md5_finish(&expected_response_state, uca_expected_response);
    
    i_result = strncmp((char*)uca_expected_response, (char*)uca_response, 16);
    if (i_result != 0)
    {
        debugVerbose(AUTH, "The response from the client does not match the expected response!\n");
        return FALSE;
    }
    
    return TRUE;
}

bool unauthorizedMessageSent()
{
    return sb_unauthorized_message_sent;
}

//------------------------------------------------------------------------
// Keyed-Hashing for Message Authentication Code (HMAC)
//
// Implementation from RFC 2104: "Appendix -- Sample Code" [1]
// ... <more descriptive text if needed> ...
//
// [1] RFC2104: http://tools.ietf.org/html/rfc2104#page-8
//
//
// BEGIN-FOREIGN-CODE (RFC2104)
void performHMACMD5(unsigned char* uca_text, int i_text_len, unsigned char* uca_key, 
                    int i_key_len, unsigned char* digest)
{
        md5_state_t context;
        unsigned char uca_k_ipad[65];  /* inner padding -
                                       * key XORd with ipad
                                       */
        unsigned char uca_k_opad[65];  /* outer padding -
                                       * key XORd with opad
                                       */
        unsigned char uca_tk[16];
        int i;
        
        /* if key is longer than 64 bytes reset it to key=MD5(key) */
        if (i_key_len > 64) {

                md5_state_t tctx;

                md5_init(&tctx);
                md5_append(&tctx, uca_key, i_key_len);
                md5_finish(&tctx, uca_tk);

                uca_key = uca_tk;
                i_key_len = 16;
        }

        /*
         * the HMAC_MD5 transform looks like:
         *
         * MD5(K XOR opad, MD5(K XOR ipad, text))
         *
         * where K is an n byte key
         * ipad is the byte 0x36 repeated 64 times
         * opad is the byte 0x5c repeated 64 times
         * and text is the data being protected
         */

        /* start out by storing key in pads */
        bzero( uca_k_ipad, sizeof(uca_k_ipad));
        bzero( uca_k_opad, sizeof(uca_k_opad));
        bcopy( uca_key, uca_k_ipad, i_key_len);
        bcopy( uca_key, uca_k_opad, i_key_len);

        /* XOR key with ipad and opad values */
        for (i = 0; i < 64; i++) {
                uca_k_ipad[i] ^= 0x36;
                uca_k_opad[i] ^= 0x5c;
        }
        
        /*
         * perform inner MD5
         */
        md5_init(&context);                         /* init context for 1st pass */
        md5_append(&context, uca_k_ipad, 64);       /* start with inner pad */
        md5_append(&context, uca_text, i_text_len); /* then text of datagram */
        md5_finish(&context, digest);               /* finish up 1st pass */
        
        /*
         * perform outer MD5
         */
        md5_init(&context);                    /* init context for 2nd pass */
        md5_append(&context, uca_k_opad, 64);  /* start with outer pad */
        md5_append(&context, digest, 16);      /* then results of 1st hash */
        md5_finish(&context, digest);          /* finish up 2nd pass */
}
// END-FOREIGN-CODE (RFC2104)
//------------------------------------------------------------------------

void testPerformHMACMD5() 
{
    unsigned char uca_key[] = "SecretKeyFromCommandLine";
    unsigned char digest[NONCE_LEN];

    createNonce(uca_key, digest);
    checkPath("../cgi-bin");
    checkPath("../cgi-bin/foo");
    checkPath("..");
    checkPath("");
    checkPath("../src/../src/auth.c");
    checkPath("/etc/");
    checkPath("/etc/foooooooo");
    checkPath("/..");
    
    fprintf(stderr, "###########################################\n");
    
    char* cp_path_1 = "/foo/../foo";
    char* cp_path_2 = "/../foo/../foo/././juhu";
    performPathChecking(&cp_path_1,&cp_path_2);
    
    char* cp_path_3 = "../../"; // TODO BUG!!!
    char* cp_path_4 = "/.";
    performPathChecking(&cp_path_3,&cp_path_4);
}

void createNonce(unsigned char* uca_key, unsigned char* uca_nonce)
{
    time_t timestamp = time(NULL);
    int i_text_len = 30;
    unsigned char uca_text[i_text_len];
    
    memset(uca_text, 0, i_text_len);
    sprintf((char*)uca_text,"%s",asctime( localtime(&timestamp) ) );  

    performHMACMD5(uca_text, i_text_len, uca_key, i_text_len, uca_nonce);
    debugVerboseHash(AUTH, uca_nonce, NONCE_LEN, "A Nonce was created!");
}

bool performPathChecking(char** cpp_path_cgi, char** cpp_path_web)
{
    constructAbsolutePath(cpp_path_cgi);
    constructAbsolutePath(cpp_path_web);
    deleteCyclesFromPath(cpp_path_cgi);
    deleteCyclesFromPath(cpp_path_web);
    
    if (checkPath(*cpp_path_cgi) == FALSE || checkPath(*cpp_path_web) == FALSE)
    {
        return FALSE;
    }
    
    if (checkIfCGIDirContainsWebDir(*cpp_path_cgi, *cpp_path_web) == TRUE)
    {
        return FALSE;
    }
    
    debug(AUTH, "Success: Path checking finished! No error ocurred!\n");
    return TRUE;
}

bool checkPath(char* ca_path)
{
    struct stat buffer;
    int i_result = stat(ca_path, &buffer);
    
    if (i_result == 0)
    {
        debugVerbose(AUTH, "Success: Directory Checked: Directory/File %s is valid!\n", ca_path);
        return TRUE;
    }

    debugVerbose(AUTH, "ERROR: Directory Checked: Directory/File %s is NOT valid!\n", ca_path);
    return FALSE;
}

bool checkIfCGIDirContainsWebDir(char* ca_path_cgi, char* ca_path_web)
{
    int i_path_cgi_len = strlen(ca_path_cgi);
    int i_path_web_len = strlen(ca_path_web);
    int i_shortest_path_len = 0;
    int i = 0;
    bool b_paths_do_not_contain_each_other = FALSE;
    
    if (i_path_web_len >= i_path_cgi_len)
    {
        i_shortest_path_len = (i_path_cgi_len > i_path_web_len) ?  i_path_web_len : i_path_cgi_len;
    
        for (i = 0; i < i_shortest_path_len; i++)
        {
            int i_result = strncmp(ca_path_cgi, ca_path_web, i);
            if (i_result != 0)
            {
                b_paths_do_not_contain_each_other = TRUE;
                break;
            }
        }
    }
    
    if (!b_paths_do_not_contain_each_other)
        debugVerbose(AUTH, "Success: CGI-Dir: '%s' does not contain WEB-Dir: '%s'!\n", ca_path_cgi, ca_path_web);
    else
        debugVerbose(AUTH, "ERROR: CGI-Dir: '%s' contains WEB-Dir: '%s'!\n", ca_path_cgi, ca_path_web);
    
    return b_paths_do_not_contain_each_other;
}

void deleteCyclesFromPath(char** cpp_path_to_check)
{
    int i_path_len = strlen(*cpp_path_to_check);
    char** cpp_path = NULL;
    char* cp_result_path = NULL;
    int i_alloc_result_path_len = 0;
    int i_num_folders = 0;
    int i_start = 0;
    
    /**
     *  Store the path into the sorted cpp_path. The result will be e.g:
     *
     *   cpp_path[0] = "Juhu"
     *   cpp_path[0][0] = 'J'
     */
    for (int i_end = 0; i_end < i_path_len; i_end++)
    {
//         debugVerbose(AUTH, "i_end: %i\n", i_end);
        
        if ((*cpp_path_to_check)[i_end] == '/' || i_end == i_path_len - 1)
        {
//             debugVerbose(AUTH, "'/' Found!\n");

            int i_num_chars = i_end - i_start + 1;
//             debugVerbose(AUTH, "  i_num_chars = %i\n", i_num_chars);

            if (i_num_folders == 0)
                cpp_path = (char**) secMalloc((i_num_folders + 1) * sizeof(char*));
            else
                cpp_path = (char**) secRealloc(cpp_path, (i_num_folders + 1) * sizeof(char*));
            
            cpp_path[i_num_folders] = (char*) secMalloc((i_num_chars + 1) * sizeof(char));
            
//             debugVerbose(AUTH, "nach malloc!\n");
            
            // Write all chars into new sorted array
            strncpy(cpp_path[i_num_folders], (*cpp_path_to_check) + i_start, i_num_chars);
            cpp_path[i_num_folders][i_num_chars] = '\0';
//             debugVerbose(AUTH, "Write from A to B: %s\n", cpp_path[i_num_folders]);
            
            i_start = i_end + 1;
            i_num_folders++;
        }
    }
    
    /**
     *  Perform the checking if '../' or './' are in the path
     *   
     *   
     */
    for (int i_current_folder = 0; i_current_folder < i_num_folders; i_current_folder++)
    {
//         for (int i = 0; i < i_num_folders; i++) {
//             debugVerbose(AUTH, "BEFORE: i_current_folder: %i, String: %s\n", i, cpp_path[i]);
//         }
        
        if (strncmp(cpp_path[i_current_folder], "../", 3) == 0 ||
            strncmp(cpp_path[i_current_folder], "..", 2) == 0)
        {
            secRealloc(cpp_path[i_current_folder], 1 * sizeof(char));
            cpp_path[i_current_folder][0] = '\0';
            if (i_current_folder > 0)
            {

                // Check recursive for the last folder
                for (int i_folder_reverse = i_current_folder - 1; i_folder_reverse >= 0; i_folder_reverse--)
                {
                    if (cpp_path[i_folder_reverse][0] != '\0')
                    {
                        secRealloc(cpp_path[i_folder_reverse], 1 * sizeof(char));
                        cpp_path[i_folder_reverse][0] = '\0';
                        break;
                    }
                }
            }
        }
        else if ( strncmp(cpp_path[i_current_folder], "./", 2) == 0 || 
                ( cpp_path[i_current_folder][0] == '.' && cpp_path[i_current_folder][1] == '\0' ) )
        {
            secRealloc(cpp_path[i_current_folder], 1 * sizeof(char));
            cpp_path[i_current_folder][0] = '\0';
        }
        
//         for (int i = 0; i < i_num_folders; i++) {
//             debugVerbose(AUTH, "AFTER:  i_current_folder: %i, String: %s\n", i, cpp_path[i]);
//         }
    }
    
    /**
     *  Store the result into the resultstring.
     */
    for (int i_current_folder = 0; i_current_folder < i_num_folders; i_current_folder++)
    {
        if (cpp_path[i_current_folder][0] != '\0')
        {
            int i_str_len = strlen(cpp_path[i_current_folder]);
            cp_result_path = secRealloc(cp_result_path, i_alloc_result_path_len + i_str_len);
            
            strncpy(cp_result_path + (i_alloc_result_path_len), cpp_path[i_current_folder], i_str_len);
            i_alloc_result_path_len += i_str_len;
        }
        
//         debugVerbose(AUTH, "Currentfolder: %i, Resultstring: %s\n", i_current_folder, cp_result_path);
    }
    cp_result_path[i_alloc_result_path_len] = '\0';
    
    /**
     *  Free Allocated Memory
     */
    for (int i_current_folder = 0; i_current_folder < i_num_folders; i_current_folder++)
        secFree(&(cpp_path[i_current_folder]));
    secFree(cpp_path);
    
    debugVerbose(AUTH, "Removed All cycles from %s\n", (*cpp_path_to_check));
    (*cpp_path_to_check) = cp_result_path;
//     debugVerbose(AUTH, "    Result: %s\n", (*cpp_path_to_check));
}

void getSortedPath(char* cp_path_to_sort, char** cpp_path)
{
    
    
}

bool isAbsolutePath(char * cp_path)
{
    int i_path_len = strlen(cp_path);
    if (i_path_len >= 1)
    {
        if (cp_path[0] == '/')
            return TRUE;
    }
    
    return FALSE;
}

void constructAbsolutePath(char** cpp_path)
{
    if (!isAbsolutePath(*cpp_path))
    {
        char* cp_buffer = NULL;
        int i_buffer_len = 20;
//         int i_path_len = strlen(*cpp_path);
        cp_buffer = secMalloc(i_buffer_len * sizeof(char));
        char* cp_result = getcwd(cp_buffer, i_buffer_len);
        
        while (cp_result == NULL)
        {
            i_buffer_len += 1;
            cp_buffer = secRealloc(cp_buffer, i_buffer_len * sizeof(char));
            cp_result = getcwd(cp_buffer, i_buffer_len);
        }
        
        strAppend(&cp_result, "/");
        strAppend(&cp_result, *cpp_path);
        secFree(cpp_path);
        (*cpp_path) = cp_result;
        
        debugVerbose(AUTH, "Absolute Path %s constructed.\n", (*cpp_path));
        return;
    }
    debugVerbose(AUTH, "Path %s is already an absolute path.\n", (*cpp_path));
}


