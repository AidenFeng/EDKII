/** @file
  SMM CPU misc functions for x64 arch specific.
  
Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PiSmmCpuDxeSmm.h"

/**
  Initialize Gdt for all processors.
  
  @param[in]   Cr3          CR3 value.
  @param[out]  GdtStepSize  The step size for GDT table.

  @return GdtBase for processor 0.
          GdtBase for processor X is: GdtBase + (GdtStepSize * X)
**/
VOID *
InitGdt (
  IN  UINTN  Cr3,
  OUT UINTN  *GdtStepSize
  )
{
  UINTN                     Index;
  IA32_SEGMENT_DESCRIPTOR   *GdtDescriptor;
  UINTN                     TssBase;
  UINTN                     GdtTssTableSize;
  UINT8                     *GdtTssTables;
  UINTN                     GdtTableStepSize;

  //
  // For X64 SMM, we allocate separate GDT/TSS for each CPUs to avoid TSS load contention
  // on each SMI entry.
  //
  GdtTssTableSize = (gcSmiGdtr.Limit + 1 + TSS_SIZE + 7) & ~7; // 8 bytes aligned
  GdtTssTables = (UINT8*)AllocatePages (EFI_SIZE_TO_PAGES (GdtTssTableSize * gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus));
  ASSERT (GdtTssTables != NULL);
  GdtTableStepSize = GdtTssTableSize;

  for (Index = 0; Index < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus; Index++) {
    CopyMem (GdtTssTables + GdtTableStepSize * Index, (VOID*)(UINTN)gcSmiGdtr.Base, gcSmiGdtr.Limit + 1 + TSS_SIZE);

    //
    // Fixup TSS descriptors
    //
    TssBase = (UINTN)(GdtTssTables + GdtTableStepSize * Index + gcSmiGdtr.Limit + 1);
    GdtDescriptor = (IA32_SEGMENT_DESCRIPTOR *)(TssBase) - 2;
    GdtDescriptor->Bits.BaseLow = (UINT16)(UINTN)TssBase;
    GdtDescriptor->Bits.BaseMid = (UINT8)((UINTN)TssBase >> 16);
    GdtDescriptor->Bits.BaseHigh = (UINT8)((UINTN)TssBase >> 24);

    if (FeaturePcdGet (PcdCpuSmmStackGuard)) {
      //
      // Setup top of known good stack as IST1 for each processor.
      //
      *(UINTN *)(TssBase + TSS_X64_IST1_OFFSET) = (mSmmStackArrayBase + EFI_PAGE_SIZE + Index * mSmmStackSize);
    }
  }

  *GdtStepSize = GdtTableStepSize;
  return GdtTssTables;
}

/**
  Get Protected mode code segment from current GDT table.

  @return  Protected mode code segment value.
**/
UINT16
GetProtectedModeCS (
  VOID
  )
{
  IA32_DESCRIPTOR          GdtrDesc;
  IA32_SEGMENT_DESCRIPTOR  *GdtEntry;
  UINTN                    GdtEntryCount;
  UINT16                   Index;

  Index = (UINT16) -1;
  AsmReadGdtr (&GdtrDesc);
  GdtEntryCount = (GdtrDesc.Limit + 1) / sizeof (IA32_SEGMENT_DESCRIPTOR);
  GdtEntry = (IA32_SEGMENT_DESCRIPTOR *) GdtrDesc.Base;
  for (Index = 0; Index < GdtEntryCount; Index++) {
    if (GdtEntry->Bits.L == 0) {
      if (GdtEntry->Bits.Type > 8 && GdtEntry->Bits.L == 0) {
        break;
      }
    }
    GdtEntry++;
  }
  ASSERT (Index != -1);
  return Index * 8;
}

/**
  Transfer AP to safe hlt-loop after it finished restore CPU features on S3 patch.

  @param[in] ApHltLoopCode    The 32-bit address of the safe hlt-loop function.
  @param[in] TopOfStack       A pointer to the new stack to use for the ApHltLoopCode.

**/
VOID
TransferApToSafeState (
  IN UINT32             ApHltLoopCode,
  IN UINT32             TopOfStack
  )
{
  AsmDisablePaging64 (
    GetProtectedModeCS (),
    (UINT32) (UINTN) ApHltLoopCode,
    0,
    0,
    TopOfStack
    );
  //
  // It should never reach here
  //
  ASSERT (FALSE);
}


