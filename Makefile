# Makefile for Polyphon-NG, procedural music generator
CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic --std=gnu99 -g

INCLUDES := midi.h music.h bits.h
LIBS := midi.o music.o bits.o

all: polyphon

polyphon: polyphon.c $(INCLUDES) $(LIBS)
	$(CC) $(CFLAGS) $(CPPFLAGS) polyphon.c $(LDFLAGS) $(LIBS) -o polyphon

music.o: bits.h

%.o: %.c %.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

