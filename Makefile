#makefile to compile Rodent II on Linux

# define the C compiler to use
CC = g++

# define the compile-time flags
CFLAGS = -g -w -Wfatal-errors -pipe -DNDEBUG -O3 -static -fno-rtti -finline-functions -fprefetch-loop-arrays

# define the link options
LDFLAGS= -s -static -lm

.PHONY: clean

default: rodent

rodent:
	$(CC) $(LDFLAGS) $(CFLAGS) -o rodentII -x c++ compile.linux

%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f rodentII

