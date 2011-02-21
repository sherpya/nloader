/*
 * Copyright (c) 2010, Sherpya <sherpya@netfarm.it>, aCaB <acab@0xacab.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holders nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "nt_structs.h"

MODULE(loader)

struct pe_image_file_hdr pe_hdr;
struct pe_image_optional_hdr32 pe_opt;
struct pe_image_section_hdr sections[16];

char **import_names = NULL;
void **import_addrs = NULL;
uint8_t *stubs = NULL;
uint32_t *thunks = NULL;
unsigned int nfuncs = 0;

extern void stub(void);
extern unsigned int sizeof_stub(void);

static int log_always = 0;
static char **modules = NULL;
static void InitLogModules(void)
{
    char *tok, *loglist;

    if (!(loglist = getenv("NLOG")))
        return;

    if ((tok = strtok(loglist, ",")))
    {
        size_t i = 0;
        modules = malloc(sizeof(char **));
        do
        {
            if (!strcmp(tok, "all"))
                log_always = 1;

            modules[i] = strdup(tok);
            modules = realloc(modules, (i + 2) * sizeof(char **));
            i++;
        } while ((tok = strtok(NULL, ",")));
        modules[i] = NULL;
    }
}

static void _LogModule(const char *module, const char *format, ...)
{
    int i;
    for (i = 0; modules[i]; i++)
    {
        if (log_always || !strcmp(modules[i], module))
        {
            va_list argptr;
            va_start(argptr, format);
            vfprintf(stderr, format, argptr);
            va_end(argptr);
            break;
        }
    }
}


#define GETB(dest, size) ((offset + size > mod_size) ? NULL : memcpy(dest, mod_start + offset, size), offset += size)

static void _NoLogModule(const char *module, const char *format, ...) {}

void *setup_nloader(void *mod_start, size_t mod_size, PRTL_USER_PROCESS_PARAMETERS *pparams, int standalone) {
    unsigned int i;
#ifndef _WIN32
    struct modify_ldt_s fs_ldt;
    PEB_LDR_DATA ldr_data;
#endif
    TEB *teb;

    uint32_t pe_off, image_size = 0, min_rva = -1;
    uint8_t *image;
    off_t offset = 0;
    RTL_USER_PROCESS_PARAMETERS *params;

    //printf("Stub size :%x\n", sizeof_stub());

    /******************************************************************************************************* MZ HEADER */
    if(!GETB(&pe_off, sizeof(pe_off))) {
	perror("read pe magic");
	return NULL;
    }
    if((pe_off & 0xffff) != 0x5a4d) {
	printf("bad mz magic\n");
	return NULL;
    }
    offset = 0x3c;
    if(!GETB(&pe_off, sizeof(pe_off))) {
	perror("read pe off");
	return NULL;
    }

    /******************************************************************************************************* PE HEADER */
    offset = pe_off;
    if(!GETB(&pe_hdr, sizeof(pe_hdr))) {
	perror("read pe hdr");
	return NULL;
    }
    if(pe_hdr.Magic != 0x4550) {
	printf("bad pe magic\n");
	return NULL;
    }
    if(pe_hdr.Machine != 0x14c) {
	printf("bad arch\n");
	return NULL;
    }
    if(!pe_hdr.NumberOfSections || pe_hdr.NumberOfSections > 16) {
	printf("bad section count\n");
	return NULL;
    }
    if(pe_hdr.SizeOfOptionalHeader < sizeof(pe_opt)) {
	printf("opt hdr too small: %d vs %u\n", pe_hdr.SizeOfOptionalHeader, (int) sizeof(pe_opt));
	return NULL;
    }

    /******************************************************************************************************* OPT HEADER */
    if(!GETB(&pe_opt, sizeof(pe_opt))) {
	perror("read pe opt");
	return NULL;
    }
    if(pe_opt.Magic != 0x10b) {
	printf("bad opt magic\n");
	return NULL;
    }
    if(pe_opt.Subsystem != 1) {
	printf("subsystem is not native\n");
	return NULL;
    }
    if(pe_opt.NumberOfRvaAndSizes != 16) {
	printf("unexpected dir count\n");
	return NULL;
    }
    if(pe_opt.ImageBase % getpagesize()) {
	printf("unaligned image base not supported - FIXME\n");
	return NULL;
    }
    if(pe_opt.SectionAlignment % getpagesize()) {
	printf("section alignement not supported - FIXME\n");
	return NULL;
    }

    /******************************************************************************************************* SECTIONS - mapping */
    if(!GETB(sections, sizeof(sections[0]) * pe_hdr.NumberOfSections)) {
	perror("read sections");
	return NULL;
    }
    for(i = 0; i<pe_hdr.NumberOfSections; i++) {
	uint32_t rva = PEALIGN(sections[i].VirtualAddress, pe_opt.SectionAlignment);
	uint32_t vsz = PESALIGN(sections[i].VirtualSize, pe_opt.SectionAlignment);

	if(rva < min_rva)
	    min_rva = rva;
	if(image_size < rva + vsz)
	    image_size = rva + vsz;
    }
    image = mmap((void *)pe_opt.ImageBase, image_size, PROT_READ|PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);

    if(image == MAP_FAILED) {
	perror("mmap image");
	return NULL;
    }
    for(i = 0; i<pe_hdr.NumberOfSections; i++) {
	uint32_t rva = PEALIGN(sections[i].VirtualAddress, pe_opt.SectionAlignment);
	uint32_t vsz = PESALIGN(sections[i].VirtualSize, pe_opt.SectionAlignment);
	uint32_t raw = PEALIGN(sections[i].PointerToRawData, pe_opt.FileAlignment);
	uint32_t rsz = PESALIGN(sections[i].SizeOfRawData, pe_opt.FileAlignment);
	char name[9];

	memcpy(name, sections[i].Name, 8);
	name[8] = '\0';
    offset = raw;
	if(!GETB(image + rva, rsz)) {
	    perror("read section");
	    return NULL;
	}
	printf("Mapped section: %8s - RVA: %x -> %x - Raw: %x -> %x\n", name, rva, rva + vsz, raw, raw+rsz);
    }


    /******************************************************************************************************* IAT - libraries */
    while(pe_opt.DataDirectory[1].Size >= sizeof(struct IMAGE_IMPORT)) {
	struct IMAGE_IMPORT *import = (struct IMAGE_IMPORT *) (image + pe_opt.DataDirectory[1].VirtualAddress);
	uint32_t *names, *addrs, dllnamelen;
	char *dllname;
	void *handle;

	pe_opt.DataDirectory[1].Size -= sizeof(struct IMAGE_IMPORT);
	pe_opt.DataDirectory[1].VirtualAddress += sizeof(struct IMAGE_IMPORT);

	if(!CLI_ISCONTAINED(image, image_size, (uint8_t *)import, sizeof(import))) {
	    printf("import descriptor out of image\n");
	    return NULL;
	}
	if(!import->DllName)
	    break;
	if(!import->OrigThunk) {
	    printf("borland-like import table is not supported\n");
	    return NULL;
	}
	names = (uint32_t *)(image + import->OrigThunk);
	addrs = (uint32_t *)(image + import->Thunk);
	dllname = (char *) (image + import->DllName);
	dllnamelen = strlen(dllname);

	handle = load_library(standalone ? NULL : dllname);

	if(!handle) {
	    printf("Loading library %s.so failed with %s, aborting\n", dllname, dlerror());
	    return NULL;
	}
	else if (!standalone)
	    printf("Loading %s.so and resolving imports\n", dllname);

	/******************************************************************************************************* IAT - functions */
	while(1) {
	    char *fname, *impname;
	    int free_impname = 0;
	    void *api_addr;

	    if(!*names)
		break;
	    if(!CLI_ISCONTAINED(image, image_size, (uint8_t *)names, sizeof(*names)) || !CLI_ISCONTAINED(image, image_size, (uint8_t *)addrs, sizeof(*addrs))) {
		printf("Thunk out of image\n");
		return NULL;
	    }
	    if(*names & 0x80000000) {
		impname = malloc(8);
		if(!impname) {
		    perror("malloc impname");
		    return NULL;
		}
		sprintf(impname, "ord_%03u", *names & ~0x80000000);
		free_impname = 1;
	    } else
		impname = (char *) (image + 2 + *names);

	    fname = malloc(dllnamelen + strlen(impname) + 2);
	    if(!fname) {
		perror("malloc fname");
		return NULL;
	    }
	    sprintf(fname, "%s!%s", dllname, impname);

	    nfuncs++;

	    import_names = realloc(import_names, nfuncs * sizeof(*import_names));
	    if(!import_names) {
		perror("realloc import_names");
		return NULL;
	    }
	    import_names[nfuncs-1] = fname;

	    import_addrs = realloc(import_addrs, nfuncs * sizeof(*import_addrs));
	    if(!import_addrs) {
		perror("realloc import_addrs");
		return NULL;
	    }
	    if(handle) {
		char *mangled_name = malloc(strlen(impname) + strlen("rpl_") + 1);
		if(!mangled_name) {
		    perror("malloc mangled name");
		    return NULL;
		}
		sprintf(mangled_name, "rpl_%s", impname);
#ifndef _WIN32
		dlerror();
#endif
		api_addr = dlsym(handle, mangled_name);
		free(mangled_name);
		if(dlerror()) {
#ifndef _WIN32
		    dlerror();
#endif
		    api_addr = dlsym(handle, impname);
		}
		if(dlerror())
		    api_addr = NULL;
#ifndef _WIN32
		else {
		    Dl_info di;
		    if(!dladdr(api_addr, &di))
			api_addr = NULL;
		    // FIXME: check against full path of argv[0]
		    else if(!standalone && (!di.dli_fname || !strstr(di.dli_fname, dllname)))
			api_addr = NULL;
		}
#endif
	    } else
		api_addr = NULL;
	    import_addrs[nfuncs-1] = api_addr;
 	    if (!standalone) printf("- %s%s\n", fname, api_addr ? "" : " (stub)");
	    if(free_impname)
		free(impname);

	    thunks = realloc(thunks, nfuncs * sizeof(*thunks));
	    if(!thunks) {
		perror("realloc thunks");
		return NULL;
	    }
	    thunks[nfuncs-1] = (uint32_t)addrs;

	    names++;
	    addrs++;
	}
    }


    /******************************************************************************************************* IAT - patching */
    if(nfuncs) {
    	unsigned int szstub = sizeof_stub();
	unsigned int stubslen = PESALIGN(nfuncs * szstub, getpagesize());

    	stubs = mmap(NULL, stubslen, PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    	if(stubs == MAP_FAILED) {
    	    perror("mmap stubs");
	    return NULL;
    	}
    	for(i=0; i<nfuncs; i++) {
	    uint32_t *thunk = (uint32_t *)thunks[i];
    	    void *stub_addr = stubs + i * szstub;
	    if(!CLI_ISCONTAINED(image, image_size, (uint8_t *)thunk, sizeof(*thunk))) {
		printf("api addr for %s is out of image\n", import_names[i]);
		return NULL;
	    }
	    memcpy(stub_addr, stub, sizeof_stub());
	    *thunk = (uint32_t)stub_addr;
    	}
	free(thunks);
	if(mprotect(stubs, stubslen, PROT_READ|PROT_EXEC) == -1) {
	    perror("mprotect stubs");
	    return NULL;
	}
    }


    /******************************************************************************************************* MZ - mapping */
    offset = 0;
    pe_off = (min_rva < pe_off + pe_opt.SizeOfHeaders) ? min_rva : pe_off + pe_opt.SizeOfHeaders;
    if(!GETB(image, pe_off)) {
	perror("read header");
	return NULL;
    }
    if(mprotect(image, pe_off, PROT_READ) == -1) {
	perror("mprotect header");
	return NULL;
    }


    /******************************************************************************************************* PE - mapping */

    /******************************************************************************************************* SECTIONS - protection */
    for(i = 0; i<pe_hdr.NumberOfSections; i++) {
	uint32_t rva = PEALIGN(sections[i].VirtualAddress, pe_opt.SectionAlignment);
	uint32_t vsz = PESALIGN(sections[i].VirtualSize, pe_opt.SectionAlignment);
	char name[9], perms[4] = "---";
	int prot = 0;

	memcpy(name, sections[i].Name, 8);
	name[8] = '\0';
	if(sections[i].Characteristics & 0x20000000) {
	    prot |= PROT_EXEC;
	    perms[2] = 'x';
	}
	if(sections[i].Characteristics & 0x40000000) {
	    prot |= PROT_READ;
	    perms[0] = 'r';
	}
	if(sections[i].Characteristics & 0x80000000) {
	    prot |= PROT_WRITE;
	    perms[1] = 'w';
	}
	if(mprotect(image + rva, vsz, prot) == -1) {
	    perror("mprotect sections");
	    return NULL;
	}
	printf("Protection for section %-8s set to %s\n", name, perms);
    }

#ifndef _WIN32
    PEB peb;

    /******************************************************************************************************* TEB setup - main */
    teb = mmap((void *)0x7efdd000, getpagesize(),  PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if(teb == MAP_FAILED) {
	perror("mmap teb");
	return NULL;
    }

    /******************************************************************************************************* TEB setup - map teb to fs:0 */
    fs_ldt.entry_number = TEB_SEL_IDX;
    fs_ldt.base_addr = (unsigned long)teb;
    fs_ldt.limit = 1;
    fs_ldt.limit_in_pages = 1;
    fs_ldt.seg_32bit = 1;
    fs_ldt.contents = MODIFY_LDT_CONTENTS_DATA;
    fs_ldt.read_exec_only = 0;
    fs_ldt.seg_not_present = 0;
    fs_ldt.useable = 1;
    if(modify_ldt(1, &fs_ldt, sizeof(fs_ldt)) == -1) {
	perror("modify_ldt");
	return NULL;
    }

    /* FIXME - fill in TEB data, SEH chain, PEB, etc here */
    teb->NtTib.ExceptionList = NULL; // filled
    teb->NtTib.StackBase = (void *)0x230000;
    teb->NtTib.StackLimit = teb->NtTib.StackBase - PESALIGN(pe_opt.SizeOfStackReserve, getpagesize());
    teb->NtTib.StackLimit = mmap(teb->NtTib.StackLimit, PESALIGN(pe_opt.SizeOfStackReserve, getpagesize()), PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

    if(teb->NtTib.StackLimit == MAP_FAILED) {
	perror("mmap stack");
	return NULL;
    }

    teb->NtTib.SubSystemTib = NULL;
    teb->NtTib.Version = 7680; // on my XP 64bit (sizeof of the whole struct ? */
    teb->NtTib.ArbitraryUserPointer = NULL;
    teb->NtTib.Self = (PNT_TIB) teb;

    teb->EnvironmentPointer = NULL;
    teb->ClientId.UniqueProcess = NtCurrentProcess();
    teb->ClientId.UniqueThread = NtCurrentThread();
    teb->Peb = &peb;
    teb->LastErrorValue = 0;
    teb->CountOfOwnedCriticalSections = 0;
    teb->CurrentLocale = 1033; // FIXME: guess by looking at message res if any?
    teb->FpSoftwareStatusRegister = 0;
    teb->ExceptionCode = 0;
    teb->LastStatusValue = 0; // STATUS_SUCCESS
    memcpy(teb->StaticUnicodeBuffer, L"1337", 10);
    teb->StaticUnicodeString.Buffer = teb->StaticUnicodeBuffer;
    teb->StaticUnicodeString.Length = 10;
    teb->StaticUnicodeString.MaximumLength = sizeof(teb->StaticUnicodeBuffer);
    // teb->DeallocationStack = (PVOID) 0x30000;

    // PEB

    // win2k3
    peb.OSMajorVersion = 5;
    peb.OSMinorVersion = 2;
    peb.OSBuildNumber  = 3790;
    peb.OSPlatformId   = 2;

    peb.InheritedAddressSpace = 0;
    peb.ReadImageFileExecOptions = 0;
    peb.BeingDebugged = 0;
    peb.Mutant = INVALID_HANDLE_VALUE;

    peb.NumberOfProcessors = 1;

    ldr_data.Length = sizeof(ldr_data);
    ldr_data.Initialized = 1;
    ldr_data.SsHandle = 0;
    peb.LdrData = &ldr_data;

    // TBD ldr_data.InLoadOrderModuleList;
    // TBD ldr_data.InMemoryOrderModuleList;
    // TBD ldr_data.InInitializationOrderModuleList;

    /* locale info */
    peb.AnsiCodePageData     = NULL; /* c_1252.nls */
    peb.OemCodePageData      = NULL; /* c_850.nls  */
    peb.UnicodeCaseTableData = NULL; /* l_intl.nls */

    *pparams = params = mmap((void *)0x20000, getpagesize(), PROT_READ|PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
    if(params == MAP_FAILED) {
	perror("mmap params");
	return NULL;
    }

    peb.ProcessParameters = params;

    params->AllocationSize = getpagesize();
    params->Size = sizeof(*params);
    params->Flags = PARAMS_ALREADY_NORMALIZED;
    params->DebugFlags = 0; /* TRUE will raise int3 before main() */

    params->ConsoleHandle = INVALID_HANDLE_VALUE;
    params->ConsoleFlags = 0;
    params->hStdInput = (HANDLE)3;
    params->hStdOutput = (HANDLE)7;
    params->hStdError = (HANDLE)11;

    params->CurrentDirectory.DosPath.Length = 0;
    params->CurrentDirectory.DosPath.MaximumLength = 0;
    params->CurrentDirectory.DosPath.Buffer = NULL;
    params->CurrentDirectory.Handle = INVALID_HANDLE_VALUE;
    params->DllPath.Length = 0;
    params->DllPath.MaximumLength = 0;
    params->DllPath.Buffer = NULL;

    /* ImagePathName and CommandLine filled later */

    peb.SubSystemData = NULL;
    peb.ProcessHeap = HANDLE_HEAP;
    peb.EnvironmentUpdateCount = 1;

    /******************************************************************************************************* KUSER_SHARED_DATA (0x7ffe0000) */
    teb->SharedUserData = mmap((void *)USER_SHARED_DATA, getpagesize(), PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
    if(teb->SharedUserData == MAP_FAILED) {
	perror("mmap timer");
	return NULL;
    }

    // used by _security_init_cookie_ex
    teb->SharedUserData->TickCountMultiplier = 1;
    teb->SharedUserData->TickCount.High2Time = 0x1337;
    teb->SharedUserData->TickCount.High1Time = 0x1337;
    teb->SharedUserData->Cookie = 0x1337;

    teb->SharedUserData->NtProductType = NtProductWinNt;
    teb->SharedUserData->ProductTypeIsValid = TRUE;
    teb->SharedUserData->NtMajorVersion = peb.OSMajorVersion;
    teb->SharedUserData->NtMinorVersion = peb.OSMinorVersion;
    teb->SharedUserData->AlternativeArchitecture = StandardDesign;

    // usually READONLY
    if(mprotect(teb->SharedUserData, getpagesize(), PROT_READ) == -1) {
        perror("mprotect SharedUserData");
        return NULL;
    }

    __asm__ volatile( "movl %0,%%eax; movw %%ax, %%fs" : : "r" (LDT_SEL(fs_ldt.entry_number)) :"eax");
    //__asm__ volatile( "movl %0,%%fs" : : "a" (LDT_SEL(fs_ldt.entry_number)));

#else
    teb = NtCurrentTeb();
    teb->SharedUserData = (PKUSER_SHARED_DATA) USER_SHARED_DATA;
    params = NtCurrentPeb()->ProcessParameters;
#endif

    /* needed for RtlFindMessage */
    NtCurrentPeb()->ImageBaseAddress = (HMODULE) (pe_opt.ImageBase);

    params->Environment = NULL; // ENV is mapped at 0x10000

    // Shared log function
    InitLogModules();
    teb->LogModule = modules ? _LogModule : _NoLogModule;

    return image + pe_opt.AddressOfEntryPoint;
}
