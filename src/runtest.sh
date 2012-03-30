socat -v TCP4-LISTEN:8080,reuseaddr,bind=localhost,fork SYSTEM:"valgrind -v ./tiniweb --cgi-dir=tests/webroot/cgi-bin --verbose --web-dir=../doc --secret=mysuperdupertestsecretstring"
