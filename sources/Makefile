#makefile to compile Rodent II on Linux

# define the C compiler to use
CC = g++

# define the compile-time flags
CFLAGS = -g -w -Wfatal-errors -pipe -DNDEBUG -O3 -static -fno-rtti -finline-functions -fprefetch-loop-arrays

# define the link options
LDFLAGS= -s -static -lm

#define the directory for the executable file
BINDIR = /usr/bin

#define the directory for the data files
DATADIR = /usr/share/rodentII

.PHONY: clean install

default: rodent

rodent:
	sed -i s\@guide.bin@$(DATADIR)/guide.bin@ src/main.cpp
	sed -i s\@rodent.bin@$(DATADIR)/rodent.bin@ src/main.cpp
	sed -i s\@basic.ini@$(DATADIR)/basic.ini@ src/main.cpp
	$(CC) $(LDFLAGS) $(CFLAGS) -o rodentII -x c++ compile.linux
	echo "SHOW_OPTIONS" > basic.ini

%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f rodentII

install:
	mkdir -p $(BINDIR)
	mkdir -p $(DATADIR)
	cp rodentII $(BINDIR)
	cp basic.ini $(DATADIR)
	cp ../books/* $(DATADIR)
	cp -r ../personalities/*/*.ini $(DATADIR)
	chmod 755 $(BINDIR)/rodentII
	chmod 644 $(DATADIR)/*
