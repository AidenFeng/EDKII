/*++

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FvbInfo.c

Abstract:

  Defines data structure that is the volume header found.These data is intent
  to decouple FVB driver with FV header.

--*/
#include "PiDxe.h"
#include <Guid/EventGroup.h>
#include <Protocol/FvbExtension.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Guid/AlternateFvBlock.h>
#include <Protocol/DevicePath.h>

#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>

#include <Guid/FirmwareFileSystem2.h>
#include <Guid/SystemNvDataGuid.h>

typedef struct {
  UINT64                      FvLength;
  EFI_FIRMWARE_VOLUME_HEADER  FvbInfo;
  //
  // EFI_FV_BLOCK_MAP_ENTRY    ExtraBlockMap[n];//n=0
  //
  EFI_FV_BLOCK_MAP_ENTRY      End[1];
} EFI_FVB_MEDIA_INFO;

EFI_FVB_MEDIA_INFO  mPlatformFvbMediaInfo[] = {
  //
  // Recovery BOIS FVB
  //
  {
    FixedPcdGet32 (PcdUnixFlashFvRecoverySize),
    {
      {
        0,
      },  // ZeroVector[16]
      EFI_FIRMWARE_FILE_SYSTEM2_GUID,
      FixedPcdGet32 (PcdUnixFlashFvRecoverySize),
      EFI_FVH_SIGNATURE,
      EFI_FVB2_READ_ENABLED_CAP |
        EFI_FVB2_READ_STATUS |
        EFI_FVB2_WRITE_ENABLED_CAP |
        EFI_FVB2_WRITE_STATUS |
        EFI_FVB2_ERASE_POLARITY,
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,  // CheckSum
      0,  // ExtHeaderOffset
      {
        0,
      },  // Reserved[1]
      1,  // Revision
      {
        {
          FixedPcdGet32 (PcdUnixFlashFvRecoverySize)/FixedPcdGet32 (PcdUnixFirmwareBlockSize),
          FixedPcdGet32 (PcdUnixFirmwareBlockSize),
        }
      }
    },
    {
      {
        0,
        0
      }
    }
  },
  //
  // Systen NvStorage FVB
  //
  {
    FixedPcdGet32 (PcdFlashNvStorageVariableSize) + \
    FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) + \
    FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize) + \
    FixedPcdGet32 (PcdUnixFlashNvStorageEventLogSize),
    {
      {
        0,
      },  // ZeroVector[16]
      EFI_SYSTEM_NV_DATA_FV_GUID,
      FixedPcdGet32 (PcdFlashNvStorageVariableSize) + \
      FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) + \
      FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize) + \
      FixedPcdGet32 (PcdUnixFlashNvStorageEventLogSize),
      EFI_FVH_SIGNATURE,
      EFI_FVB2_READ_ENABLED_CAP |
        EFI_FVB2_READ_STATUS |
        EFI_FVB2_WRITE_ENABLED_CAP |
        EFI_FVB2_WRITE_STATUS |
        EFI_FVB2_ERASE_POLARITY,
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,  // CheckSum
      0,  // ExtHeaderOffset
      {
        0,
      },  // Reserved[1]
      1,  // Revision
      {
        {
          (FixedPcdGet32 (PcdFlashNvStorageVariableSize) + \
          FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) + \
          FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize) + \
          FixedPcdGet32 (PcdUnixFlashNvStorageEventLogSize)) / FixedPcdGet32 (PcdUnixFirmwareBlockSize),
          FixedPcdGet32 (PcdUnixFirmwareBlockSize),
        }
      }
    },
    {
      {
        0,
        0
      }
    }
  }
};

EFI_STATUS
GetFvbInfo (
  IN  UINT64                        FvLength,
  OUT EFI_FIRMWARE_VOLUME_HEADER    **FvbInfo
  )
{
  UINTN Index;

  for (Index = 0; Index < sizeof (mPlatformFvbMediaInfo) / sizeof (EFI_FVB_MEDIA_INFO); Index += 1) {
    if (mPlatformFvbMediaInfo[Index].FvLength == FvLength) {
      *FvbInfo = &mPlatformFvbMediaInfo[Index].FvbInfo;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
