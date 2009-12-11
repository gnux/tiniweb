#############################################################################################################

Sase Group 03

Christian Partl (0430360),
Dieter Ladenhauf (0530361),
Georg Neubauer (0530228),
Patrick Plaschzug (0530227)

Datum der Fertigstellung: 11.12.2009

Name of the Project: Wunderwuzzi

#############################################################################################################


Installation:

make
./tiniweb --web-dir xxx --cgi-dir xxx --secret xxx (--cgi-timeout xxx) (--verbose)

Kurze Beschreibung:
Wunderwuzzi ist ein Webserver, welcher eine vereinfachte Version des HTTP/1.1
Protokolls implementiert und statische Files anbietet. Weiters ist es möglich
CGI-Skripts ausführen. 

Software Anforderungen:
Compiler GCC 4.3.2 mit GNU Libc 2.7 oder neuer
x86 (32-bit oder 64-bit) Linux mit 2.6.x Kernel

Übergabeparameter:

web-dir, cgi-dir, secret müssen mit Wert übergeben werden, falls nicht
beendet der Server mit einer Fehlermeldung, schickt jedoch nichts über
stdout.

verbose schaltet den Server in Verbose Mode, dies ist optional
cgi-timeout ist optional und wird in Sekunden übergeben als Standard wird
1sek gesetzt ist nichts angegeben.
Ist der Wert größer als 50sek wird 50sek gesetzt.
Ist der Wert kleiner als 1sek wird 1sek gesetzt.

Die Übergebenen Pfade werden zu Beginn überprüft. Wenn sie keinen gültigen 
Ordner enthalten, wird das Programm beendet.

Der Server erwartet binnen der ersten fünf Sekunden eine Nachricht am stdin
ansonsten beendet er (polling). \r\n Sequenzen werden automatisch zu single \n.
Aus der Nachricht wird der Header extrahiert normalisiert und geparst.
Beim Normalisieren wird jedes empfangene Zeichen auf Gültigkeit überprüft

-BLANK ' ' und '\t'
-NEWLINE '\n'
-CHAR 32 < x < 127

Ungültige Headers werden bereits hier erkannt. Gültige müssen wie folgt aussehen:
Fieldheader (SPACE*): Fieldbody*

Wird ein ungültiger Input erkannt antwortet der Server mit "Bad Request" und beendet.

Der Normalisierte Header wird in einer Struktur abgespeichert.

/** 
* Structure for normalised and splittet header
*/

typedef struct http_norm {
char *cp_first_line;
/**< first line of the header (just in case of request) */
ssize_t i_num_fields; /**< number of available header fields */
char **cpp_header_field_name; /**< names of the header fields */
char **cpp_header_field_body; /**< header field bodys */
} http_norm;


Diese Struktur wird an den Parser übergeben, dieser prüft nun alle Eintrage
auf Gültigkeit und Extrahiert alle erforderlichen Request Informationen.
(GET, POST, HEAD, HOST, AUTENTICATION, HTTP/1.1, etc.)

Der Parser ist auch dafür zuständig Escaped Chars aufzulösen auf ihre
Gültigkeit zu überprüfen.

Zusätzliche behandlung der URI:

+ wird zu Leerzeichen
Escaped %00 wird als Bad-Request behandelt
Wenn am Ende der URI ein / gefunden wird, wird auf URI/index.html gemappt.

Im Falle einer CGI-Response werden Normalizer und Parser auch genutzt der
Normalizer ist über ein FLAG umschaltbar, im Falle von CGI wird cp_first_line
in der Struktur nicht gesetzt und laut der RFC nur die Headerfields erkannt.

Cgi-response:

nach RFC -> Fieldheader: ...nur ohne Leerzeichen möglich
Der Parser wird für cgi-response parsen einfach über eine andere Funktion
gestartet.
Vergleiche: parse und parseCgiResponseHeader
Je nach Aufruf des Parsers, wird eine der folgenden Strukturen gefüllt:

/**
* Structure for parsed cgi response
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
* Structure for parsed http request
*/

typedef struct http_request{
  char *cp_method; /**< used request method */
  char *cp_uri; /**< requested uri */
  char *cp_query; /**< query string from uri */
  char *cp_fragment; /**< fragment from uri */
  char *cp_path; /**< requested file path */
}http_request;


Pathchecking:

Wenn Requests wie z.B. '../../index.html' eintreffen, so entspricht dies 
einem Request auf '/index.html'. Das Webroot, cgi-bin kann auf diese Weise 
nicht verlassen werden.
Damit ist es nicht möglich die Ordnerstruktur der unterliegenden Verzeichnisse
herauszufinden.
Wenn man zufällig im cgi-dir landet, so wird der Request als dynamisch 
interpretiert und das referenzierte Skript ausgeführt.


Authentication:

Die Authentifizierung ist vollständig implementiert.
Die Nonce wird mittels HMAC MD5 erzeugt. (Referenzen wurden im Code entsprechend ausgewiesen)
Sie hat das folgende Format: 

   time + md5(path) + hmacmd5(time : md5(path))

Sie ist nach dem Erstellen genau 3600 Sek (1 Stunde) gültig.


CGI:

Ein CGI Skript wird grundsätzlich mittels fork/exec ausgeführt. Dabei gilt, dass das CGI Skript 
innerhalb des CGI-Timeouts den kompletten Header geschickt haben muss, ein eventuell vorhandener
Body wird danach einfach durchgeschleust. Wird beim Durchscheusen länger als das CGI-Timeout nichts
geschickt, wird der Body als vollständig behandelt. Da nach Beenden des Servers ein CGI Skript potentiell
ewig weiterlaufen könnte, wird dieses kurz vor dem Beenden des Servers terminiert.
Weiters wird der Body eines HTTP-Requests unabhängig von der HTTP-Method (GET, POST, HEAD) immer an das
CGI Skript weitergeletet (sofern vorhanden).
Der CGI-Header muss bei unserer Implementierung als erstes Header Field den Content-Type und als 2. optional
den Status gesetzt haben (siehe RFC3875). Da ein eventuell vom CGI gesetztes Content-Length Field nicht
vertrauenswürdig ist, wird dieses auf 0 gesetzt (-> bis eof lesen). Sollte ein weiters Content-Type oder
Status Feld gefunden werden, wird ein Internal Server Error ausgegeben. Die Felder Server und Connection
werden statisch gesetzt.
