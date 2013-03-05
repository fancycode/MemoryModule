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

CFLAGS  = -Wall -g -DSAMPLEDLL_EXPORTS
LDFLAGS = -shared

OBJ = SampleDLL.o

SampleDLL.dll: $(OBJ)
	$(LINK) $(LDFLAGS) -o SampleDLL.dll $(OBJ)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<
