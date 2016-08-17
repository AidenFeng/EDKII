/** @file
  MP initialize support functions for PEI phase.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MpLib.h"
#include <Ppi/EndOfPeiPhase.h>
#include <Library/PeiServicesLib.h>

//
// Global PEI notify function descriptor on EndofPei event
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_NOTIFY_DESCRIPTOR mMpInitLibNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  CpuMpEndOfPeiCallback
};

/**
  Get pointer to CPU MP Data structure.

  @return  The pointer to CPU MP Data structure.
**/
CPU_MP_DATA *
GetCpuMpData (
  VOID
  )
{
  CPU_MP_DATA      *CpuMpData;

  CpuMpData = GetCpuMpDataFromGuidedHob ();
  ASSERT (CpuMpData != NULL);
  return CpuMpData;
}

/**
  Save the pointer to CPU MP Data structure.

  @param[in] CpuMpData  The pointer to CPU MP Data structure will be saved.
**/
VOID
SaveCpuMpData (
  IN CPU_MP_DATA   *CpuMpData
  )
{
  UINT64           Data64;
  //
  // Build location of CPU MP DATA buffer in HOB
  //
  Data64 = (UINT64) (UINTN) CpuMpData;
  BuildGuidDataHob (
    &mCpuInitMpLibHobGuid,
    (VOID *) &Data64,
    sizeof (UINT64)
    );
}

/**
  Get available system memory below 1MB by specified size.

  @param[in] PeiCpuMpData        Pointer to PEI CPU MP Data
**/
VOID
BackupAndPrepareWakeupBuffer(
  IN CPU_MP_DATA              *CpuMpData
  )
{
  CopyMem (
    (VOID *) CpuMpData->BackupBuffer,
    (VOID *) CpuMpData->WakeupBuffer,
    CpuMpData->BackupBufferSize
    );
  CopyMem (
    (VOID *) CpuMpData->WakeupBuffer,
    (VOID *) CpuMpData->AddressMap.RendezvousFunnelAddress,
    CpuMpData->AddressMap.RendezvousFunnelSize
    );
}

/**
  Restore wakeup buffer data.

  @param[in] PeiCpuMpData        Pointer to PEI CPU MP Data
**/
VOID
RestoreWakeupBuffer(
  IN CPU_MP_DATA              *CpuMpData
  )
{
  CopyMem (
    (VOID *) CpuMpData->WakeupBuffer,
    (VOID *) CpuMpData->BackupBuffer,
    CpuMpData->BackupBufferSize
    );
}

/**
  Notify function on End Of PEI PPI.

  On S3 boot, this function will restore wakeup buffer data.
  On normal boot, this function will flag wakeup buffer to be un-used type.

  @param[in]  PeiServices        The pointer to the PEI Services Table.
  @param[in]  NotifyDescriptor   Address of the notification descriptor data structure.
  @param[in]  Ppi                Address of the PPI that was installed.

  @retval EFI_SUCCESS        When everything is OK.
**/
EFI_STATUS
EFIAPI
CpuMpEndOfPeiCallback (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR    *NotifyDescriptor,
  IN VOID                         *Ppi
  )
{
  EFI_STATUS                Status;
  EFI_BOOT_MODE             BootMode;
  CPU_MP_DATA               *CpuMpData;
  EFI_PEI_HOB_POINTERS      Hob;
  EFI_HOB_MEMORY_ALLOCATION *MemoryHob;

  DEBUG ((DEBUG_INFO, "PeiMpInitLib: CpuMpEndOfPeiCallback () invoked\n"));

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  CpuMpData = GetCpuMpData ();
  if (BootMode != BOOT_ON_S3_RESUME) {
    //
    // Get the HOB list for processing
    //
    Hob.Raw = GetHobList ();
    //
    // Collect memory ranges
    //
    while (!END_OF_HOB_LIST (Hob)) {
      if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
        MemoryHob = Hob.MemoryAllocation;
        if (MemoryHob->AllocDescriptor.MemoryBaseAddress == CpuMpData->WakeupBuffer) {
          //
          // Flag this HOB type to un-used
          //
          GET_HOB_TYPE (Hob) = EFI_HOB_TYPE_UNUSED;
          break;
        }
      }
      Hob.Raw = GET_NEXT_HOB (Hob);
    }
  } else {
    CpuMpData->EndOfPeiFlag = TRUE;
    RestoreWakeupBuffer (CpuMpData);
  }
  return EFI_SUCCESS;
}

/**
  Check if AP wakeup buffer is overlapped with existing allocated buffer.

  @param[in]  WakeupBufferStart     AP wakeup buffer start address.
  @param[in]  WakeupBufferEnd       AP wakeup buffer end address.

  @retval  TRUE       There is overlap.
  @retval  FALSE      There is no overlap.
**/
BOOLEAN
CheckOverlapWithAllocatedBuffer (
  IN UINTN                WakeupBufferStart,
  IN UINTN                WakeupBufferEnd
  )
{
  EFI_PEI_HOB_POINTERS      Hob;
  EFI_HOB_MEMORY_ALLOCATION *MemoryHob;
  BOOLEAN                   Overlapped;
  UINTN                     MemoryStart;
  UINTN                     MemoryEnd;

  Overlapped = FALSE;
  //
  // Get the HOB list for processing
  //
  Hob.Raw = GetHobList ();
  //
  // Collect memory ranges
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      MemoryHob   = Hob.MemoryAllocation;
      MemoryStart = (UINTN) MemoryHob->AllocDescriptor.MemoryBaseAddress;
      MemoryEnd   = (UINTN) (MemoryHob->AllocDescriptor.MemoryBaseAddress +
                             MemoryHob->AllocDescriptor.MemoryLength);
      if (!((WakeupBufferStart >= MemoryEnd) || (WakeupBufferEnd <= MemoryStart))) {
        Overlapped = TRUE;
        break;
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  return Overlapped;
}

/**
  Get available system memory below 1MB by specified size.

  @param[in] WakeupBufferSize   Wakeup buffer size required

  @retval other   Return wakeup buffer address below 1MB.
  @retval -1      Cannot find free memory below 1MB.
**/
UINTN
GetWakeupBuffer (
  IN UINTN                WakeupBufferSize
  )
{
  EFI_PEI_HOB_POINTERS    Hob;
  UINTN                   WakeupBufferStart;
  UINTN                   WakeupBufferEnd;

  WakeupBufferSize = (WakeupBufferSize + SIZE_4KB - 1) & ~(SIZE_4KB - 1);

  //
  // Get the HOB list for processing
  //
  Hob.Raw = GetHobList ();

  //
  // Collect memory ranges
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if ((Hob.ResourceDescriptor->PhysicalStart < BASE_1MB) &&
          (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) &&
          ((Hob.ResourceDescriptor->ResourceAttribute &
            (EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED |
             EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED |
             EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED
             )) == 0)
           ) {
        //
        // Need memory under 1MB to be collected here
        //
        WakeupBufferEnd = (UINTN) (Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength);
        if (WakeupBufferEnd > BASE_1MB) {
          //
          // Wakeup buffer should be under 1MB
          //
          WakeupBufferEnd = BASE_1MB;
        }
        while (WakeupBufferEnd > WakeupBufferSize) {
          //
          // Wakeup buffer should be aligned on 4KB
          //
          WakeupBufferStart = (WakeupBufferEnd - WakeupBufferSize) & ~(SIZE_4KB - 1);
          if (WakeupBufferStart < Hob.ResourceDescriptor->PhysicalStart) {
            break;
          }
          if (CheckOverlapWithAllocatedBuffer (WakeupBufferStart, WakeupBufferEnd)) {
            //
            // If this range is overlapped with existing allocated buffer, skip it
            // and find the next range
            //
            WakeupBufferEnd -= WakeupBufferSize;
            continue;
          }
          DEBUG ((DEBUG_INFO, "WakeupBufferStart = %x, WakeupBufferSize = %x\n",
                               WakeupBufferStart, WakeupBufferSize));
          //
          // Create a memory allocation HOB.
          //
          BuildMemoryAllocationHob (
            WakeupBufferStart,
            WakeupBufferSize,
            EfiBootServicesData
            );
          return WakeupBufferStart;
        }
      }
    }
    //
    // Find the next HOB
    //
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return (UINTN) -1;
}

/**
  Allocate reset vector buffer.

  @param[in, out]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
AllocateResetVector (
  IN OUT CPU_MP_DATA          *CpuMpData
  )
{
  UINTN           ApResetVectorSize;

  if (CpuMpData->WakeupBuffer == (UINTN) -1) {
    ApResetVectorSize = CpuMpData->AddressMap.RendezvousFunnelSize +
                          sizeof (MP_CPU_EXCHANGE_INFO);

    CpuMpData->WakeupBuffer      = GetWakeupBuffer (ApResetVectorSize);
    CpuMpData->MpCpuExchangeInfo = (MP_CPU_EXCHANGE_INFO *) (UINTN)
                    (CpuMpData->WakeupBuffer + CpuMpData->AddressMap.RendezvousFunnelSize);
    BackupAndPrepareWakeupBuffer (CpuMpData);
  }

  if (CpuMpData->EndOfPeiFlag) {
    BackupAndPrepareWakeupBuffer (CpuMpData);
  }
}

/**
  Free AP reset vector buffer.

  @param[in]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
FreeResetVector (
  IN CPU_MP_DATA              *CpuMpData
  )
{
  if (CpuMpData->EndOfPeiFlag) {
    RestoreWakeupBuffer (CpuMpData);
  }
}

/**
  Initialize global data for MP support.

  @param[in] CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
InitMpGlobalData (
  IN CPU_MP_DATA               *CpuMpData
  )
{
  EFI_STATUS      Status;

  SaveCpuMpData (CpuMpData);
  //
  // Register an event for EndOfPei
  //
  Status  = PeiServicesNotifyPpi (&mMpInitLibNotifyList);
  ASSERT_EFI_ERROR (Status);
}

/**
  This service executes a caller provided function on all enabled APs.

  @param[in]  Procedure               A pointer to the function to be run on
                                      enabled APs of the system. See type
                                      EFI_AP_PROCEDURE.
  @param[in]  SingleThread            If TRUE, then all the enabled APs execute
                                      the function specified by Procedure one by
                                      one, in ascending order of processor handle
                                      number.  If FALSE, then all the enabled APs
                                      execute the function specified by Procedure
                                      simultaneously.
  @param[in]  WaitEvent               The event created by the caller with CreateEvent()
                                      service.  If it is NULL, then execute in
                                      blocking mode. BSP waits until all APs finish
                                      or TimeoutInMicroSeconds expires.  If it's
                                      not NULL, then execute in non-blocking mode.
                                      BSP requests the function specified by
                                      Procedure to be started on all the enabled
                                      APs, and go on executing immediately. If
                                      all return from Procedure, or TimeoutInMicroSeconds
                                      expires, this event is signaled. The BSP
                                      can use the CheckEvent() or WaitForEvent()
                                      services to check the state of event.  Type
                                      EFI_EVENT is defined in CreateEvent() in
                                      the Unified Extensible Firmware Interface
                                      Specification.
  @param[in]  TimeoutInMicrosecsond   Indicates the time limit in microseconds for
                                      APs to return from Procedure, either for
                                      blocking or non-blocking mode. Zero means
                                      infinity.  If the timeout expires before
                                      all APs return from Procedure, then Procedure
                                      on the failed APs is terminated. All enabled
                                      APs are available for next function assigned
                                      by MpInitLibStartupAllAPs() or
                                      MPInitLibStartupThisAP().
                                      If the timeout expires in blocking mode,
                                      BSP returns EFI_TIMEOUT.  If the timeout
                                      expires in non-blocking mode, WaitEvent
                                      is signaled with SignalEvent().
  @param[in]  ProcedureArgument       The parameter passed into Procedure for
                                      all APs.
  @param[out] FailedCpuList           If NULL, this parameter is ignored. Otherwise,
                                      if all APs finish successfully, then its
                                      content is set to NULL. If not all APs
                                      finish before timeout expires, then its
                                      content is set to address of the buffer
                                      holding handle numbers of the failed APs.
                                      The buffer is allocated by MP Initialization
                                      library, and it's the caller's responsibility to
                                      free the buffer with FreePool() service.
                                      In blocking mode, it is ready for consumption
                                      when the call returns. In non-blocking mode,
                                      it is ready when WaitEvent is signaled.  The
                                      list of failed CPU is terminated by
                                      END_OF_CPU_LIST.

  @retval EFI_SUCCESS             In blocking mode, all APs have finished before
                                  the timeout expired.
  @retval EFI_SUCCESS             In non-blocking mode, function has been dispatched
                                  to all enabled APs.
  @retval EFI_UNSUPPORTED         A non-blocking mode request was made after the
                                  UEFI event EFI_EVENT_GROUP_READY_TO_BOOT was
                                  signaled.
  @retval EFI_UNSUPPORTED         WaitEvent is not NULL if non-blocking mode is not
                                  supported.
  @retval EFI_DEVICE_ERROR        Caller processor is AP.
  @retval EFI_NOT_STARTED         No enabled APs exist in the system.
  @retval EFI_NOT_READY           Any enabled APs are busy.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before
                                  all enabled APs have finished.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.

**/
EFI_STATUS
EFIAPI
MpInitLibStartupAllAPs (
  IN  EFI_AP_PROCEDURE          Procedure,
  IN  BOOLEAN                   SingleThread,
  IN  EFI_EVENT                 WaitEvent               OPTIONAL,
  IN  UINTN                     TimeoutInMicroseconds,
  IN  VOID                      *ProcedureArgument      OPTIONAL,
  OUT UINTN                     **FailedCpuList         OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This service lets the caller get one enabled AP to execute a caller-provided
  function.

  @param[in]  Procedure               A pointer to the function to be run on the
                                      designated AP of the system. See type
                                      EFI_AP_PROCEDURE.
  @param[in]  ProcessorNumber         The handle number of the AP. The range is
                                      from 0 to the total number of logical
                                      processors minus 1. The total number of
                                      logical processors can be retrieved by
                                      MpInitLibGetNumberOfProcessors().
  @param[in]  WaitEvent               The event created by the caller with CreateEvent()
                                      service.  If it is NULL, then execute in
                                      blocking mode. BSP waits until this AP finish
                                      or TimeoutInMicroSeconds expires.  If it's
                                      not NULL, then execute in non-blocking mode.
                                      BSP requests the function specified by
                                      Procedure to be started on this AP,
                                      and go on executing immediately. If this AP
                                      return from Procedure or TimeoutInMicroSeconds
                                      expires, this event is signaled. The BSP
                                      can use the CheckEvent() or WaitForEvent()
                                      services to check the state of event.  Type
                                      EFI_EVENT is defined in CreateEvent() in
                                      the Unified Extensible Firmware Interface
                                      Specification.
  @param[in]  TimeoutInMicrosecsond   Indicates the time limit in microseconds for
                                      this AP to finish this Procedure, either for
                                      blocking or non-blocking mode. Zero means
                                      infinity.  If the timeout expires before
                                      this AP returns from Procedure, then Procedure
                                      on the AP is terminated. The
                                      AP is available for next function assigned
                                      by MpInitLibStartupAllAPs() or
                                      MpInitLibStartupThisAP().
                                      If the timeout expires in blocking mode,
                                      BSP returns EFI_TIMEOUT.  If the timeout
                                      expires in non-blocking mode, WaitEvent
                                      is signaled with SignalEvent().
  @param[in]  ProcedureArgument       The parameter passed into Procedure on the
                                      specified AP.
  @param[out] Finished                If NULL, this parameter is ignored.  In
                                      blocking mode, this parameter is ignored.
                                      In non-blocking mode, if AP returns from
                                      Procedure before the timeout expires, its
                                      content is set to TRUE. Otherwise, the
                                      value is set to FALSE. The caller can
                                      determine if the AP returned from Procedure
                                      by evaluating this value.

  @retval EFI_SUCCESS             In blocking mode, specified AP finished before
                                  the timeout expires.
  @retval EFI_SUCCESS             In non-blocking mode, the function has been
                                  dispatched to specified AP.
  @retval EFI_UNSUPPORTED         A non-blocking mode request was made after the
                                  UEFI event EFI_EVENT_GROUP_READY_TO_BOOT was
                                  signaled.
  @retval EFI_UNSUPPORTED         WaitEvent is not NULL if non-blocking mode is not
                                  supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before
                                  the specified AP has finished.
  @retval EFI_NOT_READY           The specified AP is busy.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.

**/
EFI_STATUS
EFIAPI
MpInitLibStartupThisAP (
  IN  EFI_AP_PROCEDURE          Procedure,
  IN  UINTN                     ProcessorNumber,
  IN  EFI_EVENT                 WaitEvent               OPTIONAL,
  IN  UINTN                     TimeoutInMicroseconds,
  IN  VOID                      *ProcedureArgument      OPTIONAL,
  OUT BOOLEAN                   *Finished               OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes. This call can only be performed
  by the current BSP.

  @param[in] ProcessorNumber   The handle number of AP that is to become the new
                               BSP. The range is from 0 to the total number of
                               logical processors minus 1. The total number of
                               logical processors can be retrieved by
                               MpInitLibGetNumberOfProcessors().
  @param[in] EnableOldBSP      If TRUE, then the old BSP will be listed as an
                               enabled AP. Otherwise, it will be disabled.

  @retval EFI_SUCCESS             BSP successfully switched.
  @retval EFI_UNSUPPORTED         Switching the BSP cannot be completed prior to
                                  this service returning.
  @retval EFI_UNSUPPORTED         Switching the BSP is not supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the current BSP or
                                  a disabled AP.
  @retval EFI_NOT_READY           The specified AP is busy.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
EFIAPI
MpInitLibSwitchBSP (
  IN UINTN                     ProcessorNumber,
  IN  BOOLEAN                  EnableOldBSP
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This service lets the caller enable or disable an AP from this point onward.
  This service may only be called from the BSP.

  @param[in] ProcessorNumber   The handle number of AP.
                               The range is from 0 to the total number of
                               logical processors minus 1. The total number of
                               logical processors can be retrieved by
                               MpInitLibGetNumberOfProcessors().
  @param[in] EnableAP          Specifies the new state for the processor for
                               enabled, FALSE for disabled.
  @param[in] HealthFlag        If not NULL, a pointer to a value that specifies
                               the new health status of the AP. This flag
                               corresponds to StatusFlag defined in
                               EFI_MP_SERVICES_PROTOCOL.GetProcessorInfo(). Only
                               the PROCESSOR_HEALTH_STATUS_BIT is used. All other
                               bits are ignored.  If it is NULL, this parameter
                               is ignored.

  @retval EFI_SUCCESS             The specified AP was enabled or disabled successfully.
  @retval EFI_UNSUPPORTED         Enabling or disabling an AP cannot be completed
                                  prior to this service returning.
  @retval EFI_UNSUPPORTED         Enabling or disabling an AP is not supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_NOT_FOUND           Processor with the handle specified by ProcessorNumber
                                  does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
EFIAPI
MpInitLibEnableDisableAP (
  IN  UINTN                     ProcessorNumber,
  IN  BOOLEAN                   EnableAP,
  IN  UINT32                    *HealthFlag OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}


