/** tiniweb
 * \file tiniweb.c
 * \author Sase Tutor Team
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/** tiniweb main routine
 * \param argc number of commandline arguments
 * \param argv arguments itself
 * \return tiniweb return value
 * \todo extend tiniweb with usefull functionality
 * \bug fix your bugs
 */
int main(int argc, char** argv)
{
    // just to make tcp wrapper happy
    char buf[8192];
    int ret = read(STDIN_FILENO, buf, 8192);
    fprintf(stderr, "tiniweb: got %d bytes\n", ret);

    // sample response
    printf("HTTP/1.1 200 OK\r\n"
           "Server: tiniweb/1.0\r\n"
           "Connection: close\r\n"
           "Content-type: text/html\r\n"
           "\r\n"
           "<html><body>Hello!</body></html>\r\n");

    return EXIT_SUCCESS;
}
