

###################################################
#
# Makefile
# Simple TCP/IP communication using sockets
#
# Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
#
###################################################

CC = gcc

CFLAGS = -Wall
CFLAGS += -g
# CFLAGS += -O2 -fomit-frame-pointer -finline-functions

LIBS = 

BINS = test crypto-socket-client 

all: $(BINS)



test: test.c socket-common.h
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

crypto-socket-server: crypto-socket-server.c socket-common.h
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

crypto-socket-client: crypto-socket-client.c socket-common.h
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f *.o *~ $(BINS)
