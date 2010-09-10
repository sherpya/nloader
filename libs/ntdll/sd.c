/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Security descriptor functions
 * FILE:              lib/rtl sd/acl/sid etc
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

#include "ntdll.h"
MODULE(sd)

#define SECURITY_DESCRIPTOR_REVISION1   1

#define SID_REVISION                    1
#define SID_MAX_SUB_AUTHORITIES         15
#define SID_RECOMMENDED_SUB_AUTHORITIES 1

#define SE_DACL_PRESENT                 0x0004
#define SE_SACL_PRESENT                 0x0010
#define SE_SELF_RELATIVE                0x8000

#define SE_GROUP_DEFAULTED 2
#define SE_DACL_DEFAULTED  8

#define OWNER_SECURITY_INFORMATION      0x01
#define GROUP_SECURITY_INFORMATION      0x02
#define DACL_SECURITY_INFORMATION       0x04
#define SACL_SECURITY_INFORMATION       0x08

#define MIN_ACL_REVISION                2
#define MAX_ACL_REVISION                4

/* capitan ovvio */
#define ACL_REVISION1   1
#define ACL_REVISION2   2
#define ACL_REVISION3   3
#define ACL_REVISION4   4

#define ACE_OBJECT_TYPE_PRESENT           0x00000001
#define ACE_INHERITED_OBJECT_TYPE_PRESENT 0x00000002

#define ACCESS_ALLOWED_ACE_TYPE                 0x00
#define ACCESS_ALLOWED_COMPOUND_ACE_TYPE        0x04

#define SYSTEM_AUDIT_ACE_TYPE                   0x02
#define SYSTEM_AUDIT_OBJECT_ACE_TYPE            0x07
#define SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE   0x0f
#define SYSTEM_MANDATORY_LABEL_ACE_TYPE         0x11

#define VALID_INHERIT_FLAGS         0x1f
#define SUCCESSFUL_ACCESS_ACE_FLAG  0x40
#define FAILED_ACCESS_ACE_FLAG      0x80

#define SECURITY_MANDATORY_LABEL_AUTHORITY      { 0, 0, 0, 0, 0, 16 }

typedef DWORD SECURITY_INFORMATION,*PSECURITY_INFORMATION;
typedef PVOID PSID;

typedef struct _ACL {
    BYTE AclRevision;
    BYTE Sbz1;
    WORD AclSize;
    WORD AceCount;
    WORD Sbz2;
} ACL, *PACL;

typedef enum _ACL_INFORMATION_CLASS {
    AclRevisionInformation = 1,
    AclSizeInformation
} ACL_INFORMATION_CLASS;

typedef struct _ACL_REVISION_INFORMATION {
    DWORD AclRevision;
} ACL_REVISION_INFORMATION, *PACL_REVISION_INFORMATION;

typedef struct _ACL_SIZE_INFORMATION {
    DWORD AceCount;
    DWORD AclBytesInUse;
    DWORD AclBytesFree;
} ACL_SIZE_INFORMATION, *PACL_SIZE_INFORMATION;

typedef struct _SECURITY_DESCRIPTOR_RELATIVE
{
    UCHAR Revision;
    UCHAR Sbz1;
    USHORT Control;
    ULONG Owner;
    ULONG Group;
    ULONG Sacl;
    ULONG Dacl;
} SECURITY_DESCRIPTOR_RELATIVE, *PISECURITY_DESCRIPTOR_RELATIVE;

typedef struct _SECURITY_DESCRIPTOR {
    UCHAR Revision;
    UCHAR Sbz1;
    USHORT Control;
    PSID Owner;
    PSID Group;
    PACL Sacl;
    PACL Dacl;
} SECURITY_DESCRIPTOR, *PISECURITY_DESCRIPTOR;
typedef PVOID PSECURITY_DESCRIPTOR;

typedef struct _SID_IDENTIFIER_AUTHORITY {
    UCHAR Value[6];
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY, *LPSID_IDENTIFIER_AUTHORITY;

typedef struct _SID {
    UCHAR Revision;
    UCHAR SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    ULONG SubAuthority[1];
} SID, *PISID;


typedef struct _ACE_HEADER {
    UCHAR AceType;
    UCHAR AceFlags;
    USHORT AceSize;
} ACE_HEADER, *PACE_HEADER;

typedef struct _ACE
{
    ACE_HEADER Header;
    ACCESS_MASK AccessMask;
} ACE, *PACE;


/* functions */

BOOLEAN NTAPI RtlValidSid(PSID Sid_)
{
    PISID Sid = Sid_;

    if ((Sid->Revision != SID_REVISION) || (Sid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES))
        return FALSE;
    return TRUE;
}

ULONG NTAPI RtlLengthSid(PSID Sid_)
{
    PISID Sid =  Sid_;
    return (ULONG) FIELD_OFFSET(SID, SubAuthority[Sid->SubAuthorityCount]);
}

ULONG NTAPI RtlLengthRequiredSid(ULONG SubAuthorityCount)
{
    return (ULONG) FIELD_OFFSET(SID, SubAuthority[SubAuthorityCount]);
}

NTSTATUS NTAPI RtlInitializeSid(PSID Sid_, PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    UCHAR SubAuthorityCount)
{
    PISID Sid = Sid_;

    Sid->Revision = SID_REVISION;
    Sid->SubAuthorityCount = SubAuthorityCount;

    memcpy(&Sid->IdentifierAuthority, IdentifierAuthority, sizeof(SID_IDENTIFIER_AUTHORITY));

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlCopySid(ULONG BufferLength, PSID Dest, PSID Src)
{

    if (BufferLength < RtlLengthSid(Src))
        return STATUS_UNSUCCESSFUL;

    memmove(Dest, Src, RtlLengthSid(Src));

    return STATUS_SUCCESS;
}

PULONG NTAPI RtlSubAuthoritySid(PSID Sid_, ULONG SubAuthority)
{
    PISID Sid = Sid_;
    return (PULONG) &Sid->SubAuthority[SubAuthority];
}

BOOLEAN NTAPI RtlFirstFreeAce(PACL Acl, PACE* Ace)
{
    PACE Current;
    ULONG_PTR AclEnd;
    ULONG i;

    Current = (PACE)(Acl + 1);
    *Ace = NULL;

    if (Acl->AceCount == 0)
    {
        *Ace = Current;
        return TRUE;
    }

    i = 0;
    AclEnd = (ULONG_PTR)Acl + Acl->AclSize;

    do
    {
        if ((ULONG_PTR)Current >= AclEnd)
            return FALSE;

      if (Current->Header.AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE && Acl->AclRevision < ACL_REVISION3)
            return FALSE;

      Current = (PACE)((ULONG_PTR)Current + Current->Header.AceSize);
    }
    while (++i < Acl->AceCount);

    if ((ULONG_PTR)Current < AclEnd)
      *Ace = Current;

    return TRUE;
}

static VOID RtlpAddData(PVOID AceList, ULONG AceListLength, PVOID Ace, ULONG Offset)
{
    if (Offset > 0)
        memcpy((PVOID)((ULONG_PTR)Ace + AceListLength), Ace, Offset);

    if (AceListLength != 0)
        memcpy(Ace, AceList, AceListLength);
}

static NTSTATUS RtlpAddKnownAce(PACL Acl, ULONG Revision, ULONG Flags, ACCESS_MASK AccessMask,
    GUID *ObjectTypeGuid, GUID *InheritedObjectTypeGuid, PSID Sid, ULONG Type)
{
    PACE Ace;
    PSID SidStart;
    ULONG AceSize, InvalidFlags;
    ULONG AceObjectFlags = 0;

    if (!RtlValidSid(Sid))
        return STATUS_INVALID_SID;

    if (Type == SYSTEM_MANDATORY_LABEL_ACE_TYPE)
    {
        static const SID_IDENTIFIER_AUTHORITY MandatoryLabelAuthority = { SECURITY_MANDATORY_LABEL_AUTHORITY };

       /* The SID's identifier authority must be SECURITY_MANDATORY_LABEL_AUTHORITY! */
        if (memcmp(&((PISID)Sid)->IdentifierAuthority, &MandatoryLabelAuthority,
            sizeof(MandatoryLabelAuthority)) != sizeof(MandatoryLabelAuthority))
            return STATUS_INVALID_PARAMETER;
    }

    if (Acl->AclRevision > MAX_ACL_REVISION || Revision > MAX_ACL_REVISION)
        return STATUS_UNKNOWN_REVISION;

    if (Revision < Acl->AclRevision)
        Revision = Acl->AclRevision;

   /* Validate the flags */
    if (Type == SYSTEM_AUDIT_ACE_TYPE ||
        Type == SYSTEM_AUDIT_OBJECT_ACE_TYPE ||
        Type == SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE)
        InvalidFlags = Flags & ~(VALID_INHERIT_FLAGS | SUCCESSFUL_ACCESS_ACE_FLAG | FAILED_ACCESS_ACE_FLAG);
    else
         InvalidFlags = Flags & ~VALID_INHERIT_FLAGS;

    if (InvalidFlags)
        return STATUS_INVALID_PARAMETER;

    if (!RtlFirstFreeAce(Acl, &Ace))
        return STATUS_INVALID_ACL;

    if (!Ace)
        return STATUS_ALLOTTED_SPACE_EXCEEDED;

    /* Calculate the size of the ACE */
    AceSize = RtlLengthSid(Sid) + sizeof(ACE);

    if (ObjectTypeGuid)
    {
        AceObjectFlags |= ACE_OBJECT_TYPE_PRESENT;
        AceSize += sizeof(GUID);
    }

    if (InheritedObjectTypeGuid)
    {
        AceObjectFlags |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
        AceSize += sizeof(GUID);
    }

    if (AceObjectFlags)
    {
        /* Don't forget the ACE object flags (corresponds to the Flags field in the *_OBJECT_ACE structures) */
        AceSize += sizeof(ULONG);
    }

    if ((ULONG_PTR)Ace + AceSize > (ULONG_PTR)Acl + Acl->AclSize)
        return STATUS_ALLOTTED_SPACE_EXCEEDED ;

    /* initialize the header and common fields */
    Ace->Header.AceFlags = Flags;
    Ace->Header.AceType = Type;
    Ace->Header.AceSize = (WORD)AceSize;
    Ace->AccessMask = AccessMask;

    if (AceObjectFlags)
    {
        /* Write the ACE flags to the ACE (corresponds to the Flags field in the *_OBJECT_ACE structures) */
        *(PULONG)(Ace + 1) = AceObjectFlags;
        SidStart = (PSID)((ULONG_PTR)(Ace + 1) + sizeof(ULONG));
    }
    else
        SidStart = (PSID)(Ace + 1);

    /* copy the GUIDs */
    if (ObjectTypeGuid)
    {
        memcpy(SidStart, ObjectTypeGuid, sizeof(GUID));
        SidStart = (PSID)((ULONG_PTR)SidStart + sizeof(GUID));
    }

    if (InheritedObjectTypeGuid)
    {
        memcpy(SidStart, InheritedObjectTypeGuid, sizeof(GUID));
        SidStart = (PSID)((ULONG_PTR)SidStart + sizeof(GUID));
    }

    /* copy the SID */
    RtlCopySid(RtlLengthSid(Sid), SidStart, Sid);
    Acl->AceCount++;
    Acl->AclRevision = Revision;

    return STATUS_SUCCESS;
}


NTSTATUS NTAPI RtlAddAccessAllowedAce(PACL Acl, ULONG Revision,
    ACCESS_MASK AccessMask, PSID Sid)
{
    return RtlpAddKnownAce(Acl, Revision, 0, AccessMask, NULL, NULL, Sid, ACCESS_ALLOWED_ACE_TYPE);
}

NTSTATUS NTAPI RtlAddAce(PACL Acl, ULONG AclRevision, ULONG StartingIndex,
    PVOID AceList, ULONG AceListLength)
{
    PACE Ace;
    PACE Current;
    ULONG NewAceCount;
    ULONG Index;

    if (Acl->AclRevision < MIN_ACL_REVISION || Acl->AclRevision > MAX_ACL_REVISION || !RtlFirstFreeAce(Acl, &Ace))
        return STATUS_INVALID_PARAMETER;

    if (Acl->AclRevision <= AclRevision)
        AclRevision = Acl->AclRevision;

    if (((ULONG_PTR)AceList + AceListLength) <= (ULONG_PTR)AceList)
        return STATUS_INVALID_PARAMETER;

    for (Current = AceList, NewAceCount = 0; (ULONG_PTR)Current < ((ULONG_PTR)AceList + AceListLength);
            Current = (PACE)((ULONG_PTR)Current + Current->Header.AceSize), ++NewAceCount)
    {
        if (((PACE)AceList)->Header.AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE && AclRevision < ACL_REVISION3)
            return STATUS_INVALID_PARAMETER;
    }


    if (!Ace || ((ULONG_PTR)Ace + AceListLength) > ((ULONG_PTR)Acl + Acl->AclSize))
        return STATUS_BUFFER_TOO_SMALL;

    Current = (PACE)(Acl + 1);

    for (Index = 0; Index < StartingIndex && Index < Acl->AceCount; Index++)
        Current = (PACE)((ULONG_PTR)Current + Current->Header.AceSize);

    RtlpAddData(AceList, AceListLength, Current, (ULONG)((ULONG_PTR)Ace - (ULONG_PTR)Current));
    Acl->AceCount = Acl->AceCount + NewAceCount;
    Acl->AclRevision = AclRevision;

    return STATUS_SUCCESS;
}


BOOLEAN NTAPI RtlValidAcl(PACL Acl)
{
    PACE Ace;
    USHORT Size;

    Size = ROUND_UP(Acl->AclSize, 4);

    if (Acl->AclRevision < MIN_ACL_REVISION || Acl->AclRevision > MAX_ACL_REVISION)
      return(FALSE);

    if (Size != Acl->AclSize)
      return(FALSE);

    return RtlFirstFreeAce(Acl, &Ace);
}

NTSTATUS NTAPI RtlCreateAcl(PACL Acl, ULONG AclSize, ULONG AclRevision)
{

    if (AclSize < sizeof(ACL))
        return STATUS_BUFFER_TOO_SMALL;

    if (AclRevision < MIN_ACL_REVISION || AclRevision > MAX_ACL_REVISION || AclSize > 0xffff)
        return STATUS_INVALID_PARAMETER;

    AclSize = ROUND_UP(AclSize, 4);
    Acl->AclSize = AclSize;
    Acl->AclRevision = AclRevision;
    Acl->AceCount = 0;
    Acl->Sbz1 = 0;
    Acl->Sbz2 = 0;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlQueryInformationAcl(PACL Acl, PVOID Information,
    ULONG InformationLength, ACL_INFORMATION_CLASS InformationClass)
{
    PACE Ace;


    if (Acl->AclRevision < MIN_ACL_REVISION || Acl->AclRevision > MAX_ACL_REVISION)
        return STATUS_INVALID_PARAMETER;

    switch (InformationClass)
    {
        case AclRevisionInformation:
        {
            PACL_REVISION_INFORMATION Info = Information;

            if (InformationLength < sizeof(ACL_REVISION_INFORMATION))
                return STATUS_BUFFER_TOO_SMALL;

            Info->AclRevision = Acl->AclRevision;
            break;
        }

        case AclSizeInformation:
        {
            PACL_SIZE_INFORMATION Info = Information;

            if (InformationLength < sizeof(ACL_SIZE_INFORMATION))
                return STATUS_BUFFER_TOO_SMALL;

            if (!RtlFirstFreeAce(Acl, &Ace))
                return STATUS_INVALID_PARAMETER;

            Info->AceCount = Acl->AceCount;

            if (Ace)
            {
                Info->AclBytesInUse = (DWORD)((ULONG_PTR)Ace - (ULONG_PTR)Acl);
                Info->AclBytesFree  = Acl->AclSize - Info->AclBytesInUse;
            }
            else
            {
                Info->AclBytesInUse = Acl->AclSize;
                Info->AclBytesFree  = 0;
            }
            break;
         }
        default:
            return STATUS_INVALID_INFO_CLASS;
    }

    return STATUS_SUCCESS;
}

BOOLEAN NTAPI RtlValidRelativeSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    ULONG SecurityDescriptorLength, SECURITY_INFORMATION RequiredInformation)
{
    PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR) SecurityDescriptorInput;

    if (SecurityDescriptorLength < sizeof(SECURITY_DESCRIPTOR_RELATIVE) ||
            pSD->Revision != SECURITY_DESCRIPTOR_REVISION1 || !(pSD->Control & SE_SELF_RELATIVE))
        return FALSE;

    if (pSD->Owner != 0)
    {
        PSID Owner = (PSID)((ULONG_PTR)pSD->Owner + (ULONG_PTR)pSD);
        if (!RtlValidSid(Owner))
            return FALSE;
    }
    else if (RequiredInformation & OWNER_SECURITY_INFORMATION)
        return FALSE;

    if (pSD->Group != 0)
    {
        PSID Group = (PSID)((ULONG_PTR)pSD->Group + (ULONG_PTR)pSD);
            if (!RtlValidSid(Group))
         return FALSE;
    }
    else if (RequiredInformation & GROUP_SECURITY_INFORMATION)
        return FALSE;

    if (pSD->Control & SE_DACL_PRESENT)
    {
        if (pSD->Dacl != 0 && !RtlValidAcl((PACL)((ULONG_PTR)pSD->Dacl + (ULONG_PTR)pSD)))
            return FALSE;
    }
    else if (RequiredInformation & DACL_SECURITY_INFORMATION)
      return FALSE;

    if (pSD->Control & SE_SACL_PRESENT)
    {
        if (pSD->Sacl != 0 && !RtlValidAcl((PACL)((ULONG_PTR)pSD->Sacl + (ULONG_PTR)pSD)))
         return FALSE;
    }
    else if (RequiredInformation & SACL_SECURITY_INFORMATION)
      return FALSE;

   return TRUE;
}

static VOID RtlpQuerySecurityDescriptorPointers(PISECURITY_DESCRIPTOR SecurityDescriptor,
    PSID *Owner, PSID *Group, PACL *Sacl, PACL *Dacl)
{
    if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
    {
        PISECURITY_DESCRIPTOR_RELATIVE RelSD = (PISECURITY_DESCRIPTOR_RELATIVE) SecurityDescriptor;

        if (Owner)
            *Owner = ((RelSD->Owner != 0) ? (PSID)((ULONG_PTR)RelSD + RelSD->Owner) : NULL);

        if (Group)
            *Group = ((RelSD->Group != 0) ? (PSID)((ULONG_PTR)RelSD + RelSD->Group) : NULL);

        if (Sacl)
            *Sacl = (((RelSD->Control & SE_SACL_PRESENT) && (RelSD->Sacl != 0)) ?  (PSID)((ULONG_PTR)RelSD + RelSD->Sacl) : NULL);

        if (Dacl)
            *Dacl = (((RelSD->Control & SE_DACL_PRESENT) && (RelSD->Dacl != 0)) ?  (PSID)((ULONG_PTR)RelSD + RelSD->Dacl) : NULL);
    }
    else
    {
        if (Owner)
            *Owner = SecurityDescriptor->Owner;

        if (Group)
            *Group = SecurityDescriptor->Group;

        if (Sacl)
            *Sacl = ((SecurityDescriptor->Control & SE_SACL_PRESENT) ? SecurityDescriptor->Sacl : NULL);

        if (Dacl)
            *Dacl = ((SecurityDescriptor->Control & SE_DACL_PRESENT) ? SecurityDescriptor->Dacl : NULL);
    }
}

ULONG NTAPI RtlLengthSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    PSID Owner, Group;
    PACL Sacl, Dacl;
    ULONG Length = sizeof(SECURITY_DESCRIPTOR);

    RtlpQuerySecurityDescriptorPointers((PISECURITY_DESCRIPTOR) SecurityDescriptor, &Owner, &Group, &Sacl, &Dacl);

    if (Owner)
        Length += ROUND_UP(RtlLengthSid(Owner), 4);

    if (Group)
        Length += ROUND_UP(RtlLengthSid(Group), 4);

    if (Dacl)
        Length += ROUND_UP(Dacl->AclSize, 4);

    if (Sacl)
        Length += ROUND_UP(Sacl->AclSize, 4);

    return Length;
}

BOOLEAN NTAPI RtlValidSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR) SecurityDescriptor;
    PSID Owner, Group;
    PACL Sacl, Dacl;


    if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
        return FALSE;

    RtlpQuerySecurityDescriptorPointers(pSD, &Owner, &Group, &Sacl, &Dacl);

    if ((Owner && !RtlValidSid(Owner)) ||
        (Group && !RtlValidSid(Group)) ||
        (Sacl && !RtlValidAcl(Sacl))   ||
        (Dacl && !RtlValidAcl(Dacl)))
        return FALSE;

   return TRUE;
}

NTSTATUS NTAPI RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG Revision)
{
    PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR) SecurityDescriptor;

    if (Revision != SECURITY_DESCRIPTOR_REVISION1)
        return STATUS_UNKNOWN_REVISION;

    pSD->Revision = Revision;
    pSD->Sbz1 = 0;
    pSD->Control = 0;
    pSD->Owner = NULL;
    pSD->Group = NULL;
    pSD->Sacl = NULL;
    pSD->Dacl = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlSetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
    PSID Group, BOOLEAN GroupDefaulted)
{
    PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR) SecurityDescriptor;

    if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
        return STATUS_UNKNOWN_REVISION;

    if (pSD->Control & SE_SELF_RELATIVE)
        return STATUS_BAD_DESCRIPTOR_FORMAT;

    pSD->Group = Group;
    pSD->Control &= ~(SE_GROUP_DEFAULTED);

    if (GroupDefaulted)
        pSD->Control |= SE_GROUP_DEFAULTED;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
    BOOLEAN DaclPresent, PACL Dacl, BOOLEAN DaclDefaulted)
{
    PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;


    if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
        return STATUS_UNKNOWN_REVISION;

    if (pSD->Control & SE_SELF_RELATIVE)
        return STATUS_BAD_DESCRIPTOR_FORMAT;

    if (!DaclPresent)
    {
        pSD->Control = pSD->Control & ~(SE_DACL_PRESENT);
        return STATUS_SUCCESS;
    }

    pSD->Dacl = Dacl;
    pSD->Control |= SE_DACL_PRESENT;
    pSD->Control &= ~(SE_DACL_DEFAULTED);

    if (DaclDefaulted)
        pSD->Control |= SE_DACL_DEFAULTED;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlNewSecurityObject(PSECURITY_DESCRIPTOR ParentDescriptor,
    PSECURITY_DESCRIPTOR CreatorDescriptor, PSECURITY_DESCRIPTOR *NewDescriptor,
    BOOLEAN IsDirectoryObject, HANDLE Token, PVOID /*PGENERIC_MAPPING*/ GenericMapping)
{
    Log("RtlNewSecurityObject() !!UNIMPLEMENTED!!\n");
    return STATUS_NOT_IMPLEMENTED;
}
