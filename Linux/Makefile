CFLAGS+=-MMD
LDLIBS+=-lasound -lopus

-include .config

.PHONY:	all

all:	tx rx

tx:	tx.o device.o

rx:	rx.o device.o

clean:
	rm -f tx rx
	rm -f *.o *.d
