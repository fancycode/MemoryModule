CC = g++
CFLAGS  = -Wall -g -DSAMPLEDLL_EXPORTS
LDFLAGS = -shared

OBJ = SampleDLL.o

SampleDLL.dll: $(OBJ)
	$(CC) $(LDFLAGS) -o SampleDLL.dll $(OBJ)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<
