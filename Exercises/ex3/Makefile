CC=g++
CXX=g++
RANLIB=ranlib

LIBSRC=MapReduceFramework.cpp Barrier/Barrier.cpp Barrier/Barrier.h
LIBOBJ=$(LIBSRC:.cpp=.o)

INCS=-I.
CFLAGS = -Wall -std=c++11 -g $(INCS)
CXXFLAGS = -Wall -std=c++11 -g $(INCS)

UTHREADLIB = libMapReduceFramework.a
TARGETS = $(UTHREADLIB)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex3.tar
TARSRCS= Makefile README

all: $(TARGETS)

$(TARGETS): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

clean:
	$(RM) $(TARGETS) $(OSMLIB) $(OBJ) $(LIBOBJ) *~ *core

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
