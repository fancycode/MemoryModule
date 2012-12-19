// Figuring out how I want to implement this.
typedef struct {
	int placeholder;
} MEMORYMODULESET, *PMEMORYMODULESET;

typedef struct {
	PMEMORYMODULESET head;
	PIMAGE_NT_HEADERS headers;
	unsigned char *codeBase;
	HMODULE *modules;
	int numModules;
	int initialized;
} MEMORYMODULEENTRY, *PMEMORYMODULEENTRY;