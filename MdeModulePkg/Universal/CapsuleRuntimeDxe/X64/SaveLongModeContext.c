/** @file
  Create the variable to save the base address of page table and stack
  for transferring into long mode in IA32 capsule PEI.

Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Protocol/Capsule.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/VariableLock.h>

#include <Guid/CapsuleVendor.h>
#include <Guid/AcpiS3Context.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/BaseLib.h>
#include <Library/LockBoxLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param  Size         Size of memory to allocate.
  
  @return Allocated address for output.

**/
VOID*
AllocateAcpiNvsMemoryBelow4G (
  IN   UINTN   Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer;

  Pages = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiACPIMemoryNVS,
                   Pages,
                   &Address
                   );
  ASSERT_EFI_ERROR (Status);

  Buffer = (VOID *) (UINTN) Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}

/**
  Register callback function upon VariableLockProtocol
  to lock EFI_CAPSULE_LONG_MODE_BUFFER_NAME variable to avoid malicious code to update it.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
VariableLockCapsuleLongModeBufferVariable (
  IN  EFI_EVENT                             Event,
  IN  VOID                                  *Context
  )
{
  EFI_STATUS                    Status;
  EDKII_VARIABLE_LOCK_PROTOCOL  *VariableLock;
  //
  // Mark EFI_CAPSULE_LONG_MODE_BUFFER_NAME variable to read-only if the Variable Lock protocol exists
  //
  Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **) &VariableLock);
  if (!EFI_ERROR (Status)) {
    Status = VariableLock->RequestToLock (VariableLock, EFI_CAPSULE_LONG_MODE_BUFFER_NAME, &gEfiCapsuleVendorGuid);
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  1. Allocate NVS memory for capsule PEIM to establish a 1:1 Virtual to Physical mapping.
  2. Allocate NVS memroy as a stack for capsule PEIM to transfer from 32-bit mdoe to 64-bit mode.

**/
VOID
EFIAPI
PrepareContextForCapsulePei (
  VOID
  )
{
  UINT32                                        RegEax;
  UINT32                                        RegEdx;
  UINTN                                         TotalPagesNum;
  UINT8                                         PhysicalAddressBits;
  VOID                                          *Hob;
  UINT32                                        NumberOfPml4EntriesNeeded;
  UINT32                                        NumberOfPdpEntriesNeeded;
  BOOLEAN                                       Page1GSupport;
  EFI_CAPSULE_LONG_MODE_BUFFER                  LongModeBuffer;
  EFI_STATUS                                    Status;
  VOID                                          *Registration;
  
  LongModeBuffer.PageTableAddress = (EFI_PHYSICAL_ADDRESS) PcdGet64 (PcdIdentifyMappingPageTablePtr);

  if (LongModeBuffer.PageTableAddress == 0x0) {
    //
    // Calculate the size of page table, allocate the memory, and set PcdIdentifyMappingPageTablePtr.
    //
    Page1GSupport = FALSE;
    if (PcdGetBool(PcdUse1GPageTable)) {
      AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
      if (RegEax >= 0x80000001) {
        AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
        if ((RegEdx & BIT26) != 0) {
          Page1GSupport = TRUE;
        }
      }
    }
    
    //
    // Get physical address bits supported.
    //
    Hob = GetFirstHob (EFI_HOB_TYPE_CPU);
    if (Hob != NULL) {
      PhysicalAddressBits = ((EFI_HOB_CPU *) Hob)->SizeOfMemorySpace;
    } else {
      AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
      if (RegEax >= 0x80000008) {
        AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
        PhysicalAddressBits = (UINT8) RegEax;
      } else {
        PhysicalAddressBits = 36;
      }
    }
    
    //
    // IA-32e paging translates 48-bit linear addresses to 52-bit physical addresses.
    //
    ASSERT (PhysicalAddressBits <= 52);
    if (PhysicalAddressBits > 48) {
      PhysicalAddressBits = 48;
    }
    
    //
    // Calculate the table entries needed.
    //
    if (PhysicalAddressBits <= 39 ) {
      NumberOfPml4EntriesNeeded = 1;
      NumberOfPdpEntriesNeeded = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 30));
    } else {
      NumberOfPml4EntriesNeeded = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 39));
      NumberOfPdpEntriesNeeded = 512;
    }
    
    if (!Page1GSupport) {
      TotalPagesNum = (NumberOfPdpEntriesNeeded + 1) * NumberOfPml4EntriesNeeded + 1;
    } else {
      TotalPagesNum = NumberOfPml4EntriesNeeded + 1;
    }
    
    LongModeBuffer.PageTableAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateAcpiNvsMemoryBelow4G (EFI_PAGES_TO_SIZE (TotalPagesNum));
    ASSERT (LongModeBuffer.PageTableAddress != 0);
    PcdSet64 (PcdIdentifyMappingPageTablePtr, LongModeBuffer.PageTableAddress); 
  }
  
  //
  // Allocate stack
  //
  LongModeBuffer.StackSize        = PcdGet32 (PcdCapsulePeiLongModeStackSize);
  LongModeBuffer.StackBaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateAcpiNvsMemoryBelow4G (PcdGet32 (PcdCapsulePeiLongModeStackSize));
  ASSERT (LongModeBuffer.StackBaseAddress != 0);  
  
  Status = gRT->SetVariable (
                  EFI_CAPSULE_LONG_MODE_BUFFER_NAME,
                  &gEfiCapsuleVendorGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (EFI_CAPSULE_LONG_MODE_BUFFER),
                  &LongModeBuffer
                  );
  if (!EFI_ERROR (Status)) {
      //
      // Register callback function upon VariableLockProtocol
      // to lock EFI_CAPSULE_LONG_MODE_BUFFER_NAME variable to avoid malicious code to update it.
      //
      EfiCreateProtocolNotifyEvent (
        &gEdkiiVariableLockProtocolGuid,
        TPL_CALLBACK,
        VariableLockCapsuleLongModeBufferVariable,
        NULL,
        &Registration
        );    
  } else {
      DEBUG ((EFI_D_ERROR, "FATAL ERROR: CapsuleLongModeBuffer cannot be saved: %r. Capsule in PEI may fail!\n", Status));
      gBS->FreePages (LongModeBuffer.StackBaseAddress, EFI_SIZE_TO_PAGES (LongModeBuffer.StackSize));
  }
}

/**
  Create the variable to save the base address of page table and stack
  for transferring into long mode in IA32 capsule PEI.
**/
VOID
SaveLongModeContext (
  VOID
  )
{
  if ((FeaturePcdGet(PcdSupportUpdateCapsuleReset)) && (FeaturePcdGet (PcdDxeIplSwitchToLongMode))) {
    //
    // Allocate memory for Capsule IA32 PEIM, it will create page table to transfer to long mode to access capsule above 4GB.
    //
    PrepareContextForCapsulePei ();
  }
}
