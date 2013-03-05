UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
ifndef PLATFORM
PLATFORM = i686
endif
CC = $(PLATFORM)-w64-mingw32-gcc
CXX = $(PLATFORM)-w64-mingw32-g++
LINK = $(PLATFORM)-w64-mingw32-ld
else
CC = gcc
CXX = g++
LINK = ld
endif

RM = rm
CFLAGS  = -Wall -g -DSAMPLEDLL_EXPORTS
LDFLAGS = -shared

OBJ = SampleDLL.o

SampleDLL.dll: $(OBJ)
	$(LINK) $(LDFLAGS) -o SampleDLL.dll $(OBJ)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

clean:
	$(RM) -rf $(OBJ) SampleDLL.dll
