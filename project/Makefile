CC=g++
DFLAGS=-g -fsantize=address

default: build

build: 
	${CC} *.cpp -o cpusim

debug:
	${CC} ${DFLAGS} *.cpp -o cpusim

clean: 
	rm -f cpusim
