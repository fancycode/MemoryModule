:author:    Joachim Bauch
:contact:   mail@joachim-bauch.de
:copyright: `Creative Commons License (by-sa)`__

__ http://creativecommons.org/licenses/by-sa/2.5/


.. contents::


Overview
=========

The default windows API functions to load external libraries into a program
(LoadLibrary, LoadLibraryEx) only work with files on the filesystem.  It's
therefore impossible to load a DLL from memory.
But sometimes, you need exactly this functionality (e.g. you don't want to
distribute a lot of files or want to make disassembling harder).  Common
workarounds for this problems are to write the DLL into a temporary file
first and import it from there.  When the program terminates, the temporary
file gets deleted.

In this tutorial, I will describe first, how DLL files are structured and
will present some code that can be used to load a DLL completely from memory -
without storing on the disk first.


Windows executables - the PE format
====================================

Most windows binaries that can contain executable code (.exe, .dll, .sys)
share a common file format that consists of the following parts:

+----------------+
| DOS header     |
|                |
| DOS stub       |
+----------------+
| PE header      |
+----------------+
| Section header |
+----------------+
| Section 1      |
+----------------+
| Section 2      |
+----------------+
| . . .          |
+----------------+
| Section n      |
+----------------+

All structures given below can be found in the header file `winnt.h`.


DOS header / stub
------------------

The DOS header is only used for backwards compatibility.  It precedes the DOS
stub that normally just displays an error message about the program not being
able to be run from DOS mode.

Microsoft defines the DOS header as follows::
    
    typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
        WORD   e_magic;                     // Magic number
        WORD   e_cblp;                      // Bytes on last page of file
        WORD   e_cp;                        // Pages in file
        WORD   e_crlc;                      // Relocations
        WORD   e_cparhdr;                   // Size of header in paragraphs
        WORD   e_minalloc;                  // Minimum extra paragraphs needed
        WORD   e_maxalloc;                  // Maximum extra paragraphs needed
        WORD   e_ss;                        // Initial (relative) SS value
        WORD   e_sp;                        // Initial SP value
        WORD   e_csum;                      // Checksum
        WORD   e_ip;                        // Initial IP value
        WORD   e_cs;                        // Initial (relative) CS value
        WORD   e_lfarlc;                    // File address of relocation table
        WORD   e_ovno;                      // Overlay number
        WORD   e_res[4];                    // Reserved words
        WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
        WORD   e_oeminfo;                   // OEM information; e_oemid specific
        WORD   e_res2[10];                  // Reserved words
        LONG   e_lfanew;                    // File address of new exe header
      } IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;


PE header
----------

The PE header contains informations about the different sections inside the
executable that are used to store code and data or to define imports from other
libraries or exports this libraries provides.

It's defined as follows::

    typedef struct _IMAGE_NT_HEADERS {
        DWORD Signature;
        IMAGE_FILE_HEADER FileHeader;
        IMAGE_OPTIONAL_HEADER32 OptionalHeader;
    } IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

The `FileHeader` describes the *physical* format of the file, i.e. contents, informations
about symbols, etc::

    typedef struct _IMAGE_FILE_HEADER {
        WORD    Machine;
        WORD    NumberOfSections;
        DWORD   TimeDateStamp;
        DWORD   PointerToSymbolTable;
        DWORD   NumberOfSymbols;
        WORD    SizeOfOptionalHeader;
        WORD    Characteristics;
    } IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

.. _OptionalHeader:

The `OptionalHeader` contains informations about the *logical* format of the library, 
including required OS version, memory requirements and entry points::

    typedef struct _IMAGE_OPTIONAL_HEADER {
        //
        // Standard fields.
        //
    
        WORD    Magic;
        BYTE    MajorLinkerVersion;
        BYTE    MinorLinkerVersion;
        DWORD   SizeOfCode;
        DWORD   SizeOfInitializedData;
        DWORD   SizeOfUninitializedData;
        DWORD   AddressOfEntryPoint;
        DWORD   BaseOfCode;
        DWORD   BaseOfData;
    
        //
        // NT additional fields.
        //
    
        DWORD   ImageBase;
        DWORD   SectionAlignment;
        DWORD   FileAlignment;
        WORD    MajorOperatingSystemVersion;
        WORD    MinorOperatingSystemVersion;
        WORD    MajorImageVersion;
        WORD    MinorImageVersion;
        WORD    MajorSubsystemVersion;
        WORD    MinorSubsystemVersion;
        DWORD   Win32VersionValue;
        DWORD   SizeOfImage;
        DWORD   SizeOfHeaders;
        DWORD   CheckSum;
        WORD    Subsystem;
        WORD    DllCharacteristics;
        DWORD   SizeOfStackReserve;
        DWORD   SizeOfStackCommit;
        DWORD   SizeOfHeapReserve;
        DWORD   SizeOfHeapCommit;
        DWORD   LoaderFlags;
        DWORD   NumberOfRvaAndSizes;
        IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
    } IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

.. _DataDirectory:

The `DataDirectory` contains 16 (`IMAGE_NUMBEROF_DIRECTORY_ENTRIES`) entries
defining the logical components of the library:

===== ==========================
Index Description
===== ==========================
0     Exported functions
----- --------------------------
1     Imported functions
----- --------------------------
2     Resources
----- --------------------------
3     Exception informations
----- --------------------------
4     Security informations
----- --------------------------
5     Base relocation table
----- --------------------------
6     Debug informations
----- --------------------------
7     Architecture specific data
----- --------------------------
8     Global pointer
----- --------------------------
9     Thread local storage
----- --------------------------
10    Load configuration
----- --------------------------
11    Bound imports
----- --------------------------
12    Import address table
----- --------------------------
13    Delay load imports
----- --------------------------
14    COM runtime descriptor
===== ==========================

For importing the DLL we only need the entries describing the imports and the
base relocation table.  In order to provide access to the exported functions,
the exports entry is required.


Section header
---------------

The section header is stored after the OptionalHeader_ structure in the PE
header.  Microsoft provides the macro `IMAGE_FIRST_SECTION` to get the start
address based on the PE header.

Actually, the section header is a list of informations about each section in
the file::

    typedef struct _IMAGE_SECTION_HEADER {
        BYTE    Name[IMAGE_SIZEOF_SHORT_NAME];
        union {
                DWORD   PhysicalAddress;
                DWORD   VirtualSize;
        } Misc;
        DWORD   VirtualAddress;
        DWORD   SizeOfRawData;
        DWORD   PointerToRawData;
        DWORD   PointerToRelocations;
        DWORD   PointerToLinenumbers;
        WORD    NumberOfRelocations;
        WORD    NumberOfLinenumbers;
        DWORD   Characteristics;
    } IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

A section can contain code, data, relocation informations, resources, export or
import definitions, etc.


Loading the library
====================

To emulate the PE loader, we must first understand, which steps are neccessary
to load the file to memory and prepare the structures so they can be called from
other programs.

When issuing the API call `LoadLibrary`, Windows basically performs these tasks:

1. Open the given file and check the DOS and PE headers.

2. Try to allocate a memory block of `PEHeader.OptionalHeader.SizeOfImage` bytes
   at position `PEHeader.OptionalHeader.ImageBase`.
   
3. Parse section headers and copy sections to their addresses.  The destination
   address for each section, relative to the base of the allocated memory block,
   is stored in the `VirtualAddress` attribute of the `IMAGE_SECTION_HEADER`
   structure.
   
4. If the allocated memory block differs from `ImageBase`, various references in
   the code and/or data sections must be adjusted.  This is called *Base
   relocation*.
   
5. The required imports for the library must be resolved by loading the
   corresponding libraries.
   
6. The memory regions of the different sections must be protected depending on
   the section's characteristics.  Some sections are marked as *discardable*
   and therefore can be safely freed at this point.  These sections normally
   contain temporary data that is only needed during the import, like the
   informations for the base relocation.
   
7. Now the library is loaded completely.  It must be notified about this by
   calling the entry point using the flag `DLL_PROCESS_ATTACH`.

In the following paragraphs, each step is described.


Allocate memory
----------------

All memory required for the library must be reserved / allocated using
`VirtualAlloc`, as Windows provides functions to protect these memory blocks.
This is required to restrict access to the memory, like blocking write access
to the code or constant data.

The OptionalHeader_ structure defines the size of the required memory block
for the library.  It must be reserved at the address specified by `ImageBase`
if possible::

    memory = VirtualAlloc((LPVOID)(PEHeader->OptionalHeader.ImageBase),
        PEHeader->OptionalHeader.SizeOfImage,
        MEM_RESERVE,
        PAGE_READWRITE);

If the reserved memory differs from the address given in `ImageBase`, base
relocation as described below must be done.


Copy sections
--------------

Once the memory has been reserved, the file contents can be copied to the
system.  The section header must get evaluated in order to determine the
position in the file and the target area in memory.

Before copying the data, the memory block must get committed::

    dest = VirtualAlloc(baseAddress + section->VirtualAddress,
        section->SizeOfRawData,
        MEM_COMMIT,
        PAGE_READWRITE);

Sections without data in the file (like data sections for the used variables)
have a `SizeOfRawData` of `0`, so you can use the `SizeOfInitializedData`
or `SizeOfUninitializedData` of the OptionalHeader_.  Which one must get
choosen depending on the bit flags `IMAGE_SCN_CNT_INITIALIZED_DATA` and
`IMAGE_SCN_CNT_UNINITIALIZED_DATA` that may be set in the section`s
characteristics.


Base relocation
----------------

All memory addresses in the code / data sections of a library are stored relative
to the address defined by `ImageBase` in the OptionalHeader_.  If the library
can't be imported to this memory address, the references must get adjusted
=> *relocated*.  The file format helps for this by storing informations about
all these references in the base relocation table, which can be found in the
directory entry 5 of the DataDirectory_ in the OptionalHeader_.

This table consists of a series of this structure

::

    typedef struct _IMAGE_BASE_RELOCATION {
        DWORD   VirtualAddress;
        DWORD   SizeOfBlock;
    } IMAGE_BASE_RELOCATION;

It contains `(SizeOfBlock - IMAGE_SIZEOF_BASE_RELOCATION) / 2` entries of 16 bits
each.  The upper 4 bits define the type of relocation, the lower 12 bits define
the offset relative to the `VirtualAddress`.

The only types that seem to be used in DLLs are

IMAGE_REL_BASED_ABSOLUTE
    No operation relocation.  Used for padding.
IMAGE_REL_BASED_HIGHLOW
    Add the delta between the `ImageBase` and the allocated memory block to the
    32 bits found at the offset.


Resolve imports
----------------

The directory entry 1 of the DataDirectory_ in the OptionalHeader_ specifies
a list of libraries to import symbols from.  Each entry in this list is defined
as follows::

    typedef struct _IMAGE_IMPORT_DESCRIPTOR {
        union {
            DWORD   Characteristics;            // 0 for terminating null import descriptor
            DWORD   OriginalFirstThunk;         // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
        };
        DWORD   TimeDateStamp;                  // 0 if not bound,
                                                // -1 if bound, and real date\time stamp
                                                //     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
                                                // O.W. date/time stamp of DLL bound to (Old BIND)
    
        DWORD   ForwarderChain;                 // -1 if no forwarders
        DWORD   Name;
        DWORD   FirstThunk;                     // RVA to IAT (if bound this IAT has actual addresses)
    } IMAGE_IMPORT_DESCRIPTOR;

The `Name` entry describes the offset to the NULL-terminated string of the library
name (e.g. `KERNEL32.DLL`).  The `OriginalFirstThunk` entry points to a list
of references to the function names to import from the external library.
`FirstThunk` points to a list of addresses that gets filled with pointers to
the imported symbols.

When we resolve the imports, we walk both lists in parallel, import the function
defined by the name in the first list and store the pointer to the symbol in the
second list::

    nameRef = (DWORD *)(baseAddress + importDesc->OriginalFirstThunk);
    symbolRef = (DWORD *)(baseAddress + importDesc->FirstThunk);
    for (; *nameRef; nameRef++, symbolRef++)
    {
        PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME)(codeBase + *nameRef);
        *symbolRef = (DWORD)GetProcAddress(handle, (LPCSTR)&thunkData->Name);
        if (*funcRef == 0)
        {
            handleImportError();
            return;
        }
    }


Protect memory
---------------

Every section specifies permission flags in it's `Characteristics` entry.
These flags can be one or a combination of

IMAGE_SCN_MEM_EXECUTE
    The section contains data that can be executed.
    
IMAGE_SCN_MEM_READ
    The section contains data that is readable.
    
IMAGE_SCN_MEM_WRITE
    The section contains data that is writeable.

These flags must get mapped to the protection flags

- PAGE_NOACCESS
- PAGE_WRITECOPY
- PAGE_READONLY
- PAGE_READWRITE
- PAGE_EXECUTE
- PAGE_EXECUTE_WRITECOPY
- PAGE_EXECUTE_READ
- PAGE_EXECUTE_READWRITE

Now, the function `VirtualProtect` can be used to limit access to the memory.
If the program tries to access it in a unauthorized way, an exception gets
raised by Windows.

In addition the section flags above, the following can be added:

IMAGE_SCN_MEM_DISCARDABLE
    The data in this section can be freed after the import.  Usually this is
    specified for relocation data.
    
IMAGE_SCN_MEM_NOT_CACHED
    The data in this section must not get cached by Windows.  Add the bit
    flag `PAGE_NOCACHE` to the protection flags above.


Notify library
---------------

The last thing to do is to call the DLL entry point (defined by
`AddressOfEntryPoint`) and so notifying the library about being attached
to a process.

The function at the entry point is defined as

::
    
    typedef BOOL (WINAPI *DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

So the last code we need to execute is

::

	DllEntryProc entry = (DllEntryProc)(baseAddress + PEHeader->OptionalHeader.AddressOfEntryPoint);
	(*entry)((HINSTANCE)baseAddress, DLL_PROCESS_ATTACH, 0);

Afterwards we can use the exported functions as with any normal library.


Exported functions
===================

If you want to access the functions that are exported by the library, you need to find the entry
point to a symbol, i.e. the name of the function to call.

The directory entry 0 of the DataDirectory_ in the OptionalHeader_ contains informations about
the exported functions. It's defined as follows::

    typedef struct _IMAGE_EXPORT_DIRECTORY {
        DWORD   Characteristics;
        DWORD   TimeDateStamp;
        WORD    MajorVersion;
        WORD    MinorVersion;
        DWORD   Name;
        DWORD   Base;
        DWORD   NumberOfFunctions;
        DWORD   NumberOfNames;
        DWORD   AddressOfFunctions;     // RVA from base of image
        DWORD   AddressOfNames;         // RVA from base of image
        DWORD   AddressOfNameOrdinals;  // RVA from base of image
    } IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

First thing to do, is to map the name of the function to the ordinal number of the exported
symbol. Therefore, just walk the arrays defined by `AddressOfNames` and `AddressOfNameOrdinals`
parallel until you found the required name.

Now you can use the ordinal number to read the address by evaluating the n-th element of the
`AddressOfFunctions` array.


Freeing the library
====================

To free the custom loaded library, perform the steps

- Call entry point to notify library about being detached::

	DllEntryProc entry = (DllEntryProc)(baseAddress + PEHeader->OptionalHeader.AddressOfEntryPoint);
	(*entry)((HINSTANCE)baseAddress, DLL_PROCESS_ATTACH, 0);
    
- Free external libraries used to resolve imports.
- Free allocated memory.


MemoryModule
=============

MemoryModule is a C-library that can be used to load a DLL from memory.

The interface is very similar to the standard methods for loading of libraries::

    typedef void *HMEMORYMODULE;
    
    HMEMORYMODULE MemoryLoadLibrary(const void *);
    FARPROC MemoryGetProcAddress(HMEMORYMODULE, const char *);
    void MemoryFreeLibrary(HMEMORYMODULE);


Downloads
----------

The latest development release can always be grabbed from Github at
http://github.com/fancycode/MemoryModule/


Known issues
-------------

- All memory that is not protected by section flags is gets committed using `PAGE_READWRITE`.
  I don't know if this is correct.


License
--------

Since version 0.0.2, the MemoryModule library is released under the Mozilla Public License (MPL).
Version 0.0.1 has been released unter the Lesser General Public License (LGPL).

It is provided as-is without ANY warranty.  You may use it at your own risk.


Copyright
==========

The MemoryModule library and this tutorial are
Copyright (c) 2004-2012 by Joachim Bauch.
