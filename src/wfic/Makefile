CC=gcc #-m32

OBJS= wfic.o interleaver.o puncture.o viterbi.o scrambler.o

SRCS= wfic.c interleaver.c puncture.c viterbi.c scrambler.c

INCS= 

CFLAGS= -Wall -g -O3

AR= /usr/bin/ar
ARFLAGS= q

all:	$(OBJS) libwfic.a 

libwfic.a:	$(OBJS)
	$(AR) $(ARFLAGS) libwfic.a $(OBJS)

clean:
	rm -f $(OBJS) libwfic.a

