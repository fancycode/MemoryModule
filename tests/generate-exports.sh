#!/bin/sh

##
## Generate header file.
##

cat > SampleExports.h << EOF
extern "C" {

#ifdef SAMPLEDLL_EXPORTS
#define SAMPLEDLL_API __declspec(dllexport)
#else
#define SAMPLEDLL_API __declspec(dllimport)
#endif

EOF

for i in `seq 1 100`;
do
cat >> SampleExports.h << EOF
SAMPLEDLL_API int add$i(int a);
EOF
done

cat >> SampleExports.h << EOF
}
EOF


##
## Generate source file.
##

cat > SampleExports.cpp << EOF
#include "SampleExports.h"

extern "C" {
EOF

for i in `seq 1 100 | sort -R`;
do
cat >> SampleExports.cpp << EOF
SAMPLEDLL_API int add$i(int a)
{
    return a + $i;
}
EOF
done

cat >> SampleExports.cpp << EOF
}
EOF
