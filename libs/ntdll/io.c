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

#include "ntdll.h"

#include <fcntl.h>

/* Defined in sync.c. Used to signal the optional completion Event handle on
 * NtReadFile / NtWriteFile — autochk uses this for "async" I/O even though
 * we always read synchronously here. */
extern NTSTATUS NTAPI NtSetEvent(HANDLE EventHandle, PLONG PreviousState);

/* PIO_APC_ROUTINE is typedef'd as `void *` in winternl.h (opaque). Define the
 * actual signature locally so we can call it through a cast. */
typedef VOID (NTAPI *NLOADER_APC_ROUTINE)(PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG Reserved);

#if defined(__linux)
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif

#ifndef _WIN32
#include <poll.h>
#include <unistd.h>
#endif

#if defined(__FreeBSD__) || defined(__APPLE__)
#include <sys/disk.h>
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#endif

MODULE(io)

#define DEFINE_SCANCODES
#include "scancodes.c"
#undef DEFINE_SCANCODES

#ifndef _WIN32
#define CHECK_FLAG(nt, unix) \
    do { if (DesiredAccess & nt) { flags |= unix; DesiredAccess &= ~nt; } } while (0)

static int ntflags_unix(ACCESS_MASK DesiredAccess)
{
    int flags = 0;

    CHECK_FLAG(FILE_RESERVE_OPFILTER,   0);
    CHECK_FLAG(FILE_ATTRIBUTE_NORMAL,   0);
    CHECK_FLAG(FILE_READ_EA,            0);
    CHECK_FLAG(FILE_WRITE_EA,           0);

    CHECK_FLAG(FILE_READ_ATTRIBUTES,    O_RDONLY);

    CHECK_FLAG(FILE_READ_DATA,          O_RDONLY);

    CHECK_FLAG(FILE_APPEND_DATA,        O_APPEND);
    CHECK_FLAG(0x20000,                 O_CREAT);

    CHECK_FLAG(GENERIC_READ,            O_RDONLY);

    CHECK_FLAG(FILE_WRITE_ATTRIBUTES,   O_RDWR);
    CHECK_FLAG(FILE_WRITE_DATA,         O_RDWR);
    CHECK_FLAG(GENERIC_WRITE,           O_RDWR);

    if (DesiredAccess)
        fprintf(stderr, "Unknown Access in file open 0x%08x\n", DesiredAccess);

    return flags;
}

typedef enum
{
    MTAB_MOUNTED,
    MTAB_NOTMOUNTED,
    MTAB_ERROR
} mtab_mounted;

#if defined(__FreeBSD__) || defined(__APPLE__)
static mtab_mounted is_mounted(const char *device)
{
    struct statfs *fsinfo;
    int c, i;

    if (!(c = getmntinfo(&fsinfo, MNT_NOWAIT)))
        return MTAB_ERROR;

    for (i = 0; i < c; i++)
        if (!strcmp(device, fsinfo[i].f_mntfromname))
            return MTAB_MOUNTED;

    return MTAB_NOTMOUNTED;
}
#else
static mtab_mounted is_mounted(const char *device)
{
    char line[512], *p;
    int lines = 0;
    mtab_mounted result = MTAB_NOTMOUNTED;
    FILE *f_mtab = fopen("/etc/mtab", "r");

    if (!f_mtab)
        return MTAB_ERROR;

    while ((p = fgets(line, sizeof(line), f_mtab)))
    {
        lines++;
        while (*p && (*p != ' ') && (*p != '\t')) p++;
        *p = 0;
        if (!strcmp(device, line))
        {
            result = MTAB_MOUNTED;
            break;
        }
    }

    fclose(f_mtab);

    /* we expect at least one line */
    if (lines)
        return result;

    return MTAB_ERROR;
}
#endif /* __FreeBSD__ || __APPLE__*/


static int unix_open(const char *path, int flags)
{
    if (strcasestr(path, "BOOTEX.LOG"))         /* cruft with log of the scan */
        return open("/dev/null", flags);

    else if (!strncasecmp(path, "Volume{", 7))  /* a device, we need to strip Volume{} */
    {
        size_t len = strlen(path) - 1;

        if (path[len] == '\\')                  /* we don't like Volume{xxxx}\ */
            return -1;

        len -= 6;
        char devpath[len];
        strncpy(devpath, path + 7, len - 1);
        devpath[len - 1] = 0;

        switch (is_mounted(devpath))
        {
            case MTAB_ERROR:
                fprintf(stderr, "unix_open: ERROR: unable to check if mounted\n");
                return -1;
            case MTAB_MOUNTED:
                fprintf(stderr, "unix_open: ERROR: %s is mounted\n", devpath);
                return -1;
            default:
                break;
        }
        return open(devpath, flags);
    }

    /* default or invoked with '*' */
    else if (!strcasecmp(path, "\\??\\Volume{X}\\"))
        return open("disk.img", flags);

    else if (!strcasecmp(path, "disk.img"))     /* our test image */
        return open("disk.img", flags);

    else if (!strncasecmp(path, "\\Device\\KeyboardClass", 21))
    {
        if ((path[22] == 0) && ((path[21] == '0') || (path[21] == '1')))
            return fileno(stdin);
        return -1;
    }

    else if (!strncasecmp(path, "\\Device\\Mailslot", 16))
        return fileno(stdout);

    else if (flags == O_RDONLY)                 /* a generic file only if readonly */
        return open(path, flags);

    return -1;
}
#endif /* _WIN32 */

NTSTATUS NTAPI NtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize,
    ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength)
{
#ifdef _WIN32
    HANDLE hFile;
    LARGE_INTEGER size;
#endif

    DECLAREVARCONV(ObjectAttributesA);
    CHECK_POINTER(FileHandle);
    CHECK_POINTER(IoStatusBlock);

    OA2STR(ObjectAttributes);

    IoStatusBlock->Information = FILE_DOES_NOT_EXIST;

#ifdef REDIR_IO
    {
        IO_STATUS_BLOCK iob;
        FILE_STANDARD_INFORMATION fi;

        NTSTATUS res = ftbl.nt.NtCreateFile(&hFile, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes,
            ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);

        if (res < 0)
        {
            Log("ntdll.NtCreateFile(\"%s\", 0x%08x) = 0x%08x\n", ObjectAttributesA, DesiredAccess, res);
            return res;
        }

        __CreateHandle(*FileHandle, HANDLE_FILE, ObjectAttributesA);
        (*FileHandle)->file.mode = DesiredAccess;
        (*FileHandle)->file.fh = hFile;

        res = ftbl.nt.NtQueryInformationFile(hFile, &iob, &fi, sizeof(fi), FileStandardInformation);

        if (res == STATUS_SUCCESS)
            (*FileHandle)->file.st.st_size = fi.EndOfFile.QuadPart;
        else
        {
            GET_LENGTH_INFORMATION gli;
            if ((res = ftbl.nt.NtDeviceIoControlFile(hFile, NULL, NULL, NULL, IoStatusBlock, IOCTL_DISK_GET_LENGTH_INFO,
                    NULL, 0, &gli, sizeof(GET_LENGTH_INFORMATION))) < 0)
            {
                //fprintf(stderr, "NtOpenFile() - Unable to get size of %s: 0x%08x\n", ObjectAttributesA, res);
                // KeyboardClass etc
                (*FileHandle)->file.st.st_size = 0;
            }
            else
                (*FileHandle)->file.st.st_size = gli.Length.QuadPart;
        }
    }
#endif /* REDIR_IO */

    Log("ntdll.NtCreateFile(\"%s\", 0x%08x)\n", ObjectAttributesA, DesiredAccess);

#ifdef _WIN32
    hFile = CreateFileW(ObjectAttributes->ObjectName->Buffer, (DesiredAccess << 8) & 0xf0000000, ShareAccess, NULL, CreateDisposition, FileAttributes, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        if (err != 3 /* ERROR_PATH_NOT_FOUND */)
            fwprintf(stderr, L"CreateFileW '%s' failed with %d\n", ObjectAttributes->ObjectName->Buffer, GetLastError());
        return (IoStatusBlock->u.Status = STATUS_OBJECT_NAME_NOT_FOUND);
    }

    if (!GetFileSizeEx(hFile, &size))
    {
        fprintf(stderr, "CreateFileW() - Unable to get size of %s: %d\n", ObjectAttributesA, GetLastError());
        return (IoStatusBlock->u.Status = STATUS_OBJECT_NAME_NOT_FOUND);
    }

    __CreateHandle(*FileHandle, HANDLE_FILE, ObjectAttributesA);
    (*FileHandle)->file.fh = hFile;
    (*FileHandle)->file.mode = DesiredAccess;
    (*FileHandle)->file.st.st_size = size.QuadPart;

#else /* _WIN32 */
    int fd = unix_open(ObjectAttributesA, ntflags_unix(DesiredAccess));
    struct stat st;

    if ((fd < 0) || (fstat(fd, &st) < 0))
        return (IoStatusBlock->u.Status = STATUS_OBJECT_NAME_NOT_FOUND);


#if defined(BLKGETSIZE64)
    if (S_ISBLK(st.st_mode) && (ioctl(fd, BLKGETSIZE64, &st.st_size) < 0))
#elif defined(DIOCGMEDIASIZE)
	if (S_ISCHR(st.st_mode) && (ioctl(fd, DIOCGMEDIASIZE, &st.st_size) < 0))
#else
    if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode))
#endif
        st.st_size = 0;

    __CreateHandle(*FileHandle, HANDLE_FILE, ObjectAttributesA);
    (*FileHandle)->file.fh = fd;
    (*FileHandle)->file.mode = DesiredAccess;
    memcpy(&(*FileHandle)->file.st, &st, sizeof(st));

#endif /* _WIN32 */

    IoStatusBlock->Information = FILE_CREATED;
    return (IoStatusBlock->u.Status = STATUS_SUCCESS);
}

NTSTATUS NTAPI NtOpenFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions)
{
    CHECK_POINTER(FileHandle);
    CHECK_POINTER(IoStatusBlock);

#ifdef REDIR_IO
    {
        IO_STATUS_BLOCK iob;
        FILE_STANDARD_INFORMATION fi;
        DECLAREVARCONV(ObjectAttributesA);
        HANDLE hFile;

        NTSTATUS res = ftbl.nt.NtOpenFile(&hFile, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);

        OA2STR(ObjectAttributes);

        if (res < 0)
        {
            Log("ntdll.NtOpenFile(\"%s\", 0x%08x) = 0x%08x\n", ObjectAttributesA, DesiredAccess, res);
            return res;
        }

        __CreateHandle(*FileHandle, HANDLE_FILE, ObjectAttributesA);
        (*FileHandle)->file.mode = DesiredAccess;
        (*FileHandle)->file.fh = hFile;

        res = ftbl.nt.NtQueryInformationFile(hFile, &iob, &fi, sizeof(fi), FileStandardInformation);

        if (res == STATUS_SUCCESS)
            (*FileHandle)->file.st.st_size = fi.EndOfFile.QuadPart;
        else
        {
            GET_LENGTH_INFORMATION gli;
            if ((res = ftbl.nt.NtDeviceIoControlFile(hFile, NULL, NULL, NULL, IoStatusBlock, IOCTL_DISK_GET_LENGTH_INFO,
                NULL, 0, &gli, sizeof(GET_LENGTH_INFORMATION))) < 0)
            {
                fprintf(stderr, "NtOpenFile() - Unable to get size of %s: 0x%08x\n", ObjectAttributesA, res);
                abort();
            }
            (*FileHandle)->file.st.st_size = gli.Length.QuadPart;
        }

        Log("ntdll.NtOpenFile(\"%s\", 0x%08x)\n", ObjectAttributesA, DesiredAccess);
        return res;
    }
#else
    return NtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, NULL, 0, ShareAccess, OPEN_EXISTING, OpenOptions, NULL, 0);
#endif
}

/* Async keyboard plumbing.
 *
 * autochk's AUTOCHECK_MESSAGE::IsKeyPressed opens \Device\KeyboardClass0..63,
 * issues async NtReadFile on each (with an Event handle for completion),
 * then NtWaitForMultipleObjects on the events with a 100ms timeout in a
 * 10s countdown loop. unix_open maps Class0/1 to fileno(stdin).
 *
 * If we read stdin synchronously inside NtReadFile we'd block the whole
 * countdown on a single getchar(), so the request is parked in
 * g_stdin_kbd and STATUS_PENDING is returned. nl_kbd_pump() is called
 * each pass through the wait loops in sync.c — when stdin has a byte it
 * fills the parked buffer, signals the parked Event, and the autochk
 * wait wakes up.
 *
 * Note autochk only reacts to KEY_BREAK (key release), so we always
 * report the byte as a release event.
 */
static struct {
    pthread_mutex_t mu;
    int             active;
    int             unit_id;
    PVOID           buffer;
    PIO_STATUS_BLOCK iosb;
    HANDLE          event;
    int             fd;
} g_stdin_kbd = {
    .mu = PTHREAD_MUTEX_INITIALIZER,
};

#ifndef _WIN32
static int g_stdin_raw_set = 0;
static struct termios g_stdin_saved_tty;

static void restore_stdin_tty(void)
{
    if (g_stdin_raw_set == 1)
        tcsetattr(fileno(stdin), TCSANOW, &g_stdin_saved_tty);
}

static void enable_stdin_raw_once(void)
{
    int fd;
    struct termios raw;

    if (g_stdin_raw_set)
        return;

    fd = fileno(stdin);
    if (!isatty(fd))
    {
        g_stdin_raw_set = 2;        /* nothing to restore — pipe/redirect */
        return;
    }
    if (tcgetattr(fd, &g_stdin_saved_tty) < 0)
    {
        g_stdin_raw_set = 2;
        return;
    }
    raw = g_stdin_saved_tty;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSANOW, &raw);
    g_stdin_raw_set = 1;
    atexit(restore_stdin_tty);
}

static void deliver_kbd_event(int unit_id, unsigned char ch,
                              PVOID buffer, PIO_STATUS_BLOCK iosb, HANDLE event)
{
    KEYBOARD_INPUT_DATA *k = (KEYBOARD_INPUT_DATA *)buffer;

    k->UnitId   = unit_id;
    k->MakeCode = AsciiToScan[ch];
    k->Flags    = KEY_BREAK;
    iosb->Information = sizeof(KEYBOARD_INPUT_DATA);
    iosb->u.Status    = STATUS_SUCCESS;
    if (event)
        NtSetEvent(event, NULL);
}

void nl_kbd_pump(void)
{
    struct pollfd pfd;
    unsigned char ch;
    int           uid, fd;
    PVOID         buf;
    PIO_STATUS_BLOCK iosb;
    HANDLE        event;

    pthread_mutex_lock(&g_stdin_kbd.mu);
    if (!g_stdin_kbd.active)
    {
        pthread_mutex_unlock(&g_stdin_kbd.mu);
        return;
    }
    fd = g_stdin_kbd.fd;
    pthread_mutex_unlock(&g_stdin_kbd.mu);

    pfd.fd = fd;
    pfd.events = POLLIN;
    if (poll(&pfd, 1, 0) <= 0 || !(pfd.revents & POLLIN))
        return;
    if (read(fd, &ch, 1) != 1)
        return;

    pthread_mutex_lock(&g_stdin_kbd.mu);
    if (!g_stdin_kbd.active)
    {
        /* canceled between the poll and now — drop the byte */
        pthread_mutex_unlock(&g_stdin_kbd.mu);
        return;
    }
    uid   = g_stdin_kbd.unit_id;
    buf   = g_stdin_kbd.buffer;
    iosb  = g_stdin_kbd.iosb;
    event = g_stdin_kbd.event;
    g_stdin_kbd.active = 0;
    pthread_mutex_unlock(&g_stdin_kbd.mu);

    deliver_kbd_event(uid, ch, buf, iosb, event);
}
#else
void nl_kbd_pump(void) { }
#endif

NTSTATUS NTAPI NtReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key)
{
    int count;
    CHECK_HANDLE(FileHandle, HANDLE_FILE);
    CHECK_POINTER(IoStatusBlock);

#ifdef REDIR_IO
    {
        NTSTATUS res = ftbl.nt.NtReadFile(FileHandle->file.fh, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
        Log("ntdll.NtReadFile(\"%s\", %p, %d, %lld) = 0x%08x\n", strhandle(FileHandle), Buffer, Length,
            ByteOffset ? ByteOffset->QuadPart : 0, res);
        return res;
    }
#endif

    Log("ntdll.NtReadFile(\"%s\", %p, %d, %lld)\n", strhandle(FileHandle), Buffer, Length,
        ByteOffset ? ByteOffset->QuadPart : 0);

#ifndef _WIN32
    if (FileHandle->file.fh == fileno(stdin))
    {
        char *name;
        int   uid;
        struct pollfd pfd;
        unsigned char ch;

        if (Length != sizeof(KEYBOARD_INPUT_DATA))
            return (IoStatusBlock->u.Status = STATUS_INVALID_PARAMETER);

        enable_stdin_raw_once();

        name = strhandle(FileHandle);
        uid = (strlen(name) >= 24) ? name[23] - '0' : 0;

        /* Fast path: byte already in the tty buffer — complete inline. */
        pfd.fd = FileHandle->file.fh;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN)
            && read(FileHandle->file.fh, &ch, 1) == 1)
        {
            deliver_kbd_event(uid, ch, Buffer, IoStatusBlock, Event);
            return STATUS_SUCCESS;
        }

        /* No data — park the request. The pump in the wait loop will
         * complete it once stdin has a byte. */
        pthread_mutex_lock(&g_stdin_kbd.mu);
        g_stdin_kbd.active  = 1;
        g_stdin_kbd.unit_id = uid;
        g_stdin_kbd.buffer  = Buffer;
        g_stdin_kbd.iosb    = IoStatusBlock;
        g_stdin_kbd.event   = Event;
        g_stdin_kbd.fd      = FileHandle->file.fh;
        pthread_mutex_unlock(&g_stdin_kbd.mu);

        IoStatusBlock->u.Status    = STATUS_PENDING;
        IoStatusBlock->Information = 0;
        return STATUS_PENDING;
    }
#endif

    if (!IsValidHandle(FileHandle->file.fh))
        return (IoStatusBlock->u.Status = STATUS_INVALID_HANDLE);

    if (ByteOffset && (ByteOffset->QuadPart != FILE_USE_FILE_POINTER_POSITION))
    {
        if (FileHandle->file.st.st_size < ByteOffset->QuadPart)
        {
            fprintf(stderr, "ntdll.NtReadFile() - Invalid seek -> %llu - max %llu\n", (unsigned long long)ByteOffset->QuadPart, (unsigned long long)FileHandle->file.st.st_size);
            return (IoStatusBlock->u.Status = STATUS_INVALID_PARAMETER);
        }
#ifdef _WIN32
        SetFilePointerEx(FileHandle->file.fh, *ByteOffset, NULL, 0);
    }

    if (!ReadFile(FileHandle->file.fh, Buffer, Length, &IoStatusBlock->Information, NULL))
        return (IoStatusBlock->u.Status = STATUS_UNSUCCESSFUL);
#else
        lseek(FileHandle->file.fh, ByteOffset->QuadPart, SEEK_SET);
    }

    if ((count = read(FileHandle->file.fh, Buffer, Length)) < 0)
        return (IoStatusBlock->u.Status = STATUS_UNSUCCESSFUL);

    IoStatusBlock->Information = count;
#endif
    IoStatusBlock->u.Status = STATUS_SUCCESS;
    if (Event)
        NtSetEvent(Event, NULL);
    /* No real APC delivery: invoke synchronously since the I/O has already
     * completed. autochk relies on the routine to mark the completion record
     * and wake whatever is polling on it. */
    if (ApcRoutine)
        ((NLOADER_APC_ROUTINE)ApcRoutine)(ApcContext, IoStatusBlock, 0);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtWriteFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock,
    PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key)
{
    int count;
    CHECK_HANDLE(FileHandle, HANDLE_FILE);
    CHECK_POINTER(IoStatusBlock);

#ifdef REDIR_IO
    {
        NTSTATUS res = ftbl.nt.NtWriteFile(FileHandle->file.fh, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
        Log("ntdll.NtWriteFile(\"%s\", %p, %d, %lld) = 0x%08x\n", strhandle(FileHandle), Buffer, Length,
            ByteOffset ? ByteOffset->QuadPart : 0, res);
        return res;
    }
#endif

    Log("ntdll.NtWriteFile(\"%s\", %p, %d, %lld)\n", strhandle(FileHandle), Buffer, Length,
        ByteOffset ? ByteOffset->QuadPart : 0);

    if (ByteOffset && (ByteOffset->QuadPart != FILE_USE_FILE_POINTER_POSITION))
    {
        /* avoid enlarging */
        if (FileHandle->file.st.st_size < ByteOffset->QuadPart)
        {
            fprintf(stderr, "ntdll.NtWriteFile() - Invalid seek -> %llu - max %llu\n", (unsigned long long)ByteOffset->QuadPart, (unsigned long long)FileHandle->file.st.st_size);
            return (IoStatusBlock->u.Status = STATUS_INVALID_PARAMETER);
        }
#ifdef _WIN32
        SetFilePointerEx(FileHandle->file.fh, *ByteOffset, NULL, 0);
    }

    if (!WriteFile(FileHandle->file.fh, Buffer, Length, &IoStatusBlock->Information, NULL))
        return (IoStatusBlock->u.Status = STATUS_UNSUCCESSFUL);
#else
        lseek(FileHandle->file.fh, ByteOffset->QuadPart, SEEK_SET);
    }

    if ((count = write(FileHandle->file.fh, Buffer, Length)) < 0)
        return (IoStatusBlock->u.Status = STATUS_UNSUCCESSFUL);

    IoStatusBlock->Information = count;
#endif
    IoStatusBlock->u.Status = STATUS_SUCCESS;
    if (Event)
        NtSetEvent(Event, NULL);
    if (ApcRoutine)
        ((NLOADER_APC_ROUTINE)ApcRoutine)(ApcContext, IoStatusBlock, 0);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtClose(HANDLE Handle)
{
    CHECK_POINTER(Handle);

    Log("ntdll.NtClose(\"%s\")\n", strhandle(Handle));

    if (Handle->cookie == HANDLE_COOKIE)
    {
        switch (Handle->type)
        {
            case HANDLE_FILE:
            {
#ifdef REDIR_IO
                ftbl.nt.NtClose(Handle->file.fh);
#else
                if (!IsValidHandle(Handle->file.fh))
                    break;
#ifdef _WIN32
                CloseHandle(Handle->file.fh);
#else
                if ((Handle->file.fh != fileno(stdin)) && (Handle->file.fh != fileno(stdout)) && (Handle->file.fh != fileno(stderr)))
                    close(Handle->file.fh);
#endif /* _WIN32 */
#endif /* REDIR_IO */
                break;
            }
            case HANDLE_EV:
            {
                pthread_cond_destroy(&Handle->event.cond);
                pthread_mutex_destroy(&Handle->event.state_mutex);
                break;
            }
            default:
                break;
        }
        RtlFreeHeap(GetProcessHeap(), 0, Handle);
    }
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtDeleteFile(POBJECT_ATTRIBUTES ObjectAttributes)
{
    DECLAREVARCONV(ObjectAttributesA);
    CHECK_POINTER(ObjectAttributes);

    OA2STR(ObjectAttributes);
    Log("ntdll.NtDeleteFile(\"%s\")\n", ObjectAttributesA);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtFlushBuffersFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock)
{
    CHECK_HANDLE(FileHandle, HANDLE_FILE);
    CHECK_POINTER(IoStatusBlock);

    Log("ntdll.NtFlushBuffersFile(\"%s\")\n", strhandle(FileHandle));

    if (!IsValidHandle(FileHandle->file.fh))
        return (IoStatusBlock->u.Status = STATUS_INVALID_HANDLE);

    IoStatusBlock->Information = 0;

    if (FlushFile(FileHandle->file.fh))
        return (IoStatusBlock->u.Status = STATUS_SUCCESS);
    else
        return (IoStatusBlock->u.Status = STATUS_UNSUCCESSFUL);
}

// Windows 8
NTSTATUS NTAPI NtCancelIoFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock)
{
    CHECK_HANDLE(FileHandle, HANDLE_FILE);
    CHECK_POINTER(IoStatusBlock);

    Log("ntdll.NtCancelIoFile(\"%s\")\n", strhandle(FileHandle));

    if (!IsValidHandle(FileHandle->file.fh))
        return (IoStatusBlock->u.Status = STATUS_INVALID_HANDLE);

#ifndef _WIN32
    /* Drop a parked stdin keyboard read so the pump doesn't write into
     * a buffer the caller is about to free. autochk does this in the
     * IsKeyPressed cleanup path. */
    if (FileHandle->file.fh == fileno(stdin))
    {
        HANDLE event = NULL;
        PIO_STATUS_BLOCK parked = NULL;

        pthread_mutex_lock(&g_stdin_kbd.mu);
        if (g_stdin_kbd.active)
        {
            parked = g_stdin_kbd.iosb;
            event  = g_stdin_kbd.event;
            g_stdin_kbd.active = 0;
        }
        pthread_mutex_unlock(&g_stdin_kbd.mu);

        if (parked)
        {
            parked->Information = 0;
            parked->u.Status    = STATUS_CANCELLED;
            if (event)
                NtSetEvent(event, NULL);
        }
    }
#endif

    IoStatusBlock->u.Status = STATUS_CANCELLED;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtOpenSymbolicLinkObject(PHANDLE LinkHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes)
{
    DECLAREVARCONV(ObjectAttributesA);
    CHECK_POINTER(LinkHandle);

    OA2STR(ObjectAttributes);

    if (strncasecmp(ObjectAttributesA, "Volume{", 7))
        return STATUS_OBJECT_NAME_NOT_FOUND;

    __CreateHandle(*LinkHandle, HANDLE_LINK, ObjectAttributesA);

    Log("ntdll.NtOpenSymbolicLinkObject(0x%04x, \"%s\")\n", DesiredAccess, ObjectAttributesA);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtOpenDirectoryObject(PHANDLE DirectoryHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes)
{
    DECLAREVARCONV(ObjectAttributesA);
    CHECK_POINTER(DirectoryHandle);
    CHECK_POINTER(ObjectAttributes);

    OA2STR(ObjectAttributes);

    __CreateHandle(*DirectoryHandle, HANDLE_DIR, ObjectAttributesA);

    Log("ntdll.NtOpenDirectoryObject(\"%s\")\n", ObjectAttributesA);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtQueryDirectoryObject(HANDLE DirectoryHandle, PVOID Buffer, ULONG Length,
    BOOLEAN ReturnSingleEntry, BOOLEAN RestartScan, PULONG Context, PULONG ReturnLength)
{
    static int count = 0;
    OBJECT_DIRECTORY_INFORMATION *odi = Buffer;
    SIZE_T len;
    BYTE *p;

    CHECK_HANDLE(DirectoryHandle, HANDLE_DIR);
    CHECK_POINTER(Buffer);

    if (!ReturnSingleEntry)
        return STATUS_NOT_IMPLEMENTED;

    if (count)
    {
        count = 0;
        return STATUS_NO_MORE_ENTRIES;
    }

    p = ((BYTE *) Buffer + sizeof(OBJECT_DIRECTORY_INFORMATION));

    memset(p, 0, sizeof(OBJECT_DIRECTORY_INFORMATION));
    p += sizeof(OBJECT_DIRECTORY_INFORMATION);

    memcpy(p, dirinfo_name, sizeof(dirinfo_name));
    RtlInitUnicodeString(&odi->Name, (LPCWSTR) p);
    p += sizeof(dirinfo_name);

    memcpy(p, dirinfo_type, sizeof(dirinfo_type));
    RtlInitUnicodeString(&odi->TypeName, (LPCWSTR) p);

    len = sizeof(OBJECT_DIRECTORY_INFORMATION) * 2;
    len += sizeof(dirinfo_name) + sizeof(dirinfo_type);

    count++;

    // FIXME: this check should be done before
    if (ReturnLength)
        *ReturnLength = len;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtQuerySymbolicLinkObject(HANDLE LinkHandle, PUNICODE_STRING LinkTarget, PULONG ReturnedLength)
{
    CHECK_HANDLE(LinkHandle, HANDLE_LINK);
    CHECK_POINTER(LinkTarget);

    RtlInitUnicodeString(LinkTarget, devicelink);

    if (ReturnedLength)
        *ReturnedLength = LinkTarget->Length;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtQueryDirectoryFile(HANDLE FileHandle, HANDLE Event,
    PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation,
    ULONG Length, FILE_INFORMATION_CLASS FileInformationClass,
    BOOLEAN ReturnSingleEntry, PUNICODE_STRING FileName, BOOLEAN RestartScan)
{
    CHECK_HANDLE(FileHandle, HANDLE_FILE);
    CHECK_POINTER(IoStatusBlock);

    Log("ntdll.NtQueryDirectoryFile(\"%s\")\n", strhandle(FileHandle));

    if (!IsValidHandle(FileHandle->file.fh))
        return (IoStatusBlock->u.Status = STATUS_INVALID_HANDLE);

    return (IoStatusBlock->u.Status = STATUS_UNSUCCESSFUL);
}
