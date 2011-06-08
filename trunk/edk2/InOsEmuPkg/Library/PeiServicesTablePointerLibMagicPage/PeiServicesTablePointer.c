/** @file
  PEI Services Table Pointer Library.
  
  Store PEI Services Table pointer via gInOsEmuPkgTokenSpaceGuid.PcdPeiServicesTablePage.
  This emulates a platform SRAM. The PI mechaism does not work in the emulator due to
  lack of privledge.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  Portiions copyrigth (c) 2011, Apple Inc. All rights reserved. 
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/DebugLib.h>
#include <Library/EmuMagicPageLib.h>


/**
  Caches a pointer PEI Services Table. 
 
  Caches the pointer to the PEI Services Table specified by PeiServicesTablePointer 
  in a CPU specific manner as specified in the CPU binding section of the Platform Initialization 
  Pre-EFI Initialization Core Interface Specification. 
  
  If PeiServicesTablePointer is NULL, then ASSERT().
  
  @param    PeiServicesTablePointer   The address of PeiServices pointer.
**/
VOID
EFIAPI
SetPeiServicesTablePointer (
  IN CONST EFI_PEI_SERVICES ** PeiServicesTablePointer
  )
{
  ASSERT (PeiServicesTablePointer != NULL);
  ASSERT (*PeiServicesTablePointer != NULL);
  EMU_MAGIC_PAGE()->PeiServicesTablePointer = PeiServicesTablePointer;
}

/**
  Retrieves the cached value of the PEI Services Table pointer.

  Returns the cached value of the PEI Services Table pointer in a CPU specific manner 
  as specified in the CPU binding section of the Platform Initialization Pre-EFI 
  Initialization Core Interface Specification.
  
  If the cached PEI Services Table pointer is NULL, then ASSERT().

  @return  The pointer to PeiServices.

**/
CONST EFI_PEI_SERVICES **
EFIAPI
GetPeiServicesTablePointer (
  VOID
  )
{
  CONST EFI_PEI_SERVICES **PeiServicesTablePointer;
  
  PeiServicesTablePointer = EMU_MAGIC_PAGE()->PeiServicesTablePointer;
  ASSERT (PeiServicesTablePointer != NULL);
  ASSERT (*PeiServicesTablePointer != NULL);
  return PeiServicesTablePointer;
}



