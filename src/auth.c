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
#include "secstring.h"
#include "parser.h"
#include "envvar.h"
#include "httpresponse.h"


extern char *scp_secret_;
extern http_autorization* http_autorization_;
extern http_request *http_request_;
extern char *scp_web_dir_;

static const int SCI_NONCE_LEN = 16;
static const int SCI_VALID_NONCE_TIMEOUT = 3600;
static const int SCI_VALID_NONCE_TIME = 3600;

bool authenticate(char* cp_path)
{
    char* cp_ha1 = NULL;
    char* cp_nonce = NULL;
    char* cp_realm = NULL;
    time_t timestamp = time(NULL);

    /**
     * It should be impossible for a client to send an HTTP request with 
     * valid "Authorization"-header without prior reception of an "401 
     * Unauthorized message".
     */

	if(http_autorization_ == NULL)
    {
        
	    debugVerbose(AUTH, "No 401-Unauthorized message was sent before.\n");
	    
	    if (createNonce(&cp_nonce, timestamp) == EXIT_FAILURE)
	    {
            secExit(STATUS_INTERNAL_SERVER_ERROR);
	    }
        
        if (getRealmFromHTDigestFile(cp_path, &cp_realm) == FALSE)
        {
            debugVerbose(AUTH, "ERROR: There is no Realm in the .htdigest file!\n");
            secExit(STATUS_INTERNAL_SERVER_ERROR);
        }

	    if (sendHTTPAuthorizationResponse(cp_realm, cp_nonce) == EXIT_FAILURE)
        {
            // TODO what shall we do with the drunken sailor?
        }
        
        return FALSE;
	}
	else
    {    
	    // If authentication field is available and 401 was already sent -> check if response
	    // is valid
	
	    debugVerbose(AUTH, "The Request contains an authentication field.\n");
	    
	    if (http_autorization_->cp_nonce && http_autorization_->cp_realm &&
	        http_autorization_->cp_response && http_autorization_->cp_uri &&
	        http_autorization_->cp_username && http_request_->cp_path && 
	        http_request_->cp_method && http_request_->cp_uri)
	    {
	    
	        if (getHA1HashFromHTDigestFile(cp_path, http_autorization_->cp_realm, 
	                                       http_autorization_->cp_username, &cp_ha1) == FALSE)
	        {
	            secExit(STATUS_LOGIN_FAILED);
	        }
            
	        if (verifyNonce(http_autorization_->cp_nonce) == FALSE)
            {
                secExit(STATUS_LOGIN_FAILED);
            }
            
            debug(AUTH, "####### HA1: %s\n", cp_ha1);
            debug(AUTH, "####### http_autorization_->cp_nonce: %s\n", http_autorization_->cp_nonce);
            debug(AUTH, "####### http_request_->cp_method: %s\n", http_request_->cp_method);
            debug(AUTH, "####### http_request_->cp_uri: %s\n", http_request_->cp_uri);
            debug(AUTH, "####### http_autorization_->cp_response: %s\n", http_autorization_->cp_response);
	        
	        if (verifyResponse(cp_ha1, http_autorization_->cp_nonce, http_request_->cp_method, 
	            http_request_->cp_uri, http_autorization_->cp_response) == FALSE)
	        {
                debug(AUTH, "The 'response' from the auth field is NOT valid!\n");
	            secExit(STATUS_LOGIN_FAILED);
	        }
	
	        debug(AUTH, "Success: The response from the auth field is valid!\n");
            return TRUE;
	    }
	    else
	    {
	        secExit(STATUS_LOGIN_FAILED);
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
    strAppend(&cp_search_path, "/");
    
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
      
        if (strncmp(cp_htdigest_compare_line, cp_htdigest_line, i_line_len) == 0)
        {
            debugVerbose(AUTH, "Success, expected line found in '.htdigest'-file\n");
            
            cp_ha1 = secMalloc(32 * sizeof(char));
            strncpy(cp_ha1, cp_htdigest_line + i_line_len, 32);
            (*cpp_ha1) = cp_ha1;
            
            secFree(cp_htdigest_compare_line);
            secFree(cp_htdigest_line);
            
            //TODO: This can fail.
            fclose(file_htdigest);
            
            return TRUE;
        }
    }
    
    
    debugVerbose(AUTH, "ERROR, expected line not found in '.htdigest'-file\n");
    
    secFree(cp_htdigest_compare_line);
    secFree(cp_htdigest_line);
    
    //TODO: This can fail.
    fclose(file_htdigest);
    
    return FALSE;
}

bool getRealmFromHTDigestFile(char* cp_path_to_file, char** cpp_realm)
{
    int i_htdigest_line_len = 72;
    char* cp_mode = "r";
    char* cp_htdigest_line = NULL;
    char* cp_realm = NULL;
    bool b_separator_found = FALSE;
    
    FILE* file_htdigest = fopen(cp_path_to_file, cp_mode);

    if (file_htdigest == NULL)
    {
        debugVerbose(AUTH, "ERROR, '.htdigest'-file could not be opened.\n");
        return FALSE;
    }
    
    cp_htdigest_line = secMalloc(i_htdigest_line_len);
    
    while( fgets(cp_htdigest_line, i_htdigest_line_len, file_htdigest) )
    {
        for (int i = 0; i < i_htdigest_line_len; i++)
        {
            if (cp_htdigest_line[i] == ':')
            {
                if (b_separator_found == FALSE)
                {
                    b_separator_found = TRUE;
                }
                else
                {
                    debugVerbose(AUTH, "Success: realm: %s found.\n", cp_realm);
                    secFree(cp_htdigest_line);
                    *cpp_realm = cp_realm;
                    
                    //TODO: This can fail.
                    fclose(file_htdigest);
                    
                    return TRUE;
                }
            }
            
            if (b_separator_found == TRUE && cp_htdigest_line[i] != ':')
            {
                strAppend(&cp_realm, secGetStringPart(cp_htdigest_line, i, i));
            }
        }
        
    }
    
    secFree(cp_htdigest_line);
    
    //TODO: This can fail.
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

bool verifyNonce(char* cp_nonce)
{
    // Set timelimit to one hour
    const int ci_timelimit = 3600;
    
    int i_timestamp_len = 8;
    int i_hash_len = SCI_NONCE_LEN * 2 - 1;
    int i_timestamp_current = time(NULL);
    int i_timestamp_recieved = 0;
    char* cp_timestamp_hex = NULL;
    char* cp_path_hash = NULL;
    char* cp_hmac = NULL;
    char* cp_nonce_calculated = NULL;
    
    debugVerbose(AUTH, "######### string len: %i !\n", strlen(cp_nonce));
    
    if (strlen(cp_nonce) < i_timestamp_len + i_hash_len * 2)
    {
        debugVerbose(AUTH, "ERROR: Nonce length is too small!\n");
        return FALSE;
    }
    cp_timestamp_hex = secGetStringPart(cp_nonce, 0, i_timestamp_len - 1);
    cp_path_hash = secGetStringPart(cp_nonce, i_timestamp_len, i_timestamp_len + i_hash_len);
    cp_hmac = secGetStringPart(cp_nonce, i_timestamp_len + i_hash_len, strlen(cp_nonce) - 1);
    
    debugVerbose(AUTH, "Timestamp Hex: %s \n", cp_timestamp_hex);
    debugVerbose(AUTH, "Path Hash: %s \n", cp_path_hash);
    debugVerbose(AUTH, "HMACMD5 Hash: %s \n", cp_hmac);
    
    // TODO remove this:
    // timestamp: int int verwandeln
    i_timestamp_recieved = (int) strDecodeHexToUInt(cp_timestamp_hex, 0, i_timestamp_len);
    
    if (i_timestamp_recieved + ci_timelimit < i_timestamp_current || i_timestamp_recieved > i_timestamp_current)
    {
        debugVerbose(AUTH, "ERROR: Recieved Timestamp is not valid! (Out of Time)\n", cp_hmac);
        return FALSE;
    }
    
    if (createNonce(&cp_nonce_calculated, i_timestamp_recieved) == EXIT_FAILURE)
    {
        return FALSE;
    }
    
    if (strlen(cp_nonce_calculated) != strlen(cp_nonce) || strncmp(cp_nonce_calculated, cp_nonce, strlen(cp_nonce)) != 0)
    {
        debugVerbose(AUTH, "ERROR: Recieved Nonce: %s does not match calculated nonce: %s\n", cp_nonce, cp_nonce_calculated);
        return FALSE;
    }
    
    debugVerbose(AUTH, "Success: Nonces equal!\n");

    return TRUE;
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

int createNonce(char** cpp_nonce, time_t timestamp)
{	
    char uca_time[9];
    md5_state_t path_state;
    unsigned char uca_path_nonce[SCI_NONCE_LEN + 1];
    char* cp_path_hash = NULL;
    char* cp_concatenated_time_path = NULL;
    unsigned char uca_time_path_hmac[SCI_NONCE_LEN + 1];
    char* cp_time_path_hmac = NULL;
    
    uca_path_nonce[SCI_NONCE_LEN] = '\0';
    uca_time_path_hmac[SCI_NONCE_LEN] = '\0';
    
    debugVerbose(AUTH,"Hex von 14: %x\n",14);
    
    // Creating of the MD5 Hash of the Request Path
    md5_init(&path_state);
    md5_append(&path_state, (unsigned char*)http_request_->cp_path, strlen(http_request_->cp_path));
    md5_finish(&path_state, uca_path_nonce);
    
    debugVerbose(AUTH, "###  uca_path_nonce lenght is: %i\n", strlen((char*)uca_path_nonce));
    
    if (convertHash(uca_path_nonce, SCI_NONCE_LEN, &cp_path_hash) == EXIT_FAILURE)
    {
        debugVerbose(AUTH, "ERROR: Converting of Path Hash did not work!");
        return EXIT_FAILURE;
    }
    
    debugVerbose(AUTH, "Hash of the Path: %s\n", cp_path_hash);
    debugVerbose(AUTH, "Length of hashed Path: %i\n", strlen(cp_path_hash));

    // Convert timestamp to Hex:
    memset(uca_time, 0, 9);
    sprintf((char*)uca_time,"%x",(unsigned int)timestamp);
    debugVerbose(AUTH, "Created the timestamp in Hex: %s\n", uca_time);
    debugVerbose(AUTH, "Length of timestamp hex: %i\n", strlen(uca_time));
    
    /** 
     *  STEP 1:
     *  Concatenate timestamp hex and path hash
     */
    strAppend(&cp_concatenated_time_path, (char*)uca_time);
    strAppend(&cp_concatenated_time_path, cp_path_hash);
    debugVerbose(AUTH, "Concatenation of timestamp hex and Path hash: %s\n", cp_concatenated_time_path);
    debugVerbose(AUTH, "Length of Concatenation of timestamp hex and Path hash: %i\n", strlen(cp_concatenated_time_path));
    
    /** 
     *  STEP 2:
     *  Calculate HMACMD5
     */
    performHMACMD5((unsigned char*)cp_concatenated_time_path, strlen(cp_concatenated_time_path), 
                   (unsigned char*)scp_secret_, strlen(scp_secret_), uca_time_path_hmac);
    if (convertHash(uca_time_path_hmac, SCI_NONCE_LEN, &cp_time_path_hmac) == EXIT_FAILURE)
    {
        debugVerbose(AUTH, "ERROR: Converting of HMACMD5 Hash (Time and Path) did not work!");
        return EXIT_FAILURE;
    }
    debugVerbose(AUTH, "Calculated HMACMD5 of time and Path: %s\n", cp_time_path_hmac);
    debugVerbose(AUTH, "###  Path+Time HMAC lenght is: %i\n", strlen(cp_time_path_hmac));
    /** 
     *  Concatenate STEP 1 and STEP 2:
     */
    strAppend(cpp_nonce, cp_concatenated_time_path);
    strAppend(cpp_nonce, cp_time_path_hmac);
    debugVerbose(AUTH, "Concatenated (time : md5(path) : hmacmd5(time : md5(path))): %s\n", *cpp_nonce);
	
	debugVerbose(AUTH, "###  Nonce lenght is: %i\n", strlen(*cpp_nonce));
    // TODO remove!
    //verifyNonce(*cpp_nonce);
    
    return EXIT_SUCCESS;
}

int convertHash(unsigned char* ucp_hash, int i_hash_len, char** cp_hash_nonce)
{
    int i_result_nonce_len = (i_hash_len * 2) + 1;
    //char* cp_tmp_nonce_container = NULL;
    
    if (i_result_nonce_len < i_hash_len)
        return EXIT_FAILURE;
    
   // cp_tmp_nonce_container = secMalloc(3 * sizeof(char));
    
    for (int i = 0; i < i_hash_len; i++)
    {
        //sprintf(cp_tmp_nonce_container, "%x", ucp_hash[i]);
        //cp_tmp_nonce_container[2] = '\0';
        //strAppend(cp_hash_nonce, cp_tmp_nonce_container);
        if(ucp_hash[i] <= 15)
        {
            strAppendFormatString(cp_hash_nonce,"0%x",ucp_hash[i]);
        }
        else
        {
            strAppendFormatString(cp_hash_nonce,"%x",ucp_hash[i]);
        }
    }
    
    //secFree(cp_tmp_nonce_container);    
    
    return EXIT_SUCCESS;
}

void testHash() 
{
    md5_state_t hash_state;
    unsigned char result[17];
    char* teststring = "TollerText";
    char* resultstring = NULL;
    
    debugVerbose(AUTH, "########## Bin Daaaaa, Wer noch? \n");
    
    md5_init(&hash_state);
    md5_append(&hash_state, (unsigned char*)teststring, strlen(teststring));
    md5_finish(&hash_state, result);
    
    debugVerbose(AUTH, "########## Bin Daaaaa, Wer noch?2 \n");
    
    result[16] = '\0';
    
    convertHash(result, 16, &resultstring);
    debugVerbose(AUTH, "########## HashTest: %s, Length: %i\n", resultstring, strlen(resultstring));
    
}

