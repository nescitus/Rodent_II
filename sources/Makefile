#makefile to compile Rodent II on Linux

# define the C compiler to use
CC = g++

# define the compile-time flags
CFLAGS = -g -w -Wfatal-errors -pipe -DNDEBUG -O3 -fno-rtti -finline-functions -fprefetch-loop-arrays

# define the link options
LDFLAGS = -s -lm

#define the directory for the executable file
BINDIR = /usr/bin

#define the directory for the data files
DATADIR = /usr/share/rodentII

EXENAME= rodentII
CONFIGFILE = basic.ini

.PHONY: clean install update help

default: build

build:
	@echo "Type make help for additional options"
	$(CC) $(LDFLAGS) $(CFLAGS) -o $(EXENAME) -x c++ compile.linux
	echo "SHOW_OPTIONS" > $(CONFIGFILE)

build-static:
	@echo "Type make help for additional options"
	$(CC) $(LDFLAGS) -static $(CFLAGS) -o $(EXENAME) -x c++ compile.linux
	echo "SHOW_OPTIONS" > $(CONFIGFILE)

%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXENAME)

install:
	mkdir -p $(BINDIR)
	mkdir -p $(DATADIR)
	cp $(EXENAME) $(BINDIR)
	cp $(CONFIGFILE) $(DATADIR)
	cp ../books/* $(DATADIR)
	cp -r ../personalities/*/*.ini $(DATADIR)
	chmod 755 $(BINDIR)/$(EXENAME)
	chmod 644 $(DATADIR)/*

# this just performs a copy of rodentII engine
update:
	cp $(EXENAME) $(BINDIR)
	
help:
	@echo ""
	@echo "To compile RodentII type:"
	@echo ""
	@echo "make build		> Compile Rodent II"
	@echo "make build-static	> Compile Rodent II as a static binary"
	@echo "make clean 		> Clean up"
	@echo "make install		> Install RodentII (root privileges required)"
	@echo "make update		> Update RodenII engine (root privileges required)"
	