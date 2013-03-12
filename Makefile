export

CC_PREFIX=
OUTDIR:=$(shell uname -m)
CC=$(CC_PREFIX)gcc
CFLAGS=-g -O0 -Wall -DTRACE
INCLUDE=
LIBS=-lpthread -lasound -lswscale \
		-lavformat -lavcodec \
				-lao -lavutil
LD=$(CC)

COMMON_OBJS=debug.o boltsnap.o boltsnap_command.o \
				command_table.o play_command.o \
				control_command.o common_cgi.o

IMPULSED_OBJS=daemon.o play_command_dispatch.o playthread.o\
				control_command_dispatch.o

PLAYCGI_OBJS=playcgi.o
CONTROLCGI_OBJS=controlcgi.o

IMPULSED_BINARY=$(OUTDIR)/boltsnapd
PLAYCGI_BINARY=$(OUTDIR)/bin/boltplay
CONTROLCGI_BINARY=$(OUTDIR)/bin/boltcontrol

ALL_BUILDS=boltsnapd playcgi controlcgi mediaload
ARCHS=$(shell uname -m) arm i686 amd64

MEDIA_LOAD_DIR=sql_load

default: OUTDIR:=$(shell uname -m)
default: all

arm: CC_PREFIX=arm-linux-gnueabi-
arm: OUTDIR=arm
arm: all

i686: OUTDIR=i686
i686: CFLAGS+=-m32
i686: all

amd64: OUTDIR=amd64
amd64: CFLAGS+=-m64
amd64: all

all: build

package:
	./build_package.sh

install: ARCH:=$(shell uname -m)
install: PACKAGE:=$(ARCH)_package
install: package
	tar -xzvf $(PACKAGE).tgz
	cd $(PACKAGE)
	./install.sh

mediaload:
	cd $(MEDIA_LOAD_DIR) ; make ; cd ..

boltsnap.o: boltsnap.c boltsnap.h
	$(CC) $(CFLAGS) $(INCLUDE) -c boltsnap.c

boltsnap_command.o: boltsnap_command.c boltsnap_command.h boltsnap_command.h
	$(CC) $(CFLAGS) $(INCLUDE) -c boltsnap_command.c

command_table.o: command_table.c command_table.h boltsnap_command.h	
	$(CC) $(CFLAGS) $(INCLUDE) -c command_table.c

play_command.o: play_command.c play_command.h boltsnap_command.h
	$(CC) $(CFLAGS) $(INCLUDE) -c play_command.c

play_command_dispatch.o: play_command.o play_command_dispatch.c
	$(CC) $(CFLAGS) $(INCLUDE) -c play_command_dispatch.c

daemon.o: daemon.c daemon.h boltsnap_command.h command_table.h play_command.h
	$(CC) $(CFLAGS) $(INCLUDE) -c daemon.c

playcgi.o: playcgi.c
	$(CC) $(CFLAGS) $(INCLUDE) -c playcgi.c

playthread.o: playthread.c playthread.h
	$(CC) $(CFLAGS) $(INCLUDE) -c playthread.c

control_command.o: control_command.c control_command.h
	$(CC) $(CFLAGS) $(INCLUDE) -c control_command.c

control_command_dispatch.o: control_command.o
	$(CC) $(CFLAGS) $(INCLUDE) -c control_command_dispatch.c

debug.o: debug.c
	$(CC) $(CFLAGS) $(INCLUDE) -c debug.c

common_cgi.o: common_cgi.c
	$(CC) $(CFLAGS) $(INCLUDE) -c common_cgi.c

boltsnapd: $(IMPULSED_OBJS) $(COMMON_OBJS)
	$(LD) $(IMPULSED_OBJS) $(COMMON_OBJS) -o $(IMPULSED_BINARY)	$(LIBS)

playcgi: $(PLAYCGI_OBJS) $(COMMON_OBJS)
	$(LD) $(PLAYCGI_OBJS) $(COMMON_OBJS) -o $(PLAYCGI_BINARY) $(LIBS)

controlcgi: $(CONTROLCGI_OBJS) $(COMMON_OBJS)
	$(LD) $(CONTROLCGI_OBJS) $(COMMON_OBJS) -o $(CONTROLCGI_BINARY) $(LIBS)

build: init $(ALL_BUILDS)
	@echo "Builds Complete!"
	mv *.o $(OUTDIR)/

clean:
	- rm -rf *.o $(IMPULSED_BINARY) $(PLAYCGI_BINARY)
	- rm -rf $(ARCHS)
	- rm -rf *.tgz
	- rm -rf *.sqlite3
	- rm -rf .*.sw*
	cd $(MEDIA_LOAD_DIR) ; make clean ; cd ..

test: all
	./$(IMPULSED_BINARY)

init:
	mkdir -p $(OUTDIR)/bin
	- mv $(OUTDIR)/*.o .
