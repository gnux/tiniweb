###############################################################################

Sase Group 03

Christian Partl, Dieter Ladenhauf, Georg Neubauer, Patrick Plaschzug

Datum der Fertigstellung: 11.12.2009

Name of the Project: Wunderwuzzi

###############################################################################


Installation:

Funktioniert wie in der Aufgabenstellung gefordert.


Übergabeparameter:

web-dir, cgi-dir, secret müssen mit Wert übergeben werden, falls nicht
beendet der Server mit einer Fehlermeldung, schickt jedoch nichts über
stdout.

verbose schaltet den Server in Verbose Mode, dies ist optional
cgi-timeout ist optional und wird in Sekunden übergeben als Standard wird
1sek gesetzt ist nichts angegeben. Ist der Wert größer wie 50sek wird
50sek gesetzt.

Die Übergebenen Pfade werden zu Beginn überprüft. Wenn sie keinen gültigen 
Ordner enthalten, so wird das Programm beendet.

Der Server erwartet binnen der Ersten fünf Sekunden eine Nachricht am stdin
ansonsten beendet er (polling). \r\n Sequenzen werden automatisch zu single \n.
Aus der Nachricht wird der Header extrahiert normalisiert und geparst.
Beim Normalisieren wird jedes empfangene Zeichen auf Gültigkeit überprüft

-BLANK ' ' und '\t'
-NEWLINE '\n'
-CHAR 32 < x < 127

Ungültige Headers werden bereits hier erkannt Headerfield (SPACE*): Fieldbody*


Der Normalisierte Header wird in einer Struktur abgespeichert

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
(GET, POST, HEAD, HOST, AUTENTICATION, HTTP/1.1)

Der Parser ist auch dafür zuständig Escaped Chars aufzulösen auf ihre
Gültigkeit zu überprüfen.

Zusätzliche behandlung der URI:

+ wird zu Leerzeichen
Escaped %00 wird als Bad-Request behandelt

Wenn am Ende der URI ein / gefunden wird, wird auf URI/index.html gemappt.

Im falle einer CGI-Response werden Normalizer und Parser auch genutzt der
Normalizer ist über ein FLAG umschaltbar, im Falle von CGI wird cp_first_line
in der Structur nicht gesetzt und laut der RFC nur die Headerfields erkannt.



Cgi-respons:

nach RFC -> Host: ..nur ohne leerzeichen möglich..wieso auch immer
Der Parser wird für cgi-response parsen einfach über eine andere Funktion
gestartet.

Vergleiche: parse und parseCgiResponseHeader

Je nach Aufruf wird vom Parser eine der folgenden Strukturen gefällt:

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
* Structure for parsed cgi response
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




##########Christian: CGI##################





-------Offene FRAGEN:


Get und Post Methoden werden gleich behandelt....
sieh NG: Message Body beim GET 11:34 Uhr 11.12.2009


Child lebt länger wie Parent, erwartetes Verhalten der Konsole umgehen damit das Beenden sauberer wrid


Child töten nach erfolgreichem senden des Headers, CHILD immmer töten?
