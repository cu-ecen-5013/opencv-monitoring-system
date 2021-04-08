# Name: Makefile
# Description: makefile for final project for ECEN 5013 AESD

ifeq ($(CC),)
	CC = $(CROSS_COMPILE)g++
endif

ifeq ($(LD),)
	LD = ld  # Linker
endif

ifeq ($(CCFLAGS),)
	CCFLAGS = -O0 -g -Wall -Werror 
endif

ifeq ($(LDFLAGS),)
	LDFLAGS = -pthread -lrt 
endif



CPPLIBS= -lopencv_imgcodecs -lopencv_videoio -lopencv_core -lopencv_flann -lopencv_video -lstdc++ #`pkg-config --libs opencv`

HFILES= 
CFILES= 
CPPFILES= capture.cpp

SRCS= ${HFILES} ${CFILES}
CPPOBJS= ${CPPFILES:.cpp=.o}



all:	capture 

clean:
	-rm -f *.o *.d
	-rm -f capture

distclean:
	-rm -f *.o *.d

capture: capture.o
	$(CC) $(LDFLAGS) $(CCFLAGS) -o $@ $@.o $(CPPLIBS) 
#	$(CC) $(LDFLAGS) $(CCFLAGS) -o $@ $@.o `pkg-config --libs opencv` $(CPPLIBS) 

depend:

.c.o:
	$(CC) $(CCFLAGS) -c $<

.cpp.o:
	$(CC) $(CCFLAGS) -c $<
