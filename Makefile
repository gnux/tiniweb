all: bin

doc:
	cd doc && doxygen

bin:
	$(MAKE) -C src/ all

clean:
	$(MAKE) -C src/ clean

run: bin
	socat -v TCP4-LISTEN:8080,reuseaddr,bind=localhost,fork SYSTEM:"src/tiniweb --cgi-dir=./cgi-bin --web-dir=htdoc"

singlerun: bin
	socat -v TCP4-LISTEN:8080,reuseaddr,bind=localhost SYSTEM:"src/tiniweb --cgi-dir=./cgi-bin --web-dir=htdoc"

.PHONY: doc bin clean run singlerun
