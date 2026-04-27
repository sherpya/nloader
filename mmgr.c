#define _UNICODE
#define UNICODE
#include <windows.h>
#include <ddk/mountmgr.h>
#include <stdio.h>

int
main ()
{
  wchar_t chDrive = L'N', szUniqueId[128];
  wchar_t szDeviceName[7] = L"\\\\.\\";
  HANDLE hDevice, hMountMgr;
  BYTE byBuffer[1024];
  PMOUNTDEV_NAME pMountDevName;
  DWORD cbBytesReturned, dwInBuffer, dwOutBuffer, ccb;
  PMOUNTMGR_MOUNT_POINT pMountPoint;
  BOOL bSuccess;
  PBYTE pbyInBuffer, pbyOutBuffer;
  LPTSTR pszLogicalDrives, pszDriveRoot;

  // MOUNTMGR_DOS_DEVICE_NAME is defined as L"\\\\.\\MountPointManager"
  hMountMgr = CreateFile (MOUNTMGR_DOS_DEVICE_NAME,
			  0, FILE_SHARE_READ | FILE_SHARE_WRITE,
			  NULL, OPEN_EXISTING, 0, NULL);
  if (hMountMgr == INVALID_HANDLE_VALUE)
    return 1;

  cbBytesReturned = GetLogicalDriveStrings (0, NULL);
  pszLogicalDrives = (LPTSTR) LocalAlloc (LMEM_ZEROINIT,
					  cbBytesReturned * sizeof (wchar_t));
  cbBytesReturned = GetLogicalDriveStrings (cbBytesReturned,
					    pszLogicalDrives);
  for (pszDriveRoot = pszLogicalDrives; *pszDriveRoot != L'\0';
       pszDriveRoot += lstrlen (pszDriveRoot) + 1)
    {

      szDeviceName[4] = pszDriveRoot[0];
      szDeviceName[5] = L':';
      szDeviceName[6] = L'\0';
      //lstrcpy (&szDeviceName[4], L"\\??\\USBSTOR\\DISK&VEN_SANDISK&PROD_CRUZER&REV_8.01\\1740030578903736&0");

      hDevice = CreateFile (szDeviceName, 0,
			    FILE_SHARE_READ | FILE_SHARE_WRITE,
			    NULL, OPEN_EXISTING, 0, NULL);
      if (hDevice == INVALID_HANDLE_VALUE)
	return 1;

      bSuccess = DeviceIoControl (hDevice,
				  IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
				  NULL, 0,
				  (LPVOID) byBuffer, sizeof (byBuffer),
				  &cbBytesReturned, (LPOVERLAPPED) NULL);
      pMountDevName = (PMOUNTDEV_NAME) byBuffer;
      wprintf (L"\n%.*ls\n", pMountDevName->NameLength / sizeof (wchar_t),
	       pMountDevName->Name);
      bSuccess = CloseHandle (hDevice);

      dwInBuffer = pMountDevName->NameLength + sizeof (MOUNTMGR_MOUNT_POINT);
      pbyInBuffer = (PBYTE) LocalAlloc (LMEM_ZEROINIT, dwInBuffer);
      pMountPoint = (PMOUNTMGR_MOUNT_POINT) pbyInBuffer;
      pMountPoint->DeviceNameLength = pMountDevName->NameLength;
      pMountPoint->DeviceNameOffset = sizeof (MOUNTMGR_MOUNT_POINT);
      CopyMemory (pbyInBuffer + sizeof (MOUNTMGR_MOUNT_POINT),
		  pMountDevName->Name, pMountDevName->NameLength);

      dwOutBuffer = 1024 + sizeof (MOUNTMGR_MOUNT_POINTS);
      pbyOutBuffer = (PBYTE) LocalAlloc (LMEM_ZEROINIT, dwOutBuffer);
      bSuccess = DeviceIoControl (hMountMgr,
				  IOCTL_MOUNTMGR_QUERY_POINTS,
				  pbyInBuffer, dwInBuffer,
				  (LPVOID) pbyOutBuffer, dwOutBuffer,
				  &cbBytesReturned, (LPOVERLAPPED) NULL);
      if (bSuccess)
	{
	  ULONG i;
	  PMOUNTMGR_MOUNT_POINTS pMountPoints =
	    (PMOUNTMGR_MOUNT_POINTS) pbyOutBuffer;
	  for (i = 0; i < pMountPoints->NumberOfMountPoints; i++)
	    {
	      wprintf (L"#%i:\n", i);
	      wprintf (L"    Device=%.*ls\n",
		       pMountPoints->MountPoints[i].DeviceNameLength /
		       sizeof (wchar_t),
		       pbyOutBuffer +
		       pMountPoints->MountPoints[i].DeviceNameOffset);
	      wprintf (L"    SymbolicLink=%.*ls\n",
		       pMountPoints->MountPoints[i].SymbolicLinkNameLength /
		       sizeof (wchar_t),
		       pbyOutBuffer +
		       pMountPoints->MountPoints[i].SymbolicLinkNameOffset);
	      ccb = sizeof (szUniqueId) / sizeof (szUniqueId[0]);

	      if (CryptBinaryToString
		  (pbyOutBuffer + pMountPoints->MountPoints[i].UniqueIdOffset,
		   pMountPoints->MountPoints[i].UniqueIdLength,
		   CRYPT_STRING_BASE64, szUniqueId, &ccb))
		wprintf (L"    UniqueId=%s\n", szUniqueId);
	      else
		wprintf (L"    UniqueId=%.*ls\n",
			 pMountPoints->MountPoints[i].UniqueIdLength /
			 sizeof (wchar_t),
			 pbyOutBuffer +
			 pMountPoints->MountPoints[i].UniqueIdOffset);
	    }
	}
      pbyInBuffer = (PBYTE) LocalFree (pbyInBuffer);
      pbyOutBuffer = (PBYTE) LocalFree (pbyOutBuffer);
    }
  pszLogicalDrives = (LPTSTR) LocalFree (pszLogicalDrives);
  bSuccess = CloseHandle (hMountMgr);

  return 0;
}
