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
#include <string.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>

#include "nt_structs.h"

MODULE(main)

extern uint8_t autochk_data[] asm("_binary_autochk_exe_start");
extern uint8_t autochk_data_size[] asm("_binary_autochk_exe_size");
extern uint8_t autochk_data_end[] asm("_binary_autochk_exe_end");

static jmp_buf sigsegv_env;
static void sigsegv_handler(int signum)
{
    longjmp(sigsegv_env, 1);
}

static void usage(void) {
    printf(
    "\nautochk.exe cmdline arguments (guessed)\n"
    " refs:\n"
    "  - http://www.infocellar.com/winxp/chkdsk-and-autochk.htm\n"
    "  - http://windows-xp-dox.net/MS.Press-Microsoft.Windows.XP1/prkd_tro_rgwn.htm\n"
    "  - http://support.microsoft.com/kb/218461\n"
    "  - http://support.microsoft.com/kb/160963/EN-US/\n"
    "\n"
    "autochk.exe [switches] volume | *\n"
    " * = all volumes, it queries global directory\n"
    "\n"
    " -t           - unknown (w7)\n"
    " -s           - silent execution (w7)\n"
    " -p           - force check even if dirty bit is not set\n"
    " -r           - locate and recover bad sectors, implies -p (untested)\n"
    " -b           - re-evaluates bad clusters on the volume, implies -r (w7)\n"
    " -x volume    - force dismount (untested), without arguments crashes\n"
    " -lXX         - with some values like 10 prints info and log size of the volume (the value is mul by 1024)\n"
    " -l:XX        - tries to set log size, always fails for me saying the size is too small or unable to adjust\n"
    " -k:volume    - excludes volume from the check (does make sense when using *)\n"
    " -m           - run chkdsk only if dirty bit is set (default ?)\n"
    " -i           - Performs a less vigorous check of index entries\n"
    " -i[:XX]      - XX = 0-50 - unknown\n"
    " -c           - Skips the checking of cycles within the folder structure\n"
    "\n"
    "NOTE: I'm not responsable at all for any damage you can make on your filesystem\n"
    ". (a dot) means the filesystem is clean, if you want to force the check you should specify -p cmdline option\n"
    "\n"
    );

    exit(1);
}

int main (int argc, char **argv) {

    unsigned int i;
    WCHAR commandline[1024] = L"autochk.exe *";

    uint8_t *ptr;
    void *ep;
    RTL_USER_PROCESS_PARAMETERS *params;
    size_t autochk_size = (size_t)((void *) autochk_data_size);

    const char *executable = "autochk.exe";

    ep = setup_nloader(autochk_data, autochk_size, &params, 1);

    ptr = (uint8_t *)(params+1);
    params->ImagePathName.Buffer = (WCHAR *) ptr;

    for (i = 0; i < strlen(executable); i++)
        params->ImagePathName.Buffer[i] = executable[i];

    params->ImagePathName.Length = i;
    params->ImagePathName.MaximumLength = i + 1;


    if (argc > 1)
    {
        int c = params->ImagePathName.Length;
        memcpy(commandline, params->ImagePathName.Buffer, params->ImagePathName.Length * sizeof(WCHAR));

        for (i = 1; i < (unsigned) argc; i++)
        {
            char *arg = argv[i];
            int dev = 0;
            commandline[c++] = L' ';
            if (!strncmp(arg, "/dev/", 4))
            {
                size_t len = sizeof(VOLUME_PREFIX) - sizeof(WCHAR);
                dev = 1;
                memcpy(&commandline[c], VOLUME_PREFIX, len);
                len /= sizeof(WCHAR);
                c += len;
            }

            while (*arg && (c < sizeof(commandline) - 1))
                commandline[c++] = *arg++;

            if (dev) commandline[c++] = L'}';
        }
        commandline[c++] = 0;
    }
    else
        usage();

    ptr += sizeof(WCHAR);
    memcpy(ptr, commandline, sizeof(commandline));
    params->CommandLine.Length = sizeof(commandline) - sizeof(WCHAR);
    params->CommandLine.MaximumLength = sizeof(commandline);
    params->CommandLine.Buffer = (WCHAR *) ptr;

    /******************************************************************************************************* CALL ENTRYPOINT */
    signal(SIGSEGV, sigsegv_handler);

    if (setjmp(sigsegv_env))
    {
        /* clean up console */
        fflush(stdout);
        fprintf(stderr, "Native program crashed\n");
        exit(1);
    }
    else
    {
        BPX();
        return to_ep(ep);
    }
}
