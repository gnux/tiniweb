/** tiniweb
 * @file auth.h
 * @author Dieter Ladenhauf
 */

#ifndef __AUTH_H__
#define __AUTH_H__

#include <sys/types.h>
#include "typedef.h"

bool verifyResponse(unsigned char* uca_ha1, unsigned char* uca_nonce, int i_nonce_len,
                    unsigned char* uca_http_request_method, int i_http_request_method_len,
                    unsigned char* uca_uri, int i_uri_len, unsigned char* uca_response);

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
                    
                    
void testPerformHMACMD5();

void createNonce();

void printHash(unsigned char* uca_hash, int i_hash_len);

#endif
