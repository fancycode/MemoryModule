UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
CC = i686-w64-mingw32-gcc
CXX = i686-w64-mingw32-g++
LINK = i686-w64-mingw32-ld
else
CC = gcc
CXX = g++
LINK = ld
endif

CFLAGS  = -Wall -g
LDFLAGS = 

OBJ = DllLoader.o ../../MemoryModule.o

DllLoader.exe: $(OBJ)
	$(CC) $(LDFLAGS) -o DllLoader.exe $(OBJ)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

%.o: %.cc
	$(CC) $(CFLAGS) -c $<
