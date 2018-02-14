UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
ifndef PLATFORM
PLATFORM = i686
endif
CC = $(PLATFORM)-w64-mingw32-g++
CXX = $(PLATFORM)-w64-mingw32-g++
LD = $(PLATFORM)-w64-mingw32-ld
RC = $(PLATFORM)-w64-mingw32-windres
else
CC = g++
CXX = g++
LD = ld
RC = rc
endif

RM = rm
CFLAGS  = -Wall -g -DTESTSUITE
LDFLAGS =
RCFLAGS = -O coff

ifdef UNICODE
CFLAGS += -DUNICODE -D_UNICODE
endif

CFLAGS_DLL = -DSAMPLEDLL_EXPORTS
CFLAGS_EXE =
LDFLAGS_DLL = -shared
LDFLAGS_EXE = -static

TEST_DLLS = \
	test-align-128.dll \
	test-align-256.dll \
	test-align-512.dll \
	test-align-768.dll \
	test-align-1024.dll \
	test-align-2048.dll \
	test-align-3072.dll \
	test-align-4096.dll \
	test-align-100.dll \
	test-align-200.dll \
	test-align-300.dll \
	test-align-400.dll \
	test-align-500.dll \
	test-align-600.dll \
	test-align-800.dll \
	test-align-900.dll \
	test-relocate.dll \
	test-exports.dll

LOADDLL_OBJ = LoadDll.o ../MemoryModule.o
TESTSUITE_OBJ = TestSuite.o ../MemoryModule.o
DLL_OBJ = SampleDLL.o SampleDLL.res

all: prepare_testsuite LoadDll.exe TestSuite.exe $(TEST_DLLS)

prepare_testsuite:
	rm -f $(TESTSUITE_OBJ)

LoadDll.exe: $(LOADDLL_OBJ)
	$(CC) $(LDFLAGS_EXE) $(LDFLAGS) -Wl,--image-base -Wl,0x20000000 -o LoadDll.exe $(LOADDLL_OBJ)

TestSuite.exe: $(TESTSUITE_OBJ)
	$(CC) $(LDFLAGS_EXE) $(LDFLAGS) -o TestSuite.exe $(TESTSUITE_OBJ)

LoadDll.o: LoadDll.cpp
	$(CXX) $(CFLAGS) $(CFLAGS_EXE) -c $<

test-align-%.dll: $(DLL_OBJ)
	$(LD) $(LDFLAGS_DLL) $(LDFLAGS) --file-alignment $* --section-alignment $* -o $@ $(DLL_OBJ)

test-relocate.dll: $(DLL_OBJ)
	$(CXX) $(LDFLAGS_DLL) $(LDFLAGS) -Wl,--image-base -Wl,0x20000000 -o $@ $(DLL_OBJ)

test-exports.dll: SampleExports.o
	$(CXX) $(LDFLAGS_DLL) $(LDFLAGS) -o $@ SampleExports.o

SampleExports.cpp: generate-exports.sh
	./generate-exports.sh

%.o: %.cpp
	$(CXX) $(CFLAGS) $(CFLAGS_DLL) -c $<

%.o: %.cc
	$(CC) $(CFLAGS) $(CFLAGS_DLL) -c $<

%.res: %.rc
	$(RC) $(RCFLAGS) -o $*.res $<

clean:
	$(RM) -rf LoadDll.exe $(TEST_DLLS) $(LOADDLL_OBJ) $(DLL_OBJ) $(TESTSUITE_OBJ) SampleExports.o

test: all
	./runwine.sh $(PLATFORM) TestSuite.exe
	./runtests.sh $(PLATFORM) "$(TEST_DLLS)"
