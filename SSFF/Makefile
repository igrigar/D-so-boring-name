# OPERATING SYSTEMS DESING - 16/17
# Makefile for OSD file system

INCLUDEDIR=./include
CC=gcc
CFLAGS=-g -Wall -Werror -std=gnu11 -lz -I$(INCLUDEDIR)
AR=ar
MAKE=make

OBJS_DEV= blocks_cache.o filesystem.o crc.o
LIB=libfs.a


all: create_disk test

test: $(LIB)
	$(CC) $(CFLAGS) -o test test.c libfs.a

filesystem.o: $(INCLUDEDIR)/filesystem.h
blocks_cache.o: $(INCLUDEDIR)/blocks_cache.h
crc.o: $(INCLUDEDIR)/crc.h

$(LIB): $(OBJS_DEV)
	$(AR) rcv $@ $^

create_disk: create_disk.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(LIB) $(OBJS_DEV) test create_disk create_disk.o
