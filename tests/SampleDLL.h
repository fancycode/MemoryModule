extern "C" {

#ifdef SAMPLEDLL_EXPORTS
#define SAMPLEDLL_API __declspec(dllexport)
#else
#define SAMPLEDLL_API __declspec(dllimport)
#endif

SAMPLEDLL_API int addNumbers(int a, int b);

}
