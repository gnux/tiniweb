all: bin

doc:
	cd doc && doxygen

bin:
	cd src && make

clean:
	cd src && make clean
