
Wunderwuzzi is a small webserver written in C, which implements a 
simple version of the protocol HTTP in version 1.0. It offers the 
possibility to deliver static files and to run CGI scripts.

The source code of this project was written during the course "Security
Aspects in Software Engineering" (in german "Sicherheitsaspekte in 
der Softwareentwicklung" [1]) at Graz University of Technology [2] 
during the winter term in 2009/2010.

The authors of Wunderwuzzi are:

Dieter Ladenhauf,
Georg Neubauer,
Christian Partl, and
Patrick Plaschzug

Due to the fact that this project helped us to improve our 
understanding of HTTP, CGI, webservers, and of course the programming 
language C we decided to release it under the terms and conditions of 
the GNU General Public License (GNU GPL) in order to help others. We 
hope that you benefit from it. Feel free to do whatever you want with 
Wunderwuzzi.

-----------------------------------------------------------------------
-----------------------------------------------------------------------

Software requirements:

Compiler GCC 4.3.2 with GNU Libc 2.7 or higher
x86 (32-bit or 64-bit) Linux with 2.6.x Kernel

-----------------------------------------------------------------------
-----------------------------------------------------------------------

Compile and run Wunderwuzzi:

  make

  ./tiniweb --web-dir xxx --cgi-dir xxx --secret xxx 
           (--cgi-timeout xxx) (--verbose)

-----------------------------------------------------------------------

Mandatory parameters:

If Wunderwuzzi is started without any of the following mandatory 
parameters, the webserver is terminated with displaying an error 
message. However, it does not send anything via std out.

--web-dir xxx

   The local directory which is mapped on the server. It contains the 
   files which can be accessed using a HTTP request to the server.

--cgi-dir xxx

   The local directory which is mapped on the server, containing the 
   CGI scripts.

--secret xxx

   This string determines the secret which is used for authentication 
   purposes.

-----------------------------------------------------------------------

Optional parameters:

--cgi-timeout xxx

   The CGI timeout sets the amount of time, the SGI script has to send 
   its complete header. If the script does not send any content for 
   longer than the given timeout span, it is assumed that the complete 
   body was sent.

   The time span has to be specified using seconds. If the parameter 
   is not specified, the default value of 1 second is set.

   If the value is larger than 50 seconds, it is set to 50.
   If the value is smaller than 1 second, it is set to 1.

--verbose

   This parameter triggers the webserver to start in verbose mode. In 
   this case, extended debug output will be displayed.

-----------------------------------------------------------------------
-----------------------------------------------------------------------

Testfiles:

The files request01.in - request16.in within the folder src/tests 
contain static testinput for the webserver. Additionally, there are the
files doxybrowse.sh and normalbrowse.sh in order to test the webserver 
using a web browser. 

-----------------------------------------------------------------------
-----------------------------------------------------------------------

Detailed description of Wunderwuzzi:

Wunderwuzzi includes a minimalistic garbage collection mechanism, 
which consists of a double chained list containing all registered 
memory. Using this mechanism, it is possible to free the used memory 
at once in order to terminate the webserver safely.

In the beginning, the given paths are analyzed and if they are not 
valid, the server is terminated. After starting, if no message is 
recieved on std-in for a timespan of exactly five seconds (polling), 
the server is terminated again.

-----------------------------------------------------------------------

Normalizing and parsing:

The header of a message is extracted and normalized at the beginning 
and during the normalizing process, every single character is checked 
if it is valid. The valid characters are:

-BLANK ' ' and '\t'
-NEWLINE '\n'
-CHAR 32 < x < 127

Strings containing "\r\n" are automatically converted to "\n". Not 
valid headers are detected during this procedure and in this case, 
the server answers with a "Bad Request" and terminates itself. Valid 
headers must look as follows:

Fieldheader (SPACE*): Fieldbody*

Valid headers are saved within this structure:

/** 
* Structure for normalized and split header
*/
typedef struct http_norm {

  char *cp_first_line;
  /**< first line of the header (just in case of request) */
  ssize_t i_num_fields; /**< number of available header fields */
  char **cpp_header_field_name; /**< names of the header fields */
  char **cpp_header_field_body; /**< header field bodies */

} http_norm;

This structure is passed to the parser, which checks all entries for 
validity and extracts all fields necessary for the request (GET, POST, 
HEAD, HOST, AUTENTICATION, HTTP/1.1, etc.). Additionally, the parser is
responsible for validating and converting all escaped chars. There are 
three special cases:

*)  + becomes a space
*)  %00 is treated as a "Bad Request"
*)  If there is a / at the end of the URI, the server maps to 
       URI/index.html

In the case of a CGI response, the normalizer and the parser are used 
as well, because the normalizer can be controlled via a flag. In case 
of a CGI request, the field cp_first_fine in the structure is not set 
and according to RFC, only the header fields are recognized. 

-----------------------------------------------------------------------

CGI response:

As mentioned before, the parser is started for parsing the CGI response
 with another function (parseCgiResponseHeader() instead of parse()). 
Depending on the call, one of these structures is filled:

/**
* Structure for parsed CGI response
*/
typedef struct http_cgi_response{

  int i_num_fields; /**< number of available header fields */
  char *content_type; /**< response content type */
  char *status; /**< response status */
  char *connection; /**< response connection information */
  char *server; /**< which server */
  char **cpp_header_field_name; /**< names of other header fields */
  char **cpp_header_field_body; /**< header field bodys */

} http_cgi_response;


/**
* Structure for parsed HTTP request
*/
typedef struct http_request{

  char *cp_method; /**< used request method */
  char *cp_uri; /**< requested uri */
  char *cp_query; /**< query string from uri */
  char *cp_fragment; /**< fragment from uri */
  char *cp_path; /**< requested file path */

} http_request;

-----------------------------------------------------------------------

Pathchecking:

If the requested path looks like "../../index.html", the server maps 
the request to "index.html". In this way it is ensured, that the 
webroot (and cgi-bin) directory cannot be left and the underlying 
folder structure cannot be explored. If the request targets for some 
reason the cgi-bin directory, the request is handled as dynamic and the
referenced CGI script is called.

-----------------------------------------------------------------------

Authentication:

The authentication is fully implemented. It uses a nonce, which is 
created with HMAC MD5. Here, foreign code is used. It is properly 
declared within the source files. The nonce consists of this parts:

   time + md5(path) + hmacmd5(time : md5(path))

It is valid for 3600 seconds (1 hour) after its creation.

-----------------------------------------------------------------------

CGI:

In Wunderwizzei, a CGI script is executed using fork and exec syscalls.
Thereby, the script must send the complete header during the CGI 
timeout (which can be passed on startup). Next, the body (if available)
is sent. If nothing is sent any more for a timespan longer than the CGI
timeout, it is assumed that the body is complete. Due to the fact that 
the process of a CGI script could be running indefinitly after the 
server is closed, the process is automatically terminated if the server
is shot down.

The body of a HTTP request is sent to the CGI script (if available) 
independently from the HTTP method (GET, POST, HEAD).

In Wunderwuzzi, the first header field of a CGI header must be the 
content type and the second field must be the status (see RFC3875). Due
to the fact that a content length field set from CGI, is not 
trustworthy, it is set to 0 (read until eof). If there is any 
additional content type field found, an "Internal Server Error" is 
returned by the server. The fields Server and Connect are set 
statically.

-----------------------------------------------------------------------

[1] https://online.tugraz.at/tug_online/lv.detail?clvnr=138501&sprache=1
[2] http://www.tugraz.at/


