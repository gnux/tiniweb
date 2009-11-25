/** tiniweb
 * \file tiniweb.c
 * \author Sase Group 03: Plaschzug, Partl, Ladenhauf, Neubauer
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "md5.h"
#include "secmem.h"
#include "typedef.h"
#include "cgi.h"
#include "debug.h"
#include "parser.h"
#include "normalize.h"

// default values for options, if no command line option is available
static const int SCI_CGI_TIMEOUT = 1;
unsigned char sb_flag_verbose_ = FALSE;
static char *scp_web_dir_ = NULL;
static char *scp_cgi_dir_ = NULL;
int si_cgi_timeout_ = 1000;

  
/** tiniweb main routine
 * \param argc number of commandline arguments
 * \param argv arguments itself
 * \return tiniweb return value
 * \todo extend tiniweb with usefull functionality
 * \bug fix your bugs
 */
int main(int argc, char** argv) {
    int c = 0;
    int i_option_index = 0;


   // char *out = generateHexString(&result[0], 16);
   // fprintf(stderr, "MD5-Test Finished, Result: %s \n", out);
    
    

    // command line parsing: like done in example from
    // http://www.gnu.org/s/libc/manual/html_node/Getopt-Long-Option-Example.html
    while (1) {

        static struct option long_options[] = { { "web-dir", required_argument,
                0, 0 }, { "cgi-dir", required_argument, 0, 1 }, {
                "cgi-timeout", required_argument, 0, 2 },
        // maybe specify verbose options
                { "verbose", no_argument, 0, 3 }, { 0, 0, 0, 0 } };
        i_option_index = 0;
        c = getopt_long(argc, argv, "", long_options, &i_option_index);

        if (c == -1)
            break;

        // TODO: Take arguments, in a secure way, alloc mem and copy!
        //        prove flags
        switch (c) {
        case 0:
            debugVerbose(0, "option web-dir used with argument: %s \n", optarg);
            scp_web_dir_ = secCalloc(strlen(optarg) + 1, sizeof(char));
	    strncpy(scp_web_dir_,optarg, strlen(optarg));
	    break;
        case 1:
            debugVerbose(0, "option cgi-dir used with argument: %s \n", optarg);
	    scp_cgi_dir_ = secCalloc(strlen(optarg) + 1, sizeof(char));
	    strncpy(scp_cgi_dir_, optarg, strlen(optarg));
            break;
        case 2:
            debugVerbose(0, "option cgi-timeout used with argument: %s \n",
                    optarg);
            si_cgi_timeout_ = (int) strtol(optarg, NULL, 10);
            break;
        case 3:
            sb_flag_verbose_ = TRUE;
            debugVerbose(0, "switching to verbose mode \n");
            break;
        default:
            debug(0, "encountert unknown argument \n");
	    //TODO: controlledShutdown();
            break;
        }
    }

    // TODO: prove flags, if no arg is given use default-vals

    if(!scp_cgi_dir_ || !scp_web_dir_){
      debug(0, "Mandatory parameter missing\n");
      debug(0, "usage: ./tiniweb --web-dir <path> --cgi-dir <path> (--cgi-timeout <sec>)\n");
      //TODO: controlledShutdown();
    }
    
    if(si_cgi_timeout_ < 1){
      debug(0, "cgi-timeout => 1 required, usind default value (%d)\n", SCI_CGI_TIMEOUT);
      si_cgi_timeout_ = SCI_CGI_TIMEOUT;
    }
    


    debug(0, "Argument parsing finished\n");
    debugVerbose(0, "WEB_DIR = %s \n", scp_web_dir_);
    debugVerbose(0, "CGI_DIR = %s \n", scp_cgi_dir_);
    debugVerbose(0, "CGI_TIMEOUT = %d \n", si_cgi_timeout_);
    
    normalizeHttp(stdin);

    // I am testing!
    // just to make tcp wrapper happy 
   // 	char buf[8192];
   // 	int ret = read(STDIN_FILENO, buf, 8192);
    //	parse(buf,8192);
   // 	fprintf(stderr, "tiniweb: got %d bytes\n", ret);

		
    // sample response
    //    printf("HTTP/1.1 200 OK\r\n"
    //           "Server: tiniweb/1.0\r\n"
    //           "Connection: close\r\n"
    //           "Content-type: text/html\r\n"
    //           "\r\n"
    //           "<html><body>Hello!</body></html>\r\n");   

    processCGIScript("testscript", "Testbody"); 
    
    secCleanup();
    
    
    return EXIT_SUCCESS;
}

   /* sec_test();
    
    md5_state_t my_md5_state;
    md5_init(&my_md5_state);
    
    const md5_byte_t * username = "testuser";
    const md5_byte_t * realm = "testrealm";
    const md5_byte_t * password = "test";
     
    md5_append(&my_md5_state, username, strlen(username));
    md5_append(&my_md5_state, realm, strlen(realm));
    md5_append(&my_md5_state, password, strlen(password));
    
    md5_byte_t result[16];
    memset(result,0,16);
    md5_finish(&my_md5_state, &result[0]);
*/

