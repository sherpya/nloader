#include "ntdll.h"

typedef const PGUID LPCGUID;
typedef FARPROC PETWENABLECALLBACK;
typedef ULONGLONG REGHANDLE, *PREGHANDLE;

typedef enum _EVENT_INFO_CLASS {
    EventProviderBinaryTrackInfo,
    EventProviderSetReserved1,
    EventProviderSetTraits,
    EventProviderUseDescriptorType,
    MaxEventInfo
} EVENT_INFO_CLASS;

// FIXME: this is EtwRegister's signature
NTSTATUS EtwEventRegister(LPCGUID ProviderId, PETWENABLECALLBACK EnableCallback,
    PVOID CallbackContext, PREGHANDLE RegHandle)
{
    return STATUS_SUCCESS;
}

// FIXME: this is EventSetInformation's signature
NTSTATUS EtwEventSetInformation(REGHANDLE RegHandle, EVENT_INFO_CLASS InformationClass,
    LPVOID EventInformation, ULONG InformationLength)
{
    return STATUS_SUCCESS;
}
