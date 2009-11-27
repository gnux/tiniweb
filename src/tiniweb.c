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
#include "envvar.h"
#include "auth.h"
#include "path.h"

// default values for options, if no command line option is available
//static const char SCCA_WEB_DIR[] = "/";
//static const char SCCA_CGI_DIR[] = "/cgi-bin/";
static const int SCI_CGI_TIMEOUT = 1;

unsigned char sb_flag_verbose_ = FALSE;


char *scp_web_dir_ = NULL;
char *scp_cgi_dir_ = NULL;
char *scp_secret_ = NULL;

/*char *generateHexString(char* ptr, int size){
  int i;
  unsigned char val;
  char *res;
  
  // TODO: error handling!
  if(size*2 < 0)
    abort();
  res = secMalloc(size*2 + 1);
  memset(res, 0, size*2+1);
  for(i=0; i<size; ++i)
  {
    sprintf(&res[i*2],"%x",ptr[i]);
  
  }
  
  for(i=0; i<16; ++i){
    val = ptr[i] & 0x0f;
    if(val<10)
      val+=48;
    else
      val+=55;
    res[i*2] = val;
    val = ptr[i] & 0xf0;
    val>>4;
    if(val<10)
      val+=48;
    else
      val+=55;
    res[i*2+1] = val;
}
  
    
 return res;
  
}*/

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
    // specify flags
    int b_flag_web_dir = 0;
    int b_flag_cgi_dir = 0;
    int b_flag_cgi_timeout = 0;
    int i_option_index = 0;

    // command line parsing: like done in example from
    // http://www.gnu.org/s/libc/manual/html_node/Getopt-Long-Option-Example.html
    while (1) {

        static struct option long_options[] = { { "web-dir", required_argument,
                0, 0 }, { "cgi-dir", required_argument, 0, 1 }, {
                "cgi-timeout", required_argument, 0, 2 },
		{"secret", required_argument, 0, 3},
                { "verbose", no_argument, 0, 4 }, { 0, 0, 0, 0 } };
        i_option_index = 0;
        c = getopt_long(argc, argv, "", long_options, &i_option_index);

        if (c == -1)
            break;

        // TODO: Take arguments, in a secure way, alloc mem and copy!
        //        prove flags
        switch (c) {
        case 0:
            debugVerbose(MAIN, "option web-dir used with argument: %s \n", optarg);
            scp_web_dir_ = secCalloc(strlen(optarg) + 1, sizeof(char));
	    strncpy(scp_web_dir_,optarg, strlen(optarg));
	    b_flag_web_dir = 1;
            break;
        case 1:
            debugVerbose(MAIN, "option cgi-dir used with argument: %s \n", optarg);
	    scp_cgi_dir_ = secCalloc(strlen(optarg) + 1, sizeof(char));
	    strncpy(scp_cgi_dir_, optarg, strlen(optarg));
            b_flag_cgi_dir = 1;
            break;
        case 2:
            debugVerbose(MAIN, "option cgi-timeout used with argument: %s \n",
                    optarg);
		    
	    //TODO: controlledShutdown();
	    si_cgi_timeout_ = (int) strtol(optarg, NULL, 10);
            b_flag_cgi_timeout = 1;
            break;
	case 3:
	    debugVerbose(MAIN, "option secret used with argument: %s \n", optarg);
	    scp_secret_ = secCalloc(strlen(optarg) + 1, sizeof(char));
	    strncpy(scp_secret_, optarg, strlen(optarg));
	    break;
        case 4:
            sb_flag_verbose_ = TRUE;
            debugVerbose(MAIN, "switching to verbose mode \n");
            
            break;
        default:
            debug(MAIN, "encountert unknown argument \n");
	    //TODO: controlledShutdown();
            break;
        }
    }

    // TODO: prove flags, if no arg is given use default-vals

    if(!scp_cgi_dir_ || !scp_web_dir_ || !scp_secret_){
      debug(MAIN, "Mandatory parameter missing\n");
      debug(MAIN, "usage: ./tiniweb --web-dir <path> --cgi-dir <path> --secret <secret> (--cgi-timeout <sec>)\n");
      //TODO: controlledShutdown();
      //TODO: give answer internal server error!
    }
    else if (performPathChecking(&scp_cgi_dir_, &scp_web_dir_) == FALSE)
    {
        debug(MAIN, "ERROR, Paths not valid!\n");
        //TODO: controlledShutdown();
        //TODO: give answer internal server error!
    }
//    if(!b_flag_cgi_timeout)
//      sui_cgi_timeout = SCUI_CGI_TIMEOUT;
    
    if(si_cgi_timeout_ < 1){
      debug(0, "cgi-timeout => 1 required, usind default value (%d)\n", SCI_CGI_TIMEOUT);
      si_cgi_timeout_ = SCI_CGI_TIMEOUT;
    }
	


    debug(MAIN, "Argument parsing finished\n");
    debugVerbose(MAIN, "WEB_DIR = %s \n", scp_web_dir_);
    debugVerbose(MAIN, "CGI_DIR = %s \n", scp_cgi_dir_);
    debugVerbose(MAIN, "SECRET = %s \n", scp_secret_);
    debugVerbose(MAIN, "CGI_TIMEOUT = %d \n", si_cgi_timeout_);
    
	http_norm *hnp_info = normalizeHttp(stdin);
	
	initEnvVarList("GATEWAY_INTERFACE","CGI/1.1");
    //appendToEnvVarList("SCRIPT_FILENAME",scp_cgi_dir_);
	appendToEnvVarList("DOCUMENT_ROOT",scp_web_dir_);
	appendToEnvVarList("SERVER_SOFTWARE","tiniweb/1.0");
	appendToEnvVarList("CONTENT_LENGTH","0");
	//appendToEnvVarList("QUERY_STRING",hnp_info->);
	
	debugVerbose(MAIN, "Normalize finished \n");
	parse(hnp_info);
	debugVerbose(MAIN, "Parsing finished \n"); 

    // I am testing!
    // just to make tcp wrapper happy 
    //    char buf[8192];
    //    int ret = read(STDIN_FILENO, buf, 8192);
    //    fprintf(stderr, "tiniweb: got %d bytes\n", ret);
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

    bool b_static = FALSE;
    char* cp_mapped_path = NULL;
    char* cp_path_to_htdigest_file = NULL;
    bool b_digest_file_available = FALSE;
    
    if (mapRequestPath(&cp_mapped_path, &b_static) == FALSE)
    {
        // TODO safe exit + error to client
    }
    
    if (searchForHTDigestFile(cp_mapped_path, &b_digest_file_available, &cp_path_to_htdigest_file) == EXIT_FAILURE)
    {
        //TODO error, because file is inaccessable and safe exit
    }
    
    if (b_digest_file_available)
    {
        authenticate(cp_path_to_htdigest_file);
    }
    
    processCGIScript("testscript");
    
//     sec_test();
//     sec_test();
//     
//     secMalloc(34);
//     secRealloc(secCalloc(38,55),22);
//     secProof(0);
//     secCleanup();
   
//     processCGIScript("testscript");
  //  testPerformHMACMD5();
    //testPathChecking();
    
//     secCleanup();
    secCleanup();
    
    
    return EXIT_SUCCESS;
}
