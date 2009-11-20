
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "auth.h"
#include "typedef.h"
#include "md5.h"
#include "debug.h"


static bool sb_unauthorized_message_sent = FALSE;

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
        debugVerbose(3, "The response from the client does not match the expected response!");
        return FALSE;
    }
    
    return TRUE;
}

bool unauthorizedMessageSent()
{
    return sb_unauthorized_message_sent;
}

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

void testPerformHMACMD5() 
{
    unsigned char uca_text[] = "BeispieltextHerewouldbethetimestamp";
    unsigned char uca_key[] = "SecretKeyFromCommandLine";
    unsigned char digest[16];
    
    performHMACMD5(uca_text, strlen((char*)uca_text), uca_key, strlen((char*)uca_key), digest);
    createNonce(uca_key, digest);
}

void createNonce(unsigned char* uca_key, unsigned char* uca_nonce)
{
    time_t timestamp = time(NULL);
    int i_text_len = 30;
    unsigned char uca_text[i_text_len];
    
    memset(uca_text, 0, i_text_len);
    sprintf((char*)uca_text,"%s",asctime( localtime(&timestamp) ) );  

    performHMACMD5(uca_text, i_text_len, uca_key, i_text_len, uca_nonce);
    debugVerboseHash(3, uca_nonce, 16, "A Nonce was created!");
}

