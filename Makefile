all: bin

doc:
	cd doc && doxygen

bin:
	$(MAKE) -C src/ all

clean:
	$(MAKE) -C src/ clean

.PHONY: doc bin clean
