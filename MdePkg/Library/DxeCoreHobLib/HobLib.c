/** @file
  HOB Library.

  Copyright (c) 2006 - 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


//
// The package level header files this module uses
//
#include <PiDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
//
// The Library classes this module consumes
//
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

#include "InternalHobLib.h"

/**
  Returns the pointer to the HOB list.

  This function returns the pointer to first HOB in the list.

  @return The pointer to the HOB list.

**/
VOID *
EFIAPI
GetHobList (
  VOID
  )
{
  return gHobList;
}

/**
  Returns the next instance of a HOB type from the starting HOB.

  This function searches the first instance of a HOB type from the starting HOB pointer.
  If there does not exist such HOB type from the starting HOB pointer, it will return NULL.
  In contrast with macro GET_NEXT_HOB(), this function does not skip the starting HOB pointer
  unconditionally: it returns HobStart back if HobStart itself meets the requirement;
  caller is required to use GET_NEXT_HOB() if it wishes to skip current HobStart.
  If HobStart is NULL, then ASSERT().

  @param  Type          The HOB type to return.
  @param  HobStart      The starting HOB pointer to search from.

  @return The next instance of a HOB type from the starting HOB.

**/
VOID *
EFIAPI
GetNextHob (
  IN UINT16                 Type,
  IN CONST VOID             *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  ASSERT (HobStart != NULL);

  Hob.Raw = (UINT8 *) HobStart;
  //
  // Parse the HOB list until end of list or matching type is found.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == Type) {
      return Hob.Raw;
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  return NULL;
}

/**
  Returns the first instance of a HOB type among the whole HOB list.

  This function searches the first instance of a HOB type among the whole HOB list.
  If there does not exist such HOB type in the HOB list, it will return NULL.

  @param  Type          The HOB type to return.

  @return The next instance of a HOB type from the starting HOB.

**/
VOID *
EFIAPI
GetFirstHob (
  IN UINT16                 Type
  )
{
  VOID      *HobList;

  HobList = GetHobList ();
  return GetNextHob (Type, HobList);
}

/**
  This function searches the first instance of a HOB from the starting HOB pointer.
  Such HOB should satisfy two conditions:
  its HOB type is EFI_HOB_TYPE_GUID_EXTENSION and its GUID Name equals to the input Guid.
  If there does not exist such HOB from the starting HOB pointer, it will return NULL.
  Caller is required to apply GET_GUID_HOB_DATA () and GET_GUID_HOB_DATA_SIZE ()
  to extract the data section and its size info respectively.
  In contrast with macro GET_NEXT_HOB(), this function does not skip the starting HOB pointer
  unconditionally: it returns HobStart back if HobStart itself meets the requirement;
  caller is required to use GET_NEXT_HOB() if it wishes to skip current HobStart.
  If Guid is NULL, then ASSERT().
  If HobStart is NULL, then ASSERT().

  @param  Guid          The GUID to match with in the HOB list.
  @param  HobStart      A pointer to a Guid.

  @return The next instance of the matched GUID HOB from the starting HOB.

**/
VOID *
EFIAPI
GetNextGuidHob (
  IN CONST EFI_GUID         *Guid,
  IN CONST VOID             *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  GuidHob;

  GuidHob.Raw = (UINT8 *) HobStart;
  while ((GuidHob.Raw = GetNextHob (EFI_HOB_TYPE_GUID_EXTENSION, GuidHob.Raw)) != NULL) {
    if (CompareGuid (Guid, &GuidHob.Guid->Name)) {
      break;
    }
    GuidHob.Raw = GET_NEXT_HOB (GuidHob);
  }
  return GuidHob.Raw;
}

/**
  This function searches the first instance of a HOB among the whole HOB list.
  Such HOB should satisfy two conditions:
  its HOB type is EFI_HOB_TYPE_GUID_EXTENSION and its GUID Name equals to the input Guid.
  If there does not exist such HOB from the starting HOB pointer, it will return NULL.
  Caller is required to apply GET_GUID_HOB_DATA () and GET_GUID_HOB_DATA_SIZE ()
  to extract the data section and its size info respectively.
  If Guid is NULL, then ASSERT().

  @param  Guid          The GUID to match with in the HOB list.

  @return The first instance of the matched GUID HOB among the whole HOB list.

**/
VOID *
EFIAPI
GetFirstGuidHob (
  IN CONST EFI_GUID         *Guid
  )
{
  VOID      *HobList;

  HobList = GetHobList ();
  return GetNextGuidHob (Guid, HobList);
}

/**
  Get the Boot Mode from the HOB list.

  This function returns the system boot mode information from the
  PHIT HOB in HOB list.

  @param  VOID

  @return The Boot Mode.

**/
EFI_BOOT_MODE
EFIAPI
GetBootModeHob (
  VOID
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE    *HandOffHob;

  HandOffHob = (EFI_HOB_HANDOFF_INFO_TABLE *) GetHobList ();

  return  HandOffHob->BootMode;
}
/**
  Builds a HOB for a loaded PE32 module.

  This function builds a HOB for a loaded PE32 module.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If ModuleName is NULL, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().

  @param  ModuleName              The GUID File Name of the module.
  @param  MemoryAllocationModule  The 64 bit physical address of the module.
  @param  ModuleLength            The length of the module in bytes.
  @param  EntryPoint              The 64 bit physical address of the module's entry point.

**/
VOID
EFIAPI
BuildModuleHob (
  IN CONST EFI_GUID         *ModuleName,
  IN EFI_PHYSICAL_ADDRESS   MemoryAllocationModule,
  IN UINT64                 ModuleLength,
  IN EFI_PHYSICAL_ADDRESS   EntryPoint
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB that describes a chunk of system memory.

  This function builds a HOB that describes a chunk of system memory.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  ResourceType        The type of resource described by this HOB.
  @param  ResourceAttribute   The resource attributes of the memory described by this HOB.
  @param  PhysicalStart       The 64 bit physical address of memory described by this HOB.
  @param  NumberOfBytes       The length of the memory described by this HOB in bytes.

**/
VOID
EFIAPI
BuildResourceDescriptorHob (
  IN EFI_RESOURCE_TYPE            ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  Builds a GUID HOB with a certain data length.

  This function builds a customized HOB tagged with a GUID for identification
  and returns the start address of GUID HOB data so that caller can fill the customized data.
  The HOB Header and Name field is already stripped.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If Guid is NULL, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().
  If DataLength >= (0x10000 - sizeof (EFI_HOB_GUID_TYPE)), then ASSERT().

  @param  Guid          The GUID to tag the customized HOB.
  @param  DataLength    The size of the data payload for the GUID HOB.

  @return The start address of GUID HOB data.

**/
VOID *
EFIAPI
BuildGuidHob (
  IN CONST EFI_GUID              *Guid,
  IN UINTN                       DataLength
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
  return NULL;
}

/**
  Copies a data buffer to a newly-built HOB.

  This function builds a customized HOB tagged with a GUID for identification,
  copies the input data to the HOB data field and returns the start address of the GUID HOB data.
  The HOB Header and Name field is already stripped.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If Guid is NULL, then ASSERT().
  If Data is NULL and DataLength > 0, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().
  If DataLength >= (0x10000 - sizeof (EFI_HOB_GUID_TYPE)), then ASSERT().

  @param  Guid          The GUID to tag the customized HOB.
  @param  Data          The data to be copied into the data field of the GUID HOB.
  @param  DataLength    The size of the data payload for the GUID HOB.

  @return The start address of GUID HOB data.

**/
VOID *
EFIAPI
BuildGuidDataHob (
  IN CONST EFI_GUID              *Guid,
  IN VOID                        *Data,
  IN UINTN                       DataLength
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
  return NULL;
}

/**
  Builds a Firmware Volume HOB.

  This function builds a Firmware Volume HOB.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The base address of the Firmware Volume.
  @param  Length        The size of the Firmware Volume in bytes.

**/
VOID
EFIAPI
BuildFvHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  Builds a Capsule Volume HOB.

  This function builds a Capsule Volume HOB.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The base address of the Capsule Volume.
  @param  Length        The size of the Capsule Volume in bytes.

**/
VOID
EFIAPI
BuildCvHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB for the CPU.

  This function builds a HOB for the CPU.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  SizeOfMemorySpace   The maximum physical memory addressability of the processor.
  @param  SizeOfIoSpace       The maximum physical I/O addressability of the processor.

**/
VOID
EFIAPI
BuildCpuHob (
  IN UINT8                       SizeOfMemorySpace,
  IN UINT8                       SizeOfIoSpace
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB for the Stack.

  This function builds a HOB for the stack.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The 64 bit physical address of the Stack.
  @param  Length        The length of the stack in bytes.

**/
VOID
EFIAPI
BuildStackHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB for the BSP store.

  This function builds a HOB for BSP store.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The 64 bit physical address of the BSP.
  @param  Length        The length of the BSP store in bytes.
  @param  MemoryType    Type of memory allocated by this HOB.

**/
VOID
EFIAPI
BuildBspStoreHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length,
  IN EFI_MEMORY_TYPE             MemoryType
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}

/**
  Builds a HOB for the memory allocation.

  This function builds a HOB for the memory allocation.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The 64 bit physical address of the memory.
  @param  Length        The length of the memory allocation in bytes.
  @param  MemoryType    Type of memory allocated by this HOB.

**/
VOID
EFIAPI
BuildMemoryAllocationHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length,
  IN EFI_MEMORY_TYPE             MemoryType
  )
{
  //
  // PEI HOB is read only for DXE phase
  //
  ASSERT (FALSE);
}
