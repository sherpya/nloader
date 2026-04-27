#include <nt_structs.h>

typedef struct _RTL_SRWLOCK {
  PVOID Ptr;
} RTL_SRWLOCK, *PRTL_SRWLOCK;

VOID NTAPI RtlInitializeSRWLock(OUT PRTL_SRWLOCK SRWLock)
{
    SRWLock->Ptr = NULL;
}

VOID NTAPI RtlAcquireSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock)
{
}

VOID NTAPI RtlReleaseSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock)
{
}
