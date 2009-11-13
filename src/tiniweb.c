/** tiniweb
 * \file tiniweb.c
 * \author Sase Group 03: Plaschzug, Partl, Ladenhauf, Neubauer
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#include "md5.h"
#include "secmem.h"

// default values for options, if no command line option is available
static const char str_web_dir[] = "/";
static const char str_cgi_dir[] = "/cgi-bin/";
static unsigned int val_cgi_timeout = 1;

static int flag_verbose = 0;



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
    int flag_web_dir = 0;
    int flag_cgi_dir = 0;
    int flag_cgi_timeout = 0;
    int option_index = 0;

    sec_test();
    
    md5_state_t my_md5_state;
    md5_init(&my_md5_state);
    
    const md5_byte_t * username = "testuser";
    const md5_byte_t * realm = "testrealm";
    const md5_byte_t * password = "test";
     
    md5_append(&my_md5_state, username, 8);
    md5_append(&my_md5_state, realm, 9);
    md5_append(&my_md5_state, password, 4);
    
    md5_byte_t result[16];
    md5_finish(&my_md5_state, result);

    fprintf(stderr, "MD5-Test Finished, Result: %x \n", (unsigned int)result);
    
    

    // command line parsing: like done in example from
    // http://www.gnu.org/s/libc/manual/html_node/Getopt-Long-Option-Example.html
    while (1) {

        static struct option long_options[] = { { "web-dir", required_argument,
                0, 0 }, { "cgi-dir", required_argument, 0, 1 }, {
                "cgi-timeout", required_argument, 0, 2 },
        // maybe specify verbose options
                { "verbose", no_argument, 0, 3 }, { 0, 0, 0, 0 } };
        option_index = 0;
        c = getopt_long(argc, argv, "", long_options, &option_index);

        if (c == -1)
            break;

        // TODO: Take arguments, in a secure way, alloc mem and copy!
        //        prove flags
        switch (c) {
        case 0:
            fprintf(stderr, "option web-dir used with argument: %s \n", optarg);
            flag_web_dir = 1;
            break;
        case 1:
            fprintf(stderr, "option cgi-dir used with argument: %s \n", optarg);
            flag_cgi_dir = 1;
            break;
        case 2:
            fprintf(stderr, "option cgi-timeout used with argument: %s \n",
                    optarg);
            flag_cgi_timeout = 1;
            break;
        case 3:
            fprintf(stderr, "switching to verbose mode \n");
            flag_verbose = 1;
            break;
        default:
            abort();
            break;
        }
    }

    // TODO: prove flags, if no arg is given use default-vals


    fprintf(stderr, "Argument parsing finished \n");

    // just to make tcp wrapper happy 
    //    char buf[8192];
    //    int ret = read(STDIN_FILENO, buf, 8192);
    //    fprintf(stderr, "tiniweb: got %d bytes\n", ret);

    // sample response
    //    printf("HTTP/1.1 200 OK\r\n"
    //           "Server: tiniweb/1.0\r\n"
    //           "Connection: close\r\n"
    //           "Content-type: text/html\r\n"
    //           "\r\n"
    //           "<html><body>Hello!</body></html>\r\n");   
    
    return EXIT_SUCCESS;
}
