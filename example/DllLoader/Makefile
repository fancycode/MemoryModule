CC = gcc
CPP = g++
LINK = ld
CFLAGS  = -Wall -g
LDFLAGS = 

OBJ = DllLoader.o ../../MemoryModule.o

DllLoader.exe: $(OBJ)
	$(CC) $(LDFLAGS) -o DllLoader.exe $(OBJ)

%.o: %.cpp
	$(CPP) $(CFLAGS) -c $<

%.o: %.cc
	$(CC) $(CFLAGS) -c $<
