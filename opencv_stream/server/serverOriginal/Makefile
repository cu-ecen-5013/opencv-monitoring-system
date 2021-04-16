# Name: Makefile
# Description: makefile for final project for ECEN 5013 AESD

ifeq ($(CC),)
	CC = $(CROSS_COMPILE)g++
endif

ifeq ($(LD),)
        LD = ld  # Linker
endif

ifeq ($(CCFLAGS),)
#	CCFLAGS = -g -Wall -Werror `pkg-config --cflags opencv`
	CCFLAGS = -g `pkg-config --cflags opencv`
endif

ifeq ($(LDFLAGS),)
	LDFLAGS = -pthread -lrt
endif



#HFILES= 
#CFILES= 
CPPFILES= facedetect.cpp

#CPPLIBS= `pkg-config --libs opencv` -lopencv_core -lopencv_flann -lopencv_video -lstdc++
CPPLIBS= `pkg-config --libs opencv` -lstdc++
SRCS= ${HFILES} ${CFILES}
CPPOBJS= ${CPPFILES:.cpp=.o}



all:	server

clean:
	-rm -f *.o *.d
	-rm -f server

distclean:
	-rm -f *.o *.d

server: server.o
	$(CC) $(LDFLAGS) $(CCFLAGS) -pthread -o $@ $@.o  $(CPPLIBS) 
depend:

.c.o:
	$(CC) $(CCFLAGS) -c $<

.cpp.o:
	$(CC) $(CCFLAGS) -c $<
