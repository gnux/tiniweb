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
#include "path.h"


extern char *scp_secret_;
extern http_autorization* http_autorization_;
extern http_request *http_request_;

static const int SCI_NONCE_LEN = 16;
static bool sb_unauthorized_message_sent_ = TRUE; // TODO wieder umsetzen
static char* scp_sent_nonce_ = NULL;

bool authenticate(char* cp_path)
{
    bool b_auth_field_available = FALSE;
    
    if (http_autorization_ != NULL)
        b_auth_field_available = TRUE;
    
    
    // If no authentication field is available and no unauthorized message (401) was
    // sent, create nonce and send unauthorized message (401)
    if (sb_unauthorized_message_sent_ == FALSE)
    {
        debugVerbose(AUTH, "No 401-Unauthorized message waas sent before.\n");
        int i_result_nonce_len = SCI_NONCE_LEN * 2 + 1;
        scp_sent_nonce_ = secMalloc(i_result_nonce_len * sizeof(char));
        
        if (createNonce((unsigned char*)scp_secret_, &scp_sent_nonce_) == EXIT_FAILURE)
        {
            secFree(scp_sent_nonce_);
        }
        
        // TODO send 401
        // Wait for answer
        // Send answer to normalizer/parser
        
        
        
        secFree(scp_sent_nonce_);
    }
    else
    {       
        if (b_auth_field_available == TRUE)
        {
            // If authentication field is available and 401 was already sent -> check if response
            // is valid
        
            debugVerbose(AUTH, "The Request contains an authentication field.\n");
            
            if (http_autorization_->cp_nonce && http_autorization_->cp_realm &&
                http_autorization_->cp_response && http_autorization_->cp_uri &&
                http_autorization_->cp_username && http_request_->cp_path && 
                http_request_->cp_method && http_request_->cp_uri)
            {
            
                char* cp_ha1 = NULL;
                
                if (getHA1HashFromHTDigestFile(cp_path, http_autorization_->cp_realm, 
                                           http_autorization_->cp_username, &cp_ha1) == FALSE)
                {
                    // TODO safe exit
                    // send login error
                    return FALSE;
                }
                
                // TODO: remove THIS ---------------------------------- START --- just for testing!
                int i_result_nonce_len = SCI_NONCE_LEN * 2 + 1;
                scp_sent_nonce_ = secMalloc(i_result_nonce_len * sizeof(char));
                scp_sent_nonce_ = "c4c544b9722671f08465167eebc2d54f";
                // TODO -------------------------------------------- END -----------------------
                
                
                
                if (verifyResponse(cp_ha1, scp_sent_nonce_, http_request_->cp_method, 
                    http_request_->cp_uri, http_autorization_->cp_response) == FALSE)
                {
                    //TODO Send Login Error
                    debug(AUTH, "The 'response' from the auth field is NOT valid!\n");
                    return FALSE;
                }

                debugVerbose(AUTH, "Success: The 'response' from the auth field is valid!\n");
                // TODO free memory!
                
                secFree(scp_sent_nonce_);
            }
            else
            {
                // TODO send Login Error
            }
        }
        else
        {
            debugVerbose(AUTH, "The Request contains no authentication field.\n");
            
            // TODO send Login Error
            
        }
    }
    
    return TRUE;
}

int searchForHTDigestFile(char* cp_path, char* cp_cearch_path_root, bool* bp_digest_file_available, char** cpp_path_to_htdigest_file)
{
    char** cpp_sorted_final_path = NULL;
    char** cpp_search_path_root_tmp = NULL;
    char* cp_search_path = NULL;
    char* cp_search_path_with_ht_file = NULL;
    char* cp_htdigest_filename = ".htdigest";
    bool b_htdigest_file_found = FALSE;
    int i_htdigest_filename_len = strlen(cp_htdigest_filename);
    int i_path_len = strlen(cp_path);
    
    // Is the requested file a '.htdigest'-File?
    if (i_path_len >= i_htdigest_filename_len)
    {
        if (strncmp(cp_path + i_path_len - i_htdigest_filename_len, cp_htdigest_filename, i_htdigest_filename_len) == 0)
        {
            debugVerbose(AUTH, "ERROR, requested ressource is a .htdigest file!\n");
            return EXIT_FAILURE;
        }
    }
    
    const int ci_num_folders = getSortedPath(cp_path, &cpp_sorted_final_path);
    
    // We do need the number of folders of the search root
    const int ci_root_num_folders = getSortedPath(cp_cearch_path_root, &cpp_search_path_root_tmp);
    freeSortedPath(cpp_search_path_root_tmp, ci_root_num_folders);
    
    strAppend(&cp_search_path, cp_cearch_path_root);
    
    for (int i_current_folder = 0; i_current_folder < ci_num_folders; i_current_folder++)
    {

        if (i_current_folder == 0)
        {
            strAppend(&cp_search_path_with_ht_file, cp_search_path);
            strAppend(&cp_search_path_with_ht_file, ".htdigest");
            i_current_folder = ci_root_num_folders -1;
        }
        else
        {
            strAppend(&cp_search_path, cpp_sorted_final_path[i_current_folder]);
            strAppend(&cp_search_path_with_ht_file, cp_search_path);
            strAppend(&cp_search_path_with_ht_file, ".htdigest");
        }
        
        // .htdigest file in path?
        if (checkPath(cp_search_path_with_ht_file))
        {
            
            // Already second htdigest file found?
            if (b_htdigest_file_found == TRUE)
            {
                debugVerbose(AUTH, "ERROR, path contains two .htdigest files! Ressource is inaccassable\n");
                freeSortedPath(cpp_sorted_final_path, ci_num_folders);
                secFree(cp_search_path_with_ht_file);
                secFree(cp_search_path);
                return EXIT_FAILURE;
            }
            b_htdigest_file_found = TRUE;
            
            // Store results int referenced vars:
            *bp_digest_file_available = TRUE;
            strAppend(cpp_path_to_htdigest_file, cp_search_path_with_ht_file);
        }

        secFree(cp_search_path_with_ht_file);
        cp_search_path_with_ht_file = NULL;
    }
    
    freeSortedPath(cpp_sorted_final_path, ci_num_folders);
    secFree(cp_search_path);  
    return EXIT_SUCCESS;
}

bool getHA1HashFromHTDigestFile(char* cp_path_to_file, char* cp_realm, char* cp_username, char** cpp_ha1)
{
    char* cp_mode = "r";
    char* cp_htdigest_line = NULL;
    char* cp_htdigest_compare_line = NULL;
    char* cp_ha1 = NULL;
    int i_line_len = strlen(cp_username) + strlen(cp_realm) + 2;
    int i_htdigest_line_len = i_line_len + 32;
        
    FILE* file_htdigest = fopen(cp_path_to_file, cp_mode);

    if (file_htdigest == NULL)
    {
        debugVerbose(AUTH, "ERROR, '.htdigest'-file could not be opened.\n");
        return FALSE;
    }
    
    cp_htdigest_line = secMalloc(i_htdigest_line_len);
    
    strAppend(&cp_htdigest_compare_line, cp_username);
    strAppend(&cp_htdigest_compare_line, ":");
    strAppend(&cp_htdigest_compare_line, cp_realm);
    strAppend(&cp_htdigest_compare_line, ":");
    
    while( fgets(cp_htdigest_line, i_htdigest_line_len, file_htdigest) )
    {
        //debugVerbose(AUTH, "1: %s, 2: %s\n", cp_htdigest_compare_line, cp_htdigest_line);
        
        if (strncmp(cp_htdigest_compare_line, cp_htdigest_line, i_line_len) == 0)
        {
            debugVerbose(AUTH, "Success, expected line found in '.htdigest'-file\n");
            
            cp_ha1 = secMalloc(32 * sizeof(char));
            strncpy(cp_ha1, cp_htdigest_line + i_line_len, 32);
            *cpp_ha1 = cp_ha1;
            
            secFree(cp_htdigest_compare_line);
            secFree(cp_htdigest_line);
            fclose(file_htdigest);
            
            return TRUE;
        }
    }
    
    
    debugVerbose(AUTH, "ERROR, expected line not found in '.htdigest'-file\n");
    
    secFree(cp_htdigest_compare_line);
    secFree(cp_htdigest_line);
    fclose(file_htdigest);
    
    return FALSE;
}

bool verifyResponse(char* cp_ha1, char* cp_nonce, char* cp_http_request_method,
                    char* cp_uri, char* cp_response)
{
    md5_state_t ha2_state;
    md5_state_t expected_response_state;
    unsigned char uca_ha2[SCI_NONCE_LEN];
    unsigned char uca_expected_response[SCI_NONCE_LEN];
    char* cp_expected_converted_response = NULL;
    int i_result = -1;
    int i_nonce_len = strlen(cp_nonce);
    int i_http_request_method_len = strlen(cp_http_request_method);
    int i_uri_len = strlen(cp_uri);
    
    // Calculate HA2:
    md5_init(&ha2_state);
    md5_append(&ha2_state, (unsigned char*)cp_http_request_method, i_http_request_method_len);
    md5_append(&ha2_state, (unsigned char*)cp_uri, i_uri_len);
    md5_finish(&ha2_state, uca_ha2);
    
    // Calculate expected response:
    md5_init(&expected_response_state);
    md5_append(&expected_response_state, (unsigned char*)cp_ha1, SCI_NONCE_LEN);
    md5_append(&expected_response_state, (unsigned char*)cp_nonce, i_nonce_len);
    md5_append(&expected_response_state, (unsigned char*)uca_ha2, SCI_NONCE_LEN);
    md5_finish(&expected_response_state, uca_expected_response);

    // TODO remove this debug output:
    debugVerbose(AUTH, "Hash from client: %s\n", cp_response);
    debugVerboseHash(AUTH, uca_expected_response, SCI_NONCE_LEN, "Expected Hash   :");
    
    convertHash(uca_expected_response, SCI_NONCE_LEN, &cp_expected_converted_response);
    
    i_result = strncmp(cp_response, cp_expected_converted_response, SCI_NONCE_LEN * 2);
    if (i_result != 0)
    {
        debugVerbose(AUTH, "The response from the client does not match the expected response!\n");
        secFree(cp_expected_converted_response);
        return FALSE;
    }
    
    secFree(cp_expected_converted_response);
    return TRUE;
}

bool unauthorizedMessageSent()
{
    return sb_unauthorized_message_sent_;
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

int createNonce(unsigned char* uca_key, char** cpp_nonce)
{
    time_t timestamp = time(NULL);
    int i_text_len = 30;
    unsigned char uca_text[i_text_len];
    unsigned char uca_nonce[SCI_NONCE_LEN];
    
    memset(uca_text, 0, i_text_len);
    sprintf((char*)uca_text,"%s",asctime( localtime(&timestamp) ) );  

    performHMACMD5(uca_text, i_text_len, uca_key, i_text_len, uca_nonce);
    debugVerboseHash(AUTH, uca_nonce, SCI_NONCE_LEN, "A Nonce was created!");
    
    if (convertHash(uca_nonce, SCI_NONCE_LEN, cpp_nonce) == EXIT_FAILURE)
        return EXIT_FAILURE;
    
    return EXIT_SUCCESS;
}

int convertHash(unsigned char* ucp_hash, int i_hash_len, char** cp_hash_nonce)
{
    int i_result_nonce_len = (i_hash_len * 2) + 1;
    char* cp_tmp_nonce_container = NULL;
    
    if (i_result_nonce_len < i_hash_len)
        return EXIT_FAILURE;
    
    cp_tmp_nonce_container = secMalloc(3 * sizeof(char));
    
    for (int i = 0; i < i_hash_len; i++)
    {
        sprintf(cp_tmp_nonce_container, "%x", ucp_hash[i]);
        cp_tmp_nonce_container[2] = '\0';
        strAppend(cp_hash_nonce, cp_tmp_nonce_container);
    }
    
    secFree(cp_tmp_nonce_container);    
    
    return EXIT_SUCCESS;
}


