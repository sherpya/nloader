Native win32 executables Loader
===============================

How it works
------------

The loader reads the target executable then it allocates and maps into memory
each of its sections.

It parses the import table and, for each required runtime library it
attempts to dlopen a corresponding shared lib. Since native executables run
inside a rather simple environment, the only runtime library they often rely upon
is ntdll.dll (which gets dlopen'd as ntdll.dll.so).

For each imported function a symbol lookup is performed and the
corresponding import table entry is patched with the address of a small wrapper
function. If the symbol is found the wrapper logs the call and jumps to the
actual target. If the symbol is missing, then the wrapper complains about that
and calls `abort()`.

It fills in the `PEB` and `TEB` structures and make sure the latter is
available via `fs:[]`.

Next it allocates the stack and map `KUSER_SHARED_DATA` at `0x7ffe0000` where some
static crt code expects it to be located.

Finally, it jumps to the `EP` and lets the alien code do its thing.


How to compile
--------------

You need CMake (>= 3.20) and yasm.

```
cmake -B build
cmake --build build -j
```

By default the build matches the host architecture (x86_64 or i386). To force
a 32-bit build on an x86_64 host pass `-DNLOADER_ARCH=i386` at configure time.

If you are on Linux OS and provide an autochk.exe by placing it at the toplevel
directory, the build will produce a standalone executable that also contains
the native Windows executable, suitable for a Live CD.

The current smoke-test target is the Windows 10 / 11 x86_64 autochk.exe.
The legacy Windows 7 SP1 x86 build also still works on i386: grab
`windows6.1-KB976932-X86.exe` and extract:
<pre>
cabextract -p -F "*autochk.exe" windows6.1-KB976932-X86.exe > autochk.exe
</pre>

To generate a small NTFS test image (`disk.img`, ~2 GB) build the
out-of-tree `disk_img` target — it shells out to `mkntfs`:
<pre>
cmake --build build --target disk_img
</pre>


How to run
----------

First of all you need a native executable. The repo's two main test paths use
autochk.exe (Windows 10 / 11 x86_64).

Then simply execute: `./nloader autochk.exe -p disk.img`

A few useful invocations:

<pre>
./nloader autochk.exe '*'              # query global autochk state, exits 0
./nloader autochk.exe -v -p disk.img   # verbose, force scan even if clean
</pre>

`-p` forces the check even if the disk is not flagged as dirty. On Windows 10+
`autochk -p` short-circuits with "The volume is clean" if the dirty bit
isn't set. To exercise the actual scan path against `disk.img`, set the dirty
bit first via the helper script (it has to be re-run before each scan because
autochk clears the flag at the end):

<pre>
python3 setdirty.py build/disk.img
./nloader autochk.exe -v -p disk.img
</pre>

You should get something like:

<pre>
Checking file system on disk.img
The type of the file system is NTFS.
Volume label is Native.

One of your disks needs to be checked for consistency. You
may cancel the disk check, but it is strongly recommended
that you continue.
Windows will now check the disk.

Stage 1: Examining basic file system structure ...
  27 file records processed.
File verification completed.
 Phase duration (File record verification): 0.76 milliseconds.
  0 large file records processed.
 Phase duration (Orphan file record recovery): 0.09 milliseconds.
  0 bad file records processed.
 Phase duration (Bad file record checking): 0.08 milliseconds.

Stage 2: Examining file name linkage ...
  39 index entries processed.
Index verification completed.
 Phase duration (Index verification): 1.93 milliseconds.
  0 unindexed files scanned.
 Phase duration (Orphan reconnection): 0.17 milliseconds.
  0 unindexed files recovered to lost and found.
 Phase duration (Orphan recovery to lost and found): 0.20 milliseconds.
  0 reparse records processed.
  0 reparse records processed.
 Phase duration (Reparse point and Object ID verification): 0.13 milliseconds.

Stage 3: Examining security descriptors ...
Windows has finished checking the disk.
</pre>

The 10-second "press any key to skip disk checking" countdown is
interactive — a key press during the window aborts the scan, just like
on Windows.

If you want to trace all api calls, you can enable debug logging via the NLOG
environment variable (`NLOG=all` for everything, or restrict to a subsystem
like `NLOG=rtl`, `NLOG=io`, `NLOG=nt`).
Note that this can result in a lot of spam, expecially if you set NLOG=all.


### guessed autochk.exe cmdline arguments

References:

 - [Chkdsk, ChkNTFS, and Autochk ](http://www.infocellar.com/winxp/chkdsk-and-autochk.htm)
 - [Using Chkntfs to Prevent Autochk from Running](http://windows-xp-dox.net/MS.Press-Microsoft.Windows.XP1/prkd_tro_rgwn.htm)
 - [Description of Enhanced Chkdsk, Autochk, and Chkntfs Tools in Windows 2000](http://support.microsoft.com/kb/218461)
 - [CHKNTFS.EXE: What You Can Use It For](http://support.microsoft.com/kb/160963/EN-US/)

<pre>
autochk.exe [switches] volume | * (* = all volumes, it queries global directory)

    -t           - unknown (w7)
    -s           - silent execution (w7)
    -p           - force check even if dirty bit is not set
    -r           - locate and recover bad sectors, implies -p (untested)
    -b           - re-evaluates bad clusters on the volume, implies -r (w7)
    -x volume    - force dismount (untested), without arguments crashes
    -lXX         - with some values like 10 prints info and log size of the volume (the value is mul by 1024)
    -l:XX        - tries to set log size, always fails for me saying the size is too small or unable to adjust
    -k:volume    - excludes volume from the check (does make sense when using *)
    -m           - run chkdsk only if dirty bit is set (default ?)
    -i           - Performs a less vigorous check of index entries
    -i[:XX]      - XX = 0-50 - unknown
    -c           - Skips the checking of cycles within the folder structure
</pre>


Compatibility
------------

We've tested a few native executable and the results are quite variable. Some
work perfectly, some aborts soon due to some unimplemented API, some totally
misbehave, likely due some limitations in the emu layer.

Current smoke-test status (x86_64):

 - `autochk.exe '*'` — exits 0 cleanly.
 - `autochk.exe -v -p disk.img` on a dirty image — full Stage 1/2/3 scan
   completes with "Windows has finished checking the disk."

The next known blocker is `libs/bcd` (`BcdOpenStore` is still a placeholder).
