CC = gcc
CFLAGS = -Wall -g 

MKREQ_OBJS = mkreq.o
MKSUB_OBJS = mksub.o

.PHONY: all clean 

all: clean mkreq mksub

.c.o:
	$(CC) $(CFLAGS) -c $<

mkreq: $(MKREQ_OBJS)
	$(CC) $(CFLAGS) -o mkreq $(MKREQ_OBJS) -lm

mksub: $(MKSUB_OBJS)
	$(CC) $(CFLAGS) -o mksub $(MKSUB_OBJS) -lm

clean:
	rm -f *.o *~ mkreq mksub
	#rm -f spec/*
	#rm -f alt/*
	#rm -f ../requests/*
