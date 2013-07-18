/** @file
*
*  Copyright (c) 2013, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "CpuDxe.h"

/**
  Searches memory descriptors covered by given memory range.

  This function searches into the Gcd Memory Space for descriptors
  (from StartIndex to EndIndex) that contains the memory range
  specified by BaseAddress and Length.

  @param  MemorySpaceMap       Gcd Memory Space Map as array.
  @param  NumberOfDescriptors  Number of descriptors in map.
  @param  BaseAddress          BaseAddress for the requested range.
  @param  Length               Length for the requested range.
  @param  StartIndex           Start index into the Gcd Memory Space Map.
  @param  EndIndex             End index into the Gcd Memory Space Map.

  @retval EFI_SUCCESS          Search successfully.
  @retval EFI_NOT_FOUND        The requested descriptors does not exist.

**/
EFI_STATUS
SearchGcdMemorySpaces (
  IN EFI_GCD_MEMORY_SPACE_DESCRIPTOR    *MemorySpaceMap,
  IN UINTN                               NumberOfDescriptors,
  IN EFI_PHYSICAL_ADDRESS                BaseAddress,
  IN UINT64                              Length,
  OUT UINTN                             *StartIndex,
  OUT UINTN                             *EndIndex
  )
{
  UINTN           Index;

  *StartIndex = 0;
  *EndIndex   = 0;
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if ((BaseAddress >= MemorySpaceMap[Index].BaseAddress) &&
        (BaseAddress < (MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length))) {
      *StartIndex = Index;
    }
    if (((BaseAddress + Length - 1) >= MemorySpaceMap[Index].BaseAddress) &&
        ((BaseAddress + Length - 1) < (MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length))) {
      *EndIndex = Index;
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}


/**
  Sets the attributes for a specified range in Gcd Memory Space Map.

  This function sets the attributes for a specified range in
  Gcd Memory Space Map.

  @param  MemorySpaceMap       Gcd Memory Space Map as array
  @param  NumberOfDescriptors  Number of descriptors in map
  @param  BaseAddress          BaseAddress for the range
  @param  Length               Length for the range
  @param  Attributes           Attributes to set

  @retval EFI_SUCCESS          Memory attributes set successfully
  @retval EFI_NOT_FOUND        The specified range does not exist in Gcd Memory Space

**/
EFI_STATUS
SetGcdMemorySpaceAttributes (
  IN EFI_GCD_MEMORY_SPACE_DESCRIPTOR    *MemorySpaceMap,
  IN UINTN                               NumberOfDescriptors,
  IN EFI_PHYSICAL_ADDRESS                BaseAddress,
  IN UINT64                              Length,
  IN UINT64                              Attributes
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  UINTN                 StartIndex;
  UINTN                 EndIndex;
  EFI_PHYSICAL_ADDRESS  RegionStart;
  UINT64                RegionLength;

  DEBUG ((DEBUG_GCD, "SetGcdMemorySpaceAttributes[0x%lX; 0x%lX] = 0x%lX\n",
      BaseAddress, BaseAddress + Length, Attributes));

  //
  // Get all memory descriptors covered by the memory range
  //
  Status = SearchGcdMemorySpaces (
             MemorySpaceMap,
             NumberOfDescriptors,
             BaseAddress,
             Length,
             &StartIndex,
             &EndIndex
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Go through all related descriptors and set attributes accordingly
  //
  for (Index = StartIndex; Index <= EndIndex; Index++) {
    if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
      continue;
    }
    //
    // Calculate the start and end address of the overlapping range
    //
    if (BaseAddress >= MemorySpaceMap[Index].BaseAddress) {
      RegionStart = BaseAddress;
    } else {
      RegionStart = MemorySpaceMap[Index].BaseAddress;
    }
    if ((BaseAddress + Length - 1) < (MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length)) {
      RegionLength = BaseAddress + Length - RegionStart;
    } else {
      RegionLength = MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length - RegionStart;
    }
    //
    // Set memory attributes according to MTRR attribute and the original attribute of descriptor
    //
    gDS->SetMemorySpaceAttributes (
           RegionStart,
           RegionLength,
           (MemorySpaceMap[Index].Attributes & ~EFI_MEMORY_CACHETYPE_MASK) | (MemorySpaceMap[Index].Capabilities & Attributes)
           );
  }

  return EFI_SUCCESS;
}

/**
  This function modifies the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  BaseAddress      The physical address that is the start address of a memory region.
  @param  Length           The size in bytes of the memory region.
  @param  Attributes       The bit mask of attributes to set for the memory region.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not support for the memory resource
                                range specified by BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
CpuSetMemoryAttributes (
  IN EFI_CPU_ARCH_PROTOCOL    *This,
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes
  )
{
  DEBUG ((EFI_D_PAGE, "CpuSetMemoryAttributes(%lx, %lx, %lx)\n", BaseAddress, Length, Attributes));

  if ((BaseAddress & (SIZE_4KB - 1)) != 0) {
    // Minimum granularity is SIZE_4KB (4KB on ARM)
    DEBUG ((EFI_D_PAGE, "CpuSetMemoryAttributes(%lx, %lx, %lx): Minimum ganularity is SIZE_4KB\n", BaseAddress, Length, Attributes));
    return EFI_UNSUPPORTED;
  }

  return SetMemoryAttributes (BaseAddress, Length, Attributes, 0);
}

EFI_STATUS
EFIAPI
CpuConvertPagesToUncachedVirtualAddress (
  IN  VIRTUAL_UNCACHED_PAGES_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS              Address,
  IN  UINTN                             Length,
  IN  EFI_PHYSICAL_ADDRESS              VirtualMask,
  OUT UINT64                           *Attributes     OPTIONAL
  )
{
  EFI_STATUS                      Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR GcdDescriptor;

  if (Attributes != NULL) {
    Status = gDS->GetMemorySpaceDescriptor (Address, &GcdDescriptor);
    if (!EFI_ERROR (Status)) {
      *Attributes = GcdDescriptor.Attributes;
    }
  }

  //
  // Make this address range page fault if accessed. If it is a DMA buffer than this would
  // be the PCI address. Code should always use the CPU address, and we will or in VirtualMask
  // to that address.
  //
  Status = SetMemoryAttributes (Address, Length, EFI_MEMORY_WP, 0);
  if (!EFI_ERROR (Status)) {
    Status = SetMemoryAttributes (Address | VirtualMask, Length, EFI_MEMORY_UC, VirtualMask);
  }

  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "CpuConvertPagesToUncachedVirtualAddress()\n    Unmapped 0x%08lx Mapped 0x%08lx 0x%x bytes\n", Address, Address | VirtualMask, Length));

  return Status;
}


EFI_STATUS
EFIAPI
CpuReconvertPages (
  IN  VIRTUAL_UNCACHED_PAGES_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS              Address,
  IN  UINTN                             Length,
  IN  EFI_PHYSICAL_ADDRESS              VirtualMask,
  IN  UINT64                            Attributes
  )
{
  EFI_STATUS      Status;

  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "CpuReconvertPages(%lx, %x, %lx, %lx)\n", Address, Length, VirtualMask, Attributes));

  //
  // Unmap the aliased Address
  //
  Status = SetMemoryAttributes (Address | VirtualMask, Length, EFI_MEMORY_WP, 0);
  if (!EFI_ERROR (Status)) {
    //
    // Restore atttributes
    //
    Status = SetMemoryAttributes (Address, Length, Attributes, 0);
  }

  return Status;
}


VIRTUAL_UNCACHED_PAGES_PROTOCOL  gVirtualUncachedPages = {
  CpuConvertPagesToUncachedVirtualAddress,
  CpuReconvertPages
};
