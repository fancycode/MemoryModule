#include <windows.h>
#include <stdio.h>

#include "SampleDLL.h"

extern "C" {

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	printf("DllMain called:\n");
	printf("Instance: %x\n", (DWORD)hinstDLL);
	printf("Reason:   %d\n", fdwReason);
	printf("Reserved: %x\n", (DWORD)lpvReserved);
	return 1;
}

SAMPLEDLL_API int addNumbers(int a, int b)
{
	return a + b;
}

}