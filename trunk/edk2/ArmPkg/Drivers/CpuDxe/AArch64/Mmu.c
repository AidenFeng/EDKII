/*++

Copyright (c) 2009, Hewlett-Packard Company. All rights reserved.<BR>
Portions copyright (c) 2010, Apple Inc. All rights reserved.<BR>
Portions copyright (c) 2011-2013, ARM Ltd. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


--*/

#include <Library/MemoryAllocationLib.h>
#include "CpuDxe.h"

#define TT_ATTR_INDX_INVALID    ((UINT32)~0)

STATIC
UINT64
GetFirstPageAttribute (
  IN UINT64  *FirstLevelTableAddress,
  IN UINTN    TableLevel
  )
{
  UINT64 FirstEntry;

  // Get the first entry of the table
  FirstEntry = *FirstLevelTableAddress;

  if ((FirstEntry & TT_TYPE_MASK) == TT_TYPE_TABLE_ENTRY) {
    // Only valid for Levels 0, 1 and 2
    ASSERT (TableLevel < 3);

    // Get the attribute of the subsequent table
    return GetFirstPageAttribute ((UINT64*)(FirstEntry & TT_ADDRESS_MASK_DESCRIPTION_TABLE), TableLevel + 1);
  } else if (((FirstEntry & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY) ||
             ((TableLevel == 3) && ((FirstEntry & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY_LEVEL3)))
  {
    return FirstEntry & TT_ATTR_INDX_MASK;
  } else {
    return TT_ATTR_INDX_INVALID;
  }
}

STATIC
UINT64
GetNextEntryAttribute (
  IN     UINT64 *TableAddress,
  IN     UINTN   EntryCount,
  IN     UINTN   TableLevel,
  IN     UINT64  BaseAddress,
  IN OUT UINT32 *PrevEntryAttribute,
  IN OUT UINT64 *StartGcdRegion
  )
{
  UINTN                             Index;
  UINT64                            Entry;
  UINT32                            EntryAttribute;
  UINT32                            EntryType;
  EFI_STATUS                        Status;
  UINTN                             NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap;

  // Get the memory space map from GCD
  MemorySpaceMap = NULL;
  Status = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  ASSERT_EFI_ERROR (Status);

  // We cannot get more than 3-level page table
  ASSERT (TableLevel <= 3);

  // While the top level table might not contain TT_ENTRY_COUNT entries;
  // the subsequent ones should be filled up
  for (Index = 0; Index < EntryCount; Index++) {
    Entry = TableAddress[Index];
    EntryType = Entry & TT_TYPE_MASK;
    EntryAttribute = Entry  & TT_ATTR_INDX_MASK;

    // If Entry is a Table Descriptor type entry then go through the sub-level table
    if ((EntryType == TT_TYPE_BLOCK_ENTRY) ||
        ((TableLevel == 3) && (EntryType == TT_TYPE_BLOCK_ENTRY_LEVEL3))) {
      if ((*PrevEntryAttribute == TT_ATTR_INDX_INVALID) || (EntryAttribute != *PrevEntryAttribute)) {
        if (*PrevEntryAttribute != TT_ATTR_INDX_INVALID) {
          // Update GCD with the last region
          SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors,
              *StartGcdRegion,
              (BaseAddress + (Index * TT_ADDRESS_AT_LEVEL(TableLevel)) - 1) - *StartGcdRegion,
              PageAttributeToGcdAttribute (EntryAttribute));
        }

        // Start of the new region
        *StartGcdRegion = BaseAddress + (Index * TT_ADDRESS_AT_LEVEL(TableLevel));
        *PrevEntryAttribute = EntryAttribute;
      } else {
        continue;
      }
    } else if (EntryType == TT_TYPE_TABLE_ENTRY) {
      // Table Entry type is only valid for Level 0, 1, 2
      ASSERT (TableLevel < 3);

      // Increase the level number and scan the sub-level table
      GetNextEntryAttribute ((UINT64*)(Entry & TT_ADDRESS_MASK_DESCRIPTION_TABLE),
                             TT_ENTRY_COUNT, TableLevel + 1,
                             (BaseAddress + (Index * TT_ADDRESS_AT_LEVEL(TableLevel))),
                             PrevEntryAttribute, StartGcdRegion);
    } else {
      if (*PrevEntryAttribute != TT_ATTR_INDX_INVALID) {
        // Update GCD with the last region
        SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors,
            *StartGcdRegion,
            (BaseAddress + (Index * TT_ADDRESS_AT_LEVEL(TableLevel)) - 1) - *StartGcdRegion,
            PageAttributeToGcdAttribute (EntryAttribute));

        // Start of the new region
        *StartGcdRegion = BaseAddress + (Index * TT_ADDRESS_AT_LEVEL(TableLevel));
        *PrevEntryAttribute = TT_ATTR_INDX_INVALID;
      }
    }
  }

  FreePool (MemorySpaceMap);

  return BaseAddress + (EntryCount * TT_ADDRESS_AT_LEVEL(TableLevel));
}

EFI_STATUS
SyncCacheConfig (
  IN  EFI_CPU_ARCH_PROTOCOL *CpuProtocol
  )
{
  EFI_STATUS                          Status;
  UINT32                              PageAttribute = 0;
  UINT64                             *FirstLevelTableAddress;
  UINTN                               TableLevel;
  UINTN                               TableCount;
  UINTN                               NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR    *MemorySpaceMap;
  UINTN                               Tcr;
  UINTN                               T0SZ;
  UINT64                              BaseAddressGcdRegion;
  UINT64                              EndAddressGcdRegion;

  // This code assumes MMU is enabled and filed with section translations
  ASSERT (ArmMmuEnabled ());

  //
  // Get the memory space map from GCD
  //
  MemorySpaceMap = NULL;
  Status = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  ASSERT_EFI_ERROR (Status);

  // The GCD implementation maintains its own copy of the state of memory space attributes.  GCD needs
  // to know what the initial memory space attributes are.  The CPU Arch. Protocol does not provide a
  // GetMemoryAttributes function for GCD to get this so we must resort to calling GCD (as if we were
  // a client) to update its copy of the attributes.  This is bad architecture and should be replaced
  // with a way for GCD to query the CPU Arch. driver of the existing memory space attributes instead.

  // Obtain page table base
  FirstLevelTableAddress = (UINT64*)(ArmGetTTBR0BaseAddress ());

  // Get Translation Control Register value
  Tcr = ArmGetTCR ();
  // Get Address Region Size
  T0SZ = Tcr & TCR_T0SZ_MASK;

  // Get the level of the first table for the indicated Address Region Size
  GetRootTranslationTableInfo (T0SZ, &TableLevel, &TableCount);

  // First Attribute of the Page Tables
  PageAttribute = GetFirstPageAttribute (FirstLevelTableAddress, TableLevel);

  // We scan from the start of the memory map (ie: at the address 0x0)
  BaseAddressGcdRegion = 0x0;
  EndAddressGcdRegion = GetNextEntryAttribute (FirstLevelTableAddress,
                                               TableCount, TableLevel,
                                               BaseAddressGcdRegion,
                                               &PageAttribute, &BaseAddressGcdRegion);

  // Update GCD with the last region
  SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors,
      BaseAddressGcdRegion,
      EndAddressGcdRegion - BaseAddressGcdRegion,
      PageAttributeToGcdAttribute (PageAttribute));

  FreePool (MemorySpaceMap);

  return EFI_SUCCESS;
}
