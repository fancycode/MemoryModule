@echo off
setlocal

if /I "%PLATFORM%" == "x64" (
    set CMAKE_GEN_SUFFIX= Win64
    if /I "%GENERATOR%" == "Visual Studio 9 2008" (
        echo Skipping %CONFIGURATION% build using %GENERATOR%%CMAKE_GEN_SUFFIX% on %PLATFORM%
        exit 0
    )
) else (
    set CMAKE_GEN_SUFFIX=
)

echo.
echo Preparing %CONFIGURATION% build environment for %GENERATOR%%CMAKE_GEN_SUFFIX% ...
cmake "-G%GENERATOR%%CMAKE_GEN_SUFFIX%" -DPLATFORM=%PLATFORM% -DUNICODE=%UNICODE% -DTESTSUITE=ON -H. -Bbuild
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo Building ...
cmake --build build --config %CONFIGURATION%
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo Copying generated files ...
copy /y build\example\DllLoader\%CONFIGURATION%\DllLoader.exe build\example\DllLoader\ > NUL
copy /y build\example\DllLoader\%CONFIGURATION%\DllLoaderLoader.exe build\example\DllLoader\ > NUL
copy /y build\example\SampleDLL\%CONFIGURATION%\SampleDLL.dll build\example\SampleDLL\ > NUL
copy /y build\tests\%CONFIGURATION%\TestSuite.exe build\tests\ > NUL

cd build\example\DllLoader

echo.
echo Running DllLoader.exe ...
DllLoader.exe
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo Running DllLoaderLoader.exe ...
DllLoaderLoader.exe
if %errorlevel% neq 0 exit /b %errorlevel%

cd ..\..\tests

echo.
echo Running TestSuite.exe ...
TestSuite.exe
if %errorlevel% neq 0 exit /b %errorlevel%
