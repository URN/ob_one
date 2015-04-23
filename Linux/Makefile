CFLAGS+=-MMD
LDLIBS+=-lportaudio -lasound -lopus

-include .config

.PHONY:	all

all:	tx rx

tx:	tx.o

rx:	rx.o

clean:
	rm -f tx rx
	rm -f *.o *.d

install:
	cp tx rx /usr/bin
