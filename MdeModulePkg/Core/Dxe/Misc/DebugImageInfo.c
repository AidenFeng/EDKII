/** @file
  Support functions for managing debug image info table when loading and unloading
  images.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeMain.h"


EFI_DEBUG_IMAGE_INFO_TABLE_HEADER  mDebugInfoTableHeader = {
  0,          // volatile UINT32                 UpdateStatus;
  0,          // UINT32                          TableSize;
  NULL        // EFI_DEBUG_IMAGE_INFO            *EfiDebugImageInfoTable;
};

EFI_SYSTEM_TABLE_POINTER *mDebugTable = NULL;



/**
  Creates and initializes the DebugImageInfo Table.  Also creates the configuration
  table and registers it into the system table.

**/
VOID
CoreInitializeDebugImageInfoTable (
  VOID
  )
{
  EFI_STATUS                          Status;

  //
  // Allocate 4M aligned page for the structure and fill in the data.
  // Ideally we would update the CRC now as well, but the service may not yet be available.
  // See comments in the CoreUpdateDebugTableCrc32() function below for details.
  //
  mDebugTable = AllocateAlignedPages (EFI_SIZE_TO_PAGES (sizeof (EFI_SYSTEM_TABLE_POINTER)), FOUR_MEG_ALIGNMENT); 
  mDebugTable->Signature = EFI_SYSTEM_TABLE_SIGNATURE;
  mDebugTable->EfiSystemTableBase = (EFI_PHYSICAL_ADDRESS) (UINTN) gDxeCoreST;
  mDebugTable->Crc32 = 0;
  Status = CoreInstallConfigurationTable (&gEfiDebugImageInfoTableGuid, &mDebugInfoTableHeader);
  ASSERT_EFI_ERROR (Status);
}


/**
  Update the CRC32 in the Debug Table.
  Since the CRC32 service is made available by the Runtime driver, we have to
  wait for the Runtime Driver to be installed before the CRC32 can be computed.
  This function is called elsewhere by the core when the runtime architectural
  protocol is produced.

**/
VOID
CoreUpdateDebugTableCrc32 (
  VOID
  )
{
  ASSERT(mDebugTable != NULL);
  mDebugTable->Crc32 = 0;
  gBS->CalculateCrc32 ((VOID *)mDebugTable, sizeof (EFI_SYSTEM_TABLE_POINTER), &mDebugTable->Crc32);
}


/**
  Adds a new DebugImageInfo structure to the DebugImageInfo Table.  Re-Allocates
  the table if it's not large enough to accomidate another entry.

  @param  ImageInfoType  type of debug image information
  @param  LoadedImage    pointer to the loaded image protocol for the image being
                         loaded
  @param  ImageHandle    image handle for the image being loaded

**/
VOID
CoreNewDebugImageInfoEntry (
  IN  UINT32                      ImageInfoType,
  IN  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage,
  IN  EFI_HANDLE                  ImageHandle
  )
{
  EFI_DEBUG_IMAGE_INFO      *Table;
  EFI_DEBUG_IMAGE_INFO      *NewTable;
  UINTN                     Index;
  UINTN                     MaxTableIndex;
  UINTN                     TableSize;

  //
  // Set the flag indicating that we're in the process of updating the table.
  //
  mDebugInfoTableHeader.UpdateStatus |= EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;

  Table = mDebugInfoTableHeader.EfiDebugImageInfoTable;
  MaxTableIndex = mDebugInfoTableHeader.TableSize;

  for (Index = 0; Index < MaxTableIndex; Index++) {
    if (Table[Index].NormalImage == NULL) {
      //
      // We have found a free entry so exit the loop
      //
      break;
    }
  }
  if (Index == MaxTableIndex) {
    //
    //  Table is full, so re-allocate another page for a larger table...
    //
    TableSize = MaxTableIndex * EFI_DEBUG_TABLE_ENTRY_SIZE;
    NewTable = AllocateZeroPool (TableSize + EFI_PAGE_SIZE);
    if (NewTable == NULL) {
      mDebugInfoTableHeader.UpdateStatus &= ~EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;
      return;
    }
    //
    // Copy the old table into the new one
    //
    CopyMem (NewTable, Table, TableSize);
    //
    // Free the old table
    //
    CoreFreePool (Table);
    //
    // Update the table header
    //
    Table = NewTable;
    mDebugInfoTableHeader.EfiDebugImageInfoTable = NewTable;
    mDebugInfoTableHeader.TableSize += EFI_PAGE_SIZE / EFI_DEBUG_TABLE_ENTRY_SIZE;
  }
  //
  // Allocate data for new entry
  //
  Table[Index].NormalImage = AllocateZeroPool (sizeof (EFI_DEBUG_IMAGE_INFO_NORMAL));
  if (Table[Index].NormalImage != NULL) {
    //
    // Update the entry
    //
    Table[Index].NormalImage->ImageInfoType               = (UINT32) ImageInfoType;
    Table[Index].NormalImage->LoadedImageProtocolInstance = LoadedImage;
    Table[Index].NormalImage->ImageHandle                 = ImageHandle;
  }
  mDebugInfoTableHeader.UpdateStatus &= ~EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;
}



/**
  Removes and frees an entry from the DebugImageInfo Table.

  @param  ImageHandle    image handle for the image being unloaded

**/
VOID
CoreRemoveDebugImageInfoEntry (
  EFI_HANDLE ImageHandle
  )
{
  EFI_DEBUG_IMAGE_INFO  *Table;
  UINTN                 Index;

  mDebugInfoTableHeader.UpdateStatus |= EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;

  Table = mDebugInfoTableHeader.EfiDebugImageInfoTable;

  for (Index = 0; Index < mDebugInfoTableHeader.TableSize; Index++) {
    if (Table[Index].NormalImage != NULL && Table[Index].NormalImage->ImageHandle == ImageHandle) {
      //
      // Found a match. Free up the record, then NULL the pointer to indicate the slot
      // is free.
      //
      CoreFreePool (Table[Index].NormalImage);
      Table[Index].NormalImage = NULL;
      break;
    }
  }
  mDebugInfoTableHeader.UpdateStatus &= ~EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;
}


