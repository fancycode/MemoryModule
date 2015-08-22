MemoryModule
============

[![Build Status](https://travis-ci.org/fancycode/MemoryModule.svg?branch=master)](https://travis-ci.org/fancycode/MemoryModule)[![Build status](https://ci.appveyor.com/api/projects/status/qcrfxbno0jbbl9cx/branch/master?svg=true)](https://ci.appveyor.com/project/fancycode/memorymodule)

The default windows API functions to load external libraries into a program
(`LoadLibrary`, `LoadLibraryEx`) only work with files on the filesystem.  It's
therefore impossible to load a DLL from memory.

But sometimes, you need exactly this functionality (e.g. you don't want to
distribute a lot of files or want to make disassembling harder).  Common
workarounds for this problems are to write the DLL into a temporary file
first and import it from there.  When the program terminates, the temporary
file gets deleted.

`MemoryModule` is a library that can be used to load a DLL completely from
memory - without storing on the disk first.

See `doc/readme.txt` for more informations about the format of a DLL file and
a tutorial how they can be loaded directly.
