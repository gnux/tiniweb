/** tiniweb
 * @file auth.h
 * @author Dieter Ladenhauf
 */

#ifndef __AUTH_H__
#define __AUTH_H__

#include <sys/types.h>
#include "typedef.h"

/**
 * Performs the authentication
 *
 * @param cp_path the path to the .htdigest file
 * @return TRUE if the authentication worked, FALSE if not.
 */
bool authenticate(char* cp_path);

/**
 * Searches for a .htdigest file within the mapped path
 *
 * @param cp_path mapped path
 * @param b_static does the path map the static or the dynamic ressources
 * @param bp_digest_file_available boolean to be stored in. Shows if a .htdigest file was found
 * @param cpp_path_to_htdigest_file path of the htdigest file to be stored in
 * @return EXIT_FAILURE in cause of an error (or if two .htdigest files were found), otherwise EXIT_SUCCESS
 */
int searchForHTDigestFile(char* cp_path, char* cp_cearch_path_root, bool* bp_digest_file_available, char** cpp_path_to_htdigest_file);

/**
 * Searches for the HA1 String in the .htdigest file
 *
 * @param cp_path_to_file path to the .htdigest file
 * @param cp_realm realm of the user
 * @param cp_username username of the user
 * @param cpp_ha1 ha1 sting to be stored in
 * @return TRUE if the HA1 string was found, FALSE in case of an error
 */
bool getHA1HashFromHTDigestFile(char* cp_path_to_file, char* cp_realm, char* cp_username, char** cpp_ha1);

/**
 * Searches for a realm in the .htdigest file
 *
 * @param cp_path_to_file path to the .htdigest file
 * @param cpp_realm realm sting to be stored in
 * @return TRUE if the realm string was found, FALSE in case of an error
 */
bool getRealmFromHTDigestFile(char* cp_path_to_file, char** cpp_realm);

/**
 * Checks the response from the client, which should be calculated in the following way:
 *
 *   HA1 = md5( username : realm : password )
 *   HA2 = md5( HTTP-Request-Method : URI)
 *   response = md5( HA1 : nonce : HA2 )
 *
 * @param cp_ha1
 * @param cp_nonce Nonce
 * @param cp_http_request_method HTTP Request Method
 * @param cp_uri URI
 * @param cp_response response from the client (this will be checked)
 * @return TRUE if the response was valid, FALSE if not
 */
bool verifyResponse(char* cp_ha1, char* cp_nonce, char* cp_http_request_method,
                    char* cp_uri, char* cp_response);

/**
 * Verifies the given nonce
 * 
 * @param cp_nonce the nonce to verify
 * @return TRUE if the nonce is valid, FALSE if not
 */
bool verifyNonce(char* cp_nonce);

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
 * @param cpp_nonce nonce to be filled in
 * @param timestamp timestamp to create the nonce with
 */
int createNonce(char** cpp_nonce, unsigned long timestamp);

/**
 * Converts the hash from an unsigned char to a readable string
 *
 * @param ucp_hash hash to be converted
 * @param i_hash_len hash length
 * @param cp_hash_nonce readable hast to be stored in
 * @return EXIT_SUCCESS if everything worked, EXIT_FAILURE if not
 */
int convertHash(unsigned char* ucp_hash, int i_hash_len, char** cp_hash_nonce);

#endif
