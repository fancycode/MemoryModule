#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <malloc.h>

#include "../../MemoryModule.h"

typedef int (*addNumberProc)(int, int);

#define DLL_FILE "..\\SampleDLL\\SampleDLL.dll"

void LoadFromFile(void)
{
	addNumberProc addNumber;
	HINSTANCE handle = LoadLibrary(DLL_FILE);
	if (handle == INVALID_HANDLE_VALUE)
		return;

	addNumber = (addNumberProc)GetProcAddress(handle, "addNumbers");
	printf("From file: %d\n", addNumber(1, 2));
	FreeLibrary(handle);
}

void LoadFromMemory(void)
{
	FILE *fp;
	unsigned char *data=NULL;
	size_t size;
	HMEMORYMODULE module;
	addNumberProc addNumber;
	
	fp = fopen(DLL_FILE, "rb");
	if (fp == NULL)
	{
		printf("Can't open DLL file \"%s\".", DLL_FILE);
		goto exit;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	data = (unsigned char *)malloc(size);
	fseek(fp, 0, SEEK_SET);
	fread(data, 1, size, fp);
	fclose(fp);

	module = MemoryLoadLibrary(data);
	if (module == NULL)
	{
		printf("Can't load library from memory.\n");
		goto exit;
	}

	addNumber = (addNumberProc)MemoryGetProcAddress(module, "addNumbers");
	printf("From memory: %d\n", addNumber(1, 2));
	MemoryFreeLibrary(module);

exit:
	if (data)
		free(data);
}

int main(int argc, char* argv[])
{
	LoadFromFile();
	printf("\n\n");
	LoadFromMemory();
	return 0;
}

