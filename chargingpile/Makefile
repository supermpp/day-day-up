#
# Makefile for EXE 'chargingpile'
#

TARG   = chargingpile
OBJS   = main.o common.o mypthread.o rwconfig.o tcpsocket.o ini-file.o myprotocol.o rc4.o linklist.o mytime.o serial.o rwserial.o

GCC_ROOT=/usr/local/arm/4.2.2-eabi

CC     = $(GCC_ROOT)/usr/bin/arm-linux-gcc
CFLAGS= -O2 -Wall -g -D_REENTRANT -I../lib 

LD       = $(GCC_ROOT)/usr/bin/arm-linux-gcc
LDFLAGS= -O2 -Wall -g -D_REENTRANT  -L/lib

LIBS= -lpthread
all: $(TARG)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

$(TARG): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.PHONY : clean

clean:
	rm -f $(OBJS) $(TARG)


