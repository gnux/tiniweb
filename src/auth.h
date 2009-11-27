/** tiniweb
 * @file auth.h
 * @author Dieter Ladenhauf
 */

#ifndef __AUTH_H__
#define __AUTH_H__

#include <sys/types.h>
#include "typedef.h"

/**
 * TODO specify everything
 *
 */
bool authenticate(char* cp_path);

bool mapRequestPath(char** cpp_final_path, bool *cb_static);

int searchForHTDigestFile(char* cp_path, bool* bp_digest_file_available, char** cpp_path_to_htdigest_file);

bool getHA1HashFromHTDigestFile(char* cp_path_to_file, char* cp_realm, char* cp_username, char** cpp_ha1);

/**
 * Checks the response from the client, which should be calculated in the following way:
 *
 *   HA1 = md5( username : realm : password )
 *   HA2 = md5( HTTP-Request-Method : URI)
 *   response = md5( HA1 : nonce : HA2 )
 *
 * @param uca_ha1
 * @param uca_nonce Nonce
 * @param uca_http_request_method HTTP Request Method
 * @param uca_uri URI
 * @param uca_response response from the client (this will be checked)
 * @return TRUE if the response was valid, FALSE if not
 */
bool verifyResponse(unsigned char* uca_ha1, unsigned char* uca_nonce,
                    unsigned char* uca_http_request_method,
                    unsigned char* uca_uri, unsigned char* uca_response);

/**
 * Checks if we already sent a 401-Unauthorized Message back. 
 * It should be impossible for a client to send an HTTP request with 
 * valid "Authorization"-header without prior reception of an "401 
 * Unauthorized message".
 */
bool unauthorizedMessageSent();

/**
 * Implementation of HMAC using MD5 from RFC2104 (http://tools.ietf.org/html/rfc2104)
 * 
 * @param uca_text pointer to data stream
 * @param i_text_len length of data stream
 * @param uca_key pointer to authentication key
 * @param i_key_len length of key
 * @param digest caller digest to be filled in
 */
void performHMACMD5(unsigned char* uca_text, int i_text_len, 
                    unsigned char* uca_key, int i_key_len, unsigned char* digest);
                    

/**
 * Creates the nonce for authentication purposes
 *
 * @param uca_key the secret key, providied from the commandline
 * @param uca_nonce nonce to be filled in
 */
void createNonce(unsigned char* uca_key, unsigned char* uca_nonce);




#endif
