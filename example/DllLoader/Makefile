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
OBJ_LOADER = DllLoaderLoader.o ../../MemoryModule.o

all: DllLoader.exe DllLoaderLoader.exe

DllLoader.exe: $(OBJ)
	$(CC) $(LDFLAGS) -Wl,--image-base -Wl,0x20000000 -o DllLoader.exe $(OBJ)

DllLoaderLoader.exe: $(OBJ_LOADER)
	$(CC) $(LDFLAGS) -Wl,--image-base -Wl,0x10000000 -o DllLoaderLoader.exe $(OBJ_LOADER)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

%.o: %.cc
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) -rf $(OBJ) $(OBJ_LOADER) DllLoader.exe DllLoaderLoader.exe
