DEPFLAGS=-E -Wp,-MM
CC=gcc
INCLUDES=-I
CFLAGS=-Wall -Werror -pipe -g `getconf LFS_CFLAGS` -lm

.PHONY: all clean
all: decode-dimm

dep: .depend


decode-dimm: decode-dimm.c struct.h vendors.h


clean:
	rm -rf .depend *~ *.o decode-dimm
