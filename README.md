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

You need GNU make (sometimes called gmake) and yasm.
Simply run make from the top source directory.

If you are on Linux OS and provide an autochk.exe by placing it at the toplevel
directory, the makefile will make for you a standalone executable that also contains
the native Windows executable, suitable for a Live CD.


How to run
----------

First of all you need a native executable. If you want to run autochk.exe pick
the one from W7. Threading is not properly implemented and, for some weird
reason, all previous versions rely on it.

Then simply execute: ./nloader autochk.exe -p disk.img

NOTE: -p option will force the check even if the disk is not flagged as dirty

You should get something like:

<pre>
The type of the file system is NTFS.
Volume label is sheghey. (guarda che me ne so accorto)

One of your disks needs to be checked for consistency. You
may cancel the disk check, but it is strongly recommended
that you continue.
Windows will now check the disk.

CHKDSK is verifying files (stage 1 of 3)...

File verification completed.

CHKDSK is verifying indexes (stage 2 of 3)...
Deleting index entry System Volume Information in index $I30 of file 5.

Index verification completed.

CHKDSK is verifying security descriptors (stage 3 of 3)...

Security descriptor verification completed.

Windows has checked the file system and found no problems.

   2094079 KB total disk space.
    854716 KB in 4754 files.
      1808 KB in 302 indexes.
         0 KB in bad sectors.
     18379 KB in use by the system.
     12528 KB occupied by the log file.
   1219176 KB available on disk.

      4096 bytes in each allocation unit.
    523519 total allocation units on disk.
    304794 allocation units available on disk.
</pre>


If you want to trace all api calls, you can enable debug logging via the NLOG
environment variable.
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
