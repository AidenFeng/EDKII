/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
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

#include <Library/ArmGicLib.h>
#include <Library/ArmMPCoreMailBoxLib.h>
#include <Chipset/ArmV7.h>

#include "PrePeiCore.h"

/*
 * This is the main function for secondary cores. They loop around until a non Null value is written to
 * SYS_FLAGS register.The SYS_FLAGS register is platform specific.
 * Note:The secondary cores, while executing secondary_main, assumes that:
 *      : SGI 0 is configured as Non-secure interrupt
 *      : Priority Mask is configured to allow SGI 0
 *      : Interrupt Distributor and CPU interfaces are enabled
 *
 */
VOID
EFIAPI
SecondaryMain (
  IN UINTN MpId
  )
{
  // Function pointer to Secondary Core entry point
  VOID (*secondary_start)(VOID);
  UINTN secondary_entry_addr=0;

  // Clear Secondary cores MailBox
  ArmClearMPCoreMailbox();

  while (secondary_entry_addr = ArmGetMPCoreMailbox(), secondary_entry_addr == 0) {
    ArmCallWFI();
    // Acknowledge the interrupt and send End of Interrupt signal.
    ArmGicAcknowledgeSgiFrom (PcdGet32(PcdGicInterruptInterfaceBase), PRIMARY_CORE_ID);
  }

  secondary_start = (VOID (*)())secondary_entry_addr;

  // Jump to secondary core entry point.
  secondary_start();

  // The secondaries shouldn't reach here
  ASSERT(FALSE);
}

VOID
EFIAPI
PrimaryMain (
  IN  EFI_PEI_CORE_ENTRY_POINT  PeiCoreEntryPoint
  )
{
  EFI_SEC_PEI_HAND_OFF        SecCoreData;
  UINTN                       PpiListSize;
  EFI_PEI_PPI_DESCRIPTOR      *PpiList;
  UINTN                       TemporaryRamBase;
  UINTN                       TemporaryRamSize;

  CreatePpiList (&PpiListSize, &PpiList);

  // Enable the GIC Distributor
  ArmGicEnableDistributor(PcdGet32(PcdGicDistributorBase));

  // If ArmVe has not been built as Standalone then we need to wake up the secondary cores
  if (FeaturePcdGet (PcdSendSgiToBringUpSecondaryCores)) {
    // Sending SGI to all the Secondary CPU interfaces
    ArmGicSendSgiTo (PcdGet32(PcdGicDistributorBase), ARM_GIC_ICDSGIR_FILTER_EVERYONEELSE, 0x0E);
  }

  // Adjust the Temporary Ram as the new Ppi List (Common + Platform Ppi Lists) is created at
  // the base of the primary core stack
  PpiListSize = ALIGN_VALUE(PpiListSize, 0x4);
  TemporaryRamBase = (UINTN)PcdGet32 (PcdCPUCoresStackBase) + PpiListSize;
  TemporaryRamSize = (UINTN)PcdGet32 (PcdCPUCorePrimaryStackSize) - PpiListSize;

  //
  // Bind this information into the SEC hand-off state
  // Note: this must be in sync with the stuff in the asm file
  // Note also:  HOBs (pei temp ram) MUST be above stack
  //
  SecCoreData.DataSize               = sizeof(EFI_SEC_PEI_HAND_OFF);
  SecCoreData.BootFirmwareVolumeBase = (VOID *)(UINTN)PcdGet32 (PcdFvBaseAddress);
  SecCoreData.BootFirmwareVolumeSize = PcdGet32 (PcdFvSize);
  SecCoreData.TemporaryRamBase       = (VOID *)TemporaryRamBase; // We run on the primary core (and so we use the first stack)
  SecCoreData.TemporaryRamSize       = TemporaryRamSize;
  SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize    = SecCoreData.TemporaryRamSize / 2;
  SecCoreData.StackBase              = (VOID *)((UINTN)(SecCoreData.TemporaryRamBase) + (SecCoreData.TemporaryRamSize/2));
  SecCoreData.StackSize              = SecCoreData.TemporaryRamSize / 2;

  // Jump to PEI core entry point
  (PeiCoreEntryPoint)(&SecCoreData, PpiList);
}
