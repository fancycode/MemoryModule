#include "SampleDLL.h"

extern "C" {

SAMPLEDLL_API int addNumbers(int a, int b)
{
	return a + b;
}

}
