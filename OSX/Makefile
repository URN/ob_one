CFLAGS+=-MMD -I/usr/local/include
LDLIBS+=-L/usr/local/lib -lportaudio -lopus

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