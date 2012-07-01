CC = gcc
CFLAGS = -Wall -g -mcmodel=medium 

EMBED_OBJS = util.o embed.o

.PHONY: all clean 

all: clean embed

.c.o:
	$(CC) $(CFLAGS) -c $<

embed: $(EMBED_OBJS)
	$(CC) $(CFLAGS) -o embed $(EMBED_OBJS) -lm


clean:
	rm -f *.o *~ embed
