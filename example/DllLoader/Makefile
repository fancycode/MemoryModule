UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
ifndef PLATFORM
PLATFORM = i686
endif
CC = $(PLATFORM)-w64-mingw32-g++
CXX = $(PLATFORM)-w64-mingw32-g++
LINK = $(PLATFORM)-w64-mingw32-ld
else
CC = g++
CXX = g++
LINK = ld
endif

RM = rm
CFLAGS  = -Wall -g
LDFLAGS = -static

ifdef UNICODE
CFLAGS += -DUNICODE -D_UNICODE
endif

OBJ = DllLoader.o ../../MemoryModule.o

DllLoader.exe: $(OBJ)
	$(CC) $(LDFLAGS) -o DllLoader.exe $(OBJ)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

%.o: %.cc
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) -rf $(OBJ) DllLoader.exe
