UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
ifndef PLATFORM
PLATFORM = i686
endif
CC = $(PLATFORM)-w64-mingw32-g++
CXX = $(PLATFORM)-w64-mingw32-g++
LINK = $(PLATFORM)-w64-mingw32-g++
RC = $(PLATFORM)-w64-mingw32-windres
else
CC = g++
CXX = g++
LINK = ld
RC = rc
endif

RM = rm
CFLAGS  = -Wall -g -DSAMPLEDLL_EXPORTS
LDFLAGS = -shared
RCFLAGS = -O coff

ifdef UNICODE
CFLAGS += -DUNICODE -D_UNICODE
endif

OBJ = SampleDLL.o SampleDLL.res

SampleDLL.dll: $(OBJ)
	$(LINK) $(LDFLAGS) -o SampleDLL.dll $(OBJ)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

%.res: %.rc
	$(RC) $(RCFLAGS) -o $*.res $<

clean:
	$(RM) -rf $(OBJ) SampleDLL.dll
