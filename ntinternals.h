/*
 * Undocumented Windows structures
 * 
 * Found on http://undocumented.ntinternals.net/
 */

#ifndef __NT_INTERNALS
#define __NT_INTERNALS

#include <windows.h>
#include <winternl.h>

#ifndef UNICODE_STRING
// usually included in "winternl.h"

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

#endif

// constants from the article
// "What Goes On Inside Windows 2000: Solving the Mysteries of the Loader"
// by Russ Osterlund
// http://msdn.microsoft.com/msdnmag/issues/02/03/Loader/default.aspx
#define MAX_DLL_NAME_LENGTH	0x214

#define STATIC_LINK				0x00000002
#define IMAGE_DLL				0x00000004
#define LOAD_IN_PROGRESS		0x00001000
#define UNLOAD_IN_PROGRESS		0x00002000
#define ENTRY_PROCESSED			0x00004000
#define ENTRY_INSERTED			0x00008000
#define CURRENT_LOAD			0x00010000
#define FAILED_BUILTIN_LOAD		0x00020000
#define DONT_CALL_FOR_THREAD	0x00040000
#define PROCESS_ATTACH_CALLED	0x00080000
#define DEBUG_SYMBOLS_LOADED	0x00100000
#define IMAGE_NOT_AT_BASE		0x00200000
#define WX86_IGNORE_MACHINETYPE	0x00400000

/*
 * Documented by:
 *   Reactos
 *   Tomasz Nowak
 */
typedef struct _LDR_MODULE {
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID BaseAddress;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	SHORT LoadCount;
	SHORT TlsIndex;
	LIST_ENTRY HashTableEntry;
	ULONG TimeDateStamp;
} LDR_MODULE, *PLDR_MODULE;

/*
 * Documented by:
 *   Reactos
 *   Tomasz Nowak
 */
typedef struct _PEB_LDR_DATA {
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

/*
 * Documented by:
 *   Reactos
 */
typedef struct _RTL_DRIVE_LETTER_CURDIR {
	USHORT Flags;
	USHORT Length;
	ULONG TimeStamp;
	UNICODE_STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

/*
 * Documented by:
 *   Reactos
 *   Tomasz Nowak
 */
typedef struct _RTL_USER_PROCESS_PARAMETERS {
	ULONG MaximumLength;
	ULONG Length;
	ULONG Flags;
	ULONG DebugFlags;
	PVOID ConsoleHandle;
	ULONG ConsoleFlags;
	HANDLE StdInputHandle;
	HANDLE StdOutputHandle;
	HANDLE StdErrorHandle;
	UNICODE_STRING CurrentDirectoryPath;
	HANDLE CurrentDirectoryHandle;
	UNICODE_STRING DllPath;
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
	PVOID Environment;
	ULONG StartingPositionLeft;
	ULONG StartingPositionTop;
	ULONG Width;
	ULONG Height;
	ULONG CharWidth;
	ULONG CharHeight;
	ULONG ConsoleTextAttributes;
	ULONG WindowFlags;
	ULONG ShowWindowFlags;
	UNICODE_STRING WindowTitle;
	UNICODE_STRING DesktopName;
	UNICODE_STRING ShellInfo;
	UNICODE_STRING RuntimeData;
	RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

/*
 * Address of fast-locking routine for PEB
 */
typedef void (*PPEBLOCKROUTINE)(
    PVOID PebLock
);

typedef LPVOID *PPVOID;

/*
 * Structure PEB_FREE_BLOCK is used internally in PEB (Process Enviroment Block)
 * structure for describe free blocks in memory allocated for PEB.
 *
 * Documented by:
 *   Reactos
 */
typedef struct _PEB_FREE_BLOCK {
	struct _PEB_FREE_BLOCK *Next;
	ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

/*
 * Structure PEB (Process Enviroment Block) contains all User-Mode parameters
 * associated by system with current process.
 *
 * Documented by:
 *   Reactos
 *   Tomasz Nowak
 */
typedef struct _PEB {
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	BOOLEAN Spare;
	HANDLE Mutant;
	PVOID ImageBaseAddress;
	PPEB_LDR_DATA LoaderData;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	PVOID SubSystemData;
	PVOID ProcessHeap;
	PVOID FastPebLock;
	PPEBLOCKROUTINE FastPebLockRoutine;
	PPEBLOCKROUTINE FastPebUnlockRoutine;
	ULONG EnvironmentUpdateCount;
	PPVOID KernelCallbackTable;
	PVOID EventLogSection;
	PVOID EventLog;
	PPEB_FREE_BLOCK FreeList;
	ULONG TlsExpansionCounter;
	PVOID TlsBitmap;
	ULONG TlsBitmapBits[0x2];
	PVOID ReadOnlySharedMemoryBase;
	PVOID ReadOnlySharedMemoryHeap;
	PPVOID ReadOnlyStaticServerData;
	PVOID AnsiCodePageData;
	PVOID OemCodePageData;
	PVOID UnicodeCaseTableData;
	ULONG NumberOfProcessors;
	ULONG NtGlobalFlag;
	BYTE Spare2[0x4];
	LARGE_INTEGER CriticalSectionTimeout;
	ULONG HeapSegmentReserve;
	ULONG HeapSegmentCommit;
	ULONG HeapDeCommitTotalFreeThreshold;
	ULONG HeapDeCommitFreeBlockThreshold;
	ULONG NumberOfHeaps;
	ULONG MaximumNumberOfHeaps;
	PPVOID *ProcessHeaps;
	PVOID GdiSharedHandleTable;
	PVOID ProcessStarterHelper;
	PVOID GdiDCAttributeList;
	PVOID LoaderLock;
	ULONG OSMajorVersion;
	ULONG OSMinorVersion;
	ULONG OSBuildNumber;
	ULONG OSPlatformId; 
	ULONG ImageSubSystem;
	ULONG ImageSubSystemMajorVersion;
	ULONG ImageSubSystemMinorVersion;
	ULONG GdiHandleBuffer[0x22];
	ULONG PostProcessInitRoutine;
	ULONG TlsExpansionBitmap;
	BYTE TlsExpansionBitmapBits[0x80];
	ULONG SessionId;
} PEB, *PPEB;

#endif  // __NT_INTERNALS
