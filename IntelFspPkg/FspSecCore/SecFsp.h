/** @file

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SEC_FSP_H_
#define _SEC_FSPE_H_

#include <PiPei.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/SerialPortLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FspCommonLib.h>

#include <FspApi.h>

#define FSP_MCUD_SIGNATURE  SIGNATURE_32 ('M', 'C', 'U', 'D')
#define FSP_PER0_SIGNATURE  SIGNATURE_32 ('P', 'E', 'R', '0')

/**

  Calculate the FSP IDT gate descriptor.

  @param[in] IdtEntryTemplate     IDT gate descriptor template.

  @return                     FSP specific IDT gate descriptor.

**/
UINT64
FspGetExceptionHandler(
  IN  UINT64  IdtEntryTemplate
  );

/**

  Initialize the FSP global data region.
  It needs to be done as soon as possible after the stack is setup.

  @param[in,out] PeiFspData             Pointer of the FSP global data.
  @param[in]     BootloaderStack        Bootloader stack.
  @param[in]     ApiIdx                 The index of the FSP API.

**/
VOID
FspGlobalDataInit (
  IN OUT  FSP_GLOBAL_DATA    *PeiFspData,
  IN UINT32                   BootloaderStack,
  IN UINT8                    ApiIdx
  );


/**

  Adjust the FSP data pointers after the stack is migrated to memory.

  @param[in] OffsetGap             The offset gap between the old stack and the new stack.

**/
VOID
FspDataPointerFixUp (
  IN UINT32   OffsetGap
  );


/**
  This interface returns the base address of FSP binary.

  @return   FSP binary base address.

**/
UINT32
EFIAPI
GetFspBaseAddress (
  VOID
  );

/**
  This function gets the FSP UPD region offset in flash.

  @return the offset of the UPD region.

**/
UINT32
EFIAPI
GetFspUpdRegionOffset (
  VOID
  );

#endif
