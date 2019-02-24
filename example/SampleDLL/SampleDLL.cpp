#include "SampleDLL.h"

extern "C" {

SAMPLEDLL_API int addNumbers(int a, int b)
{
    return a + b;
}

#ifdef _WIN64
SAMPLEDLL_API void throwException(void)
{
    throw 42;
}
#endif

}
