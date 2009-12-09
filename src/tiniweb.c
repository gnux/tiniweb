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
#include <time.h>

#include "md5.h"
#include "secmem.h"
#include "cgi.h"
#include "debug.h"
#include "parser.h"
#include "normalize.h"
#include "envvar.h"
#include "auth.h"
#include "path.h"
#include "httpresponse.h"
#include "staticfile.h"
#include "pipe.h"
#include "filehandling.h"
#include "typedef.h"

// default values for options, if no command line option is available
//static const char SCCA_WEB_DIR[] = "/";
//static const char SCCA_CGI_DIR[] = "/cgi-bin/";
//static const int SCI_CGI_TIMEOUT = 1;

bool b_flag_verbose_ = FALSE;


char *scp_web_dir_ = NULL;
char *scp_cgi_dir_ = NULL;
char *scp_secret_ = NULL;

int si_cgi_timeout_ = CGI_TIME_OUT_MIN;

  
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
            b_flag_verbose_ = TRUE;
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
      debug(MAIN, "usage: ./tiniweb (--verbose) --web-dir <path> --cgi-dir <path> --secret <secret> (--cgi-timeout <msec>)\n");
	  secExit(-1);
	  
	  //TODO: controlledShutdown();
      //TODO: give answer internal server error! -> no answer!
    }
    if (performPathChecking(&scp_cgi_dir_, &scp_web_dir_) == FALSE)
    {
        debug(MAIN, "ERROR, Paths not valid!\n");
        secAbort();
    }
//    if(!b_flag_cgi_timeout)
//      sui_cgi_timeout = SCUI_CGI_TIMEOUT;
    
    if(si_cgi_timeout_ < CGI_TIME_OUT_MIN)
	{
      debug(0, "cgi-timeout is too short, using default value (%d)\n", CGI_TIME_OUT_MIN);
      si_cgi_timeout_ = CGI_TIME_OUT_MIN;
    }
	else if(si_cgi_timeout_ > CGI_TIME_OUT_MAX)
	{
      debug(0, "cgi-timeout is too long, using default value (%d)\n", CGI_TIME_OUT_MAX);
      si_cgi_timeout_ = CGI_TIME_OUT_MAX;
	}
	


    debug(MAIN, "Argument parsing finished\n");
    debugVerbose(MAIN, "WEB_DIR = %s \n", scp_web_dir_);
    debugVerbose(MAIN, "CGI_DIR = %s \n", scp_cgi_dir_);
    debugVerbose(MAIN, "SECRET = %s \n", scp_secret_);
    debugVerbose(MAIN, "CGI_TIMEOUT = %d \n", si_cgi_timeout_);
	
	
	debugVerbose(MAIN, "Switching stdin to non_blocking mode\n");
	if(setNonblocking(STDIN_FILENO))
	{
		debugVerbose(MAIN, "Switching stdin to non_blocking mode failed!\n");
		secAbort();
	}
	
	char* cp_header = retrieveHeader(STDIN_FILENO, STDIN_TIMEOUT);
	// TODO: EXIT WITHOUT ANY MESSAGE
	if(cp_header == NULL)
	{
		debugVerbose(MAIN,"STDIN has to talk to use! We don't like DoS\n");
		secCleanup();
		exit(0);
	}

	http_norm *hnp_info = normalizeHttp(cp_header, FALSE);
	
	initEnvVarList("GATEWAY_INTERFACE","CGI/1.1");
    //appendToEnvVarList("SCRIPT_FILENAME",scp_cgi_dir_);
	appendToEnvVarList("DOCUMENT_ROOT",scp_web_dir_);
	appendToEnvVarList("SERVER_SOFTWARE","tiniweb/1.0");
	appendToEnvVarList("CONTENT_LENGTH","0");
	//appendToEnvVarList("QUERY_STRING",hnp_info->);
	
	debugVerbose(MAIN, "Normalize finished \n");
	parse(hnp_info);
	debugVerbose(MAIN, "Parsing finished \n"); 

    bool b_static = FALSE;
    char* cp_mapped_path = NULL;
    char* cp_path_to_htdigest_file = NULL;
    char* cp_search_path_root = NULL;
    bool b_digest_file_available = FALSE;
    bool b_authenticated = TRUE;
    
    if (mapRequestPath(&cp_mapped_path, &b_static) == FALSE)
    {
        secExit(STATUS_NOT_FOUND);
    }
    
    if (checkRequestPath(cp_mapped_path) == FALSE)
    {
        secExit(STATUS_NOT_FOUND);
    }
    
    cp_search_path_root = b_static ? scp_web_dir_ : scp_cgi_dir_;
    
    if (searchForHTDigestFile(cp_mapped_path, cp_search_path_root, &b_digest_file_available, &cp_path_to_htdigest_file) == EXIT_FAILURE)
    {
        
         //  We found two .htdigest Files in the path! File is protected!
         
        secExit(STATUS_FORBIDDEN);
    }
    
    if (b_digest_file_available)
    {
        b_authenticated = authenticate(cp_path_to_htdigest_file);
    }


    if(b_authenticated == TRUE)
    {
        if(b_static == TRUE)
        {
            processStaticFile(cp_mapped_path);
        }
        else
        {
            processCGIScript(cp_mapped_path);
        }
    }
    
    secCleanup();
    return EXIT_SUCCESS;
}
