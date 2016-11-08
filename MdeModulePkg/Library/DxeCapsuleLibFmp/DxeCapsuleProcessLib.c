/** @file
  DXE capsule process.

  Caution: This module requires additional review when modified.
  This module will have external input - capsule image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  ProcessCapsules(), ProcessTheseCapsules() will receive untrusted
  input and do basic validation.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Protocol/EsrtManagement.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/CapsuleLib.h>

#include <IndustryStandard/WindowsUxCapsule.h>

/**
  Return if this FMP is a system FMP or a device FMP, based upon CapsuleHeader.

  @param[in] CapsuleHeader A pointer to EFI_CAPSULE_HEADER

  @retval TRUE  It is a system FMP.
  @retval FALSE It is a device FMP.
**/
BOOLEAN
IsFmpCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  );

/**
  Validate Fmp capsules layout.

  Caution: This function may receive untrusted input.

  This function assumes the caller validated the capsule by using
  IsValidCapsuleHeader(), so that all fields in EFI_CAPSULE_HEADER are correct.
  The capsule buffer size is CapsuleHeader->CapsuleImageSize.

  This function validates the fields in EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER
  and EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER.

  This function need support nested FMP capsule.

  @param[in]   CapsuleHeader        Points to a capsule header.
  @param[out]  EmbeddedDriverCount  The EmbeddedDriverCount in the FMP capsule.

  @retval EFI_SUCESS             Input capsule is a correct FMP capsule.
  @retval EFI_INVALID_PARAMETER  Input capsule is not a correct FMP capsule.
**/
EFI_STATUS
ValidateFmpCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader,
  OUT UINT16             *EmbeddedDriverCount OPTIONAL
  );

/**
  Validate if it is valid capsule header

  This function assumes the caller provided correct CapsuleHeader pointer
  and CapsuleSize.

  This function validates the fields in EFI_CAPSULE_HEADER.

  @param[in]  CapsuleHeader    Points to a capsule header.
  @param[in]  CapsuleSize      Size of the whole capsule image.

**/
BOOLEAN
IsValidCapsuleHeader (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader,
  IN UINT64              CapsuleSize
  );

extern BOOLEAN                   mDxeCapsuleLibEndOfDxe;
extern BOOLEAN                   mAreAllImagesProcessed;
BOOLEAN                          mNeedReset;

/**

  This routine is called to process capsules.

  Caution: This function may receive untrusted input.

  Each individual capsule result is recorded in capsule record variable.

  @param[in]  NeedBlockDriver         TRUE: Need skip the FMP capsules with non zero EmbeddedDriverCount.
                                      FALSE: No need to skip any FMP capsules.

  @retval EFI_SUCCESS             There is no error when processing capsules.
  @retval EFI_OUT_OF_RESOURCES    No enough resource to process capsules.

**/
EFI_STATUS
ProcessTheseCapsules (
  IN BOOLEAN  NeedBlockDriver
  )
{
  EFI_STATUS                  Status;
  EFI_PEI_HOB_POINTERS        HobPointer;
  EFI_CAPSULE_HEADER          *CapsuleHeader;
  UINT32                      Size;
  UINT32                      CapsuleNumber;
  UINT32                      CapsuleTotalNumber;
  EFI_CAPSULE_TABLE           *CapsuleTable;
  UINT32                      Index;
  UINT32                      CacheIndex;
  UINT32                      CacheNumber;
  VOID                        **CapsulePtr;
  VOID                        **CapsulePtrCache;
  EFI_GUID                    *CapsuleGuidCache;
  EFI_STATUS                  *CapsuleStatusArray;
  BOOLEAN                     DisplayCapsuleExist;
  ESRT_MANAGEMENT_PROTOCOL    *EsrtManagement;
  UINT16                      EmbeddedDriverCount;

  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, (EFI_SOFTWARE | PcdGet32(PcdStatusCodeSubClassCapsule) | PcdGet32(PcdCapsuleStatusCodeProcessCapsulesBegin)));

  CapsuleNumber       = 0;
  CapsuleTotalNumber  = 0;
  CacheIndex          = 0;
  CacheNumber         = 0;
  CapsulePtr          = NULL;
  CapsulePtrCache     = NULL;
  CapsuleGuidCache    = NULL;
  DisplayCapsuleExist = FALSE;
  EsrtManagement      = NULL;

  Status = EFI_SUCCESS;
  //
  // Find all capsule images from hob
  //
  HobPointer.Raw = GetHobList ();
  while ((HobPointer.Raw = GetNextHob (EFI_HOB_TYPE_UEFI_CAPSULE, HobPointer.Raw)) != NULL) {
    if (!IsValidCapsuleHeader((VOID *)(UINTN)HobPointer.Capsule->BaseAddress, HobPointer.Capsule->Length)) {
      HobPointer.Header->HobType = EFI_HOB_TYPE_UNUSED; // Mark this hob as invalid
    } else {
      CapsuleTotalNumber++;
    }
    HobPointer.Raw = GET_NEXT_HOB (HobPointer);
  }

  if (CapsuleTotalNumber == 0) {
    //
    // We didn't find a hob, so had no errors.
    //
    DEBUG ((DEBUG_ERROR, "We can not find capsule data in capsule update boot mode.\n"));
    Status = EFI_SUCCESS;
    goto Done;
  }

  //
  // Init temp Capsule Data table.
  //
  CapsulePtr       = (VOID **) AllocateZeroPool (sizeof (VOID *) * CapsuleTotalNumber);
  ASSERT (CapsulePtr != NULL);
  if (CapsulePtr == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }
  CapsulePtrCache  = (VOID **) AllocateZeroPool (sizeof (VOID *) * CapsuleTotalNumber);
  ASSERT (CapsulePtrCache != NULL);
  if (CapsulePtrCache == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }
  CapsuleGuidCache = (EFI_GUID *) AllocateZeroPool (sizeof (EFI_GUID) * CapsuleTotalNumber);
  ASSERT (CapsuleGuidCache != NULL);
  if (CapsuleGuidCache == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }
  CapsuleStatusArray = (EFI_STATUS *) AllocateZeroPool (sizeof (EFI_STATUS) * CapsuleTotalNumber);
  ASSERT (CapsuleStatusArray != NULL);
  if (CapsuleStatusArray == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Find all capsule images from hob
  //
  HobPointer.Raw = GetHobList ();
  while ((HobPointer.Raw = GetNextHob (EFI_HOB_TYPE_UEFI_CAPSULE, HobPointer.Raw)) != NULL) {
    CapsulePtr [CapsuleNumber++] = (VOID *) (UINTN) HobPointer.Capsule->BaseAddress;
    HobPointer.Raw = GET_NEXT_HOB (HobPointer);
  }

  //
  // Check the capsule flags,if contains CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE, install
  // capsuleTable to configure table with EFI_CAPSULE_GUID
  //

  //
  // Capsules who have CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE always are used for operating
  // System to have information persist across a system reset. EFI System Table must
  // point to an array of capsules that contains the same CapsuleGuid value. And agents
  // searching for this type capsule will look in EFI System Table and search for the
  // capsule's Guid and associated pointer to retrieve the data. Two steps below describes
  // how to sorting the capsules by the unique guid and install the array to EFI System Table.
  // Firstly, Loop for all coalesced capsules, record unique CapsuleGuids and cache them in an
  // array for later sorting capsules by CapsuleGuid.
  //
  for (Index = 0; Index < CapsuleTotalNumber; Index++) {
    CapsuleStatusArray [Index] = EFI_UNSUPPORTED;
    CapsuleHeader = (EFI_CAPSULE_HEADER*) CapsulePtr [Index];
    if ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) != 0) {
      //
      // For each capsule, we compare it with known CapsuleGuid in the CacheArray.
      // If already has the Guid, skip it. Whereas, record it in the CacheArray as
      // an additional one.
      //
      CacheIndex = 0;
      while (CacheIndex < CacheNumber) {
        if (CompareGuid(&CapsuleGuidCache[CacheIndex],&CapsuleHeader->CapsuleGuid)) {
          break;
        }
        CacheIndex++;
      }
      if (CacheIndex == CacheNumber) {
        CopyMem(&CapsuleGuidCache[CacheNumber++],&CapsuleHeader->CapsuleGuid,sizeof(EFI_GUID));
      }
    }
  }

  //
  // Secondly, for each unique CapsuleGuid in CacheArray, gather all coalesced capsules
  // whose guid is the same as it, and malloc memory for an array which preceding
  // with UINT32. The array fills with entry point of capsules that have the same
  // CapsuleGuid, and UINT32 represents the size of the array of capsules. Then install
  // this array into EFI System Table, so that agents searching for this type capsule
  // will look in EFI System Table and search for the capsule's Guid and associated
  // pointer to retrieve the data.
  //
  CacheIndex = 0;
  while (CacheIndex < CacheNumber) {
    CapsuleNumber = 0;
    for (Index = 0; Index < CapsuleTotalNumber; Index++) {
      CapsuleHeader = (EFI_CAPSULE_HEADER*) CapsulePtr [Index];
      if ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) != 0) {
        if (CompareGuid (&CapsuleGuidCache[CacheIndex], &CapsuleHeader->CapsuleGuid)) {
          //
          // Cache Caspuleheader to the array, this array is uniqued with certain CapsuleGuid.
          //
          CapsulePtrCache[CapsuleNumber++] = (VOID*)CapsuleHeader;
          //
          // When a Capsule is listed in CapsulePtrCache, it will be reported in ConfigurationTable
          // So, report the CapsuleStatus as "processed successfully".
          //
          CapsuleStatusArray [Index] = EFI_SUCCESS;
        }
      }
    }
    if (CapsuleNumber != 0) {
      Size = sizeof(EFI_CAPSULE_TABLE) + (CapsuleNumber - 1) * sizeof(VOID*);
      CapsuleTable = AllocateRuntimePool (Size);
      ASSERT (CapsuleTable != NULL);
      if (CapsuleTable == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      CapsuleTable->CapsuleArrayNumber =  CapsuleNumber;
      CopyMem(&CapsuleTable->CapsulePtr[0], CapsulePtrCache, CapsuleNumber * sizeof(VOID*));
      Status = gBS->InstallConfigurationTable (&CapsuleGuidCache[CacheIndex], (VOID*)CapsuleTable);
      ASSERT_EFI_ERROR (Status);
    }
    CacheIndex++;
  }

  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, (EFI_SOFTWARE | PcdGet32(PcdStatusCodeSubClassCapsule) | PcdGet32(PcdCapsuleStatusCodeUpdatingFirmware)));

  //
  // If Windows UX capsule exist, process it first
  //
  for (Index = 0; Index < CapsuleTotalNumber; Index++) {
    CapsuleHeader = (EFI_CAPSULE_HEADER*) CapsulePtr [Index];
    if (CompareGuid(&CapsuleHeader->CapsuleGuid ,&gWindowsUxCapsuleGuid)) {
      DEBUG ((DEBUG_INFO, "ProcessCapsuleImage (Ux) - 0x%x\n", CapsuleHeader));
      DisplayCapsuleExist = TRUE;
      DEBUG ((DEBUG_INFO, "Display logo capsule is found.\n"));
      Status = ProcessCapsuleImage (CapsuleHeader);
      DEBUG((DEBUG_INFO, "ProcessCapsuleImage (Ux) - %r\n", Status));
      CapsuleStatusArray [Index] = Status;
      break;
    }
  }

  if (!DisplayCapsuleExist) {
    //
    // Display Capsule not found. Display the default string.
    //
    Print (L"Updating the firmware ......\r\n");
  }

  //
  // All capsules left are recognized by platform.
  //
  for (Index = 0; Index < CapsuleTotalNumber; Index++) {
    CapsuleHeader = (EFI_CAPSULE_HEADER*) CapsulePtr [Index];
    if (!CompareGuid(&CapsuleHeader->CapsuleGuid ,&gWindowsUxCapsuleGuid)) {
      //
      // Call capsule library to process capsule image.
      //
      EmbeddedDriverCount = 0;
      if (IsFmpCapsule(CapsuleHeader)) {
        Status = ValidateFmpCapsule(CapsuleHeader, &EmbeddedDriverCount);
        if (EFI_ERROR(Status)) {
          DEBUG((DEBUG_ERROR, "ValidateFmpCapsule failed. Ignore!\n"));
          continue;
        }
      }

      if ((!NeedBlockDriver) || (EmbeddedDriverCount == 0)) {
        DEBUG((DEBUG_INFO, "ProcessCapsuleImage - 0x%x\n", CapsuleHeader));
        Status = ProcessCapsuleImage (CapsuleHeader);
        CapsuleStatusArray [Index] = Status;
        DEBUG((DEBUG_INFO, "ProcessCapsuleImage - %r\n", Status));

        if (EFI_ERROR(Status)) {
          REPORT_STATUS_CODE(EFI_ERROR_CODE, (EFI_SOFTWARE | PcdGet32(PcdStatusCodeSubClassCapsule) | PcdGet32(PcdCapsuleStatusCodeUpdateFirmwareFailed)));
          DEBUG ((DEBUG_ERROR, "Capsule process failed. reset the system!\n"));
          Print (L"Firmware update failed...\r\n");
        } else {
          REPORT_STATUS_CODE(EFI_PROGRESS_CODE, (EFI_SOFTWARE | PcdGet32(PcdStatusCodeSubClassCapsule) | PcdGet32(PcdCapsuleStatusCodeUpdateFirmwareSuccess)));
        }

        if ((CapsuleHeader->Flags & PcdGet16(PcdSystemRebootAfterCapsuleProcessFlag)) != 0 ||
            IsFmpCapsule(CapsuleHeader)) {
          mNeedReset = TRUE;
        }
      }
    }
  }

  Status = gBS->LocateProtocol(&gEsrtManagementProtocolGuid, NULL, (VOID **)&EsrtManagement);
  //
  // Always sync ESRT Cache from FMP Instance
  //
  if (!EFI_ERROR(Status)) {
    EsrtManagement->SyncEsrtFmp();
  }
  Status = EFI_SUCCESS;

Done:
  //
  // Free the allocated temp memory space.
  //
  if (CapsuleGuidCache != NULL) {
    FreePool(CapsuleGuidCache);
  }
  if (CapsulePtrCache != NULL) {
    FreePool(CapsulePtrCache);
  }
  if (CapsulePtr != NULL) {
    FreePool(CapsulePtr);
  }

  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, (EFI_SOFTWARE | PcdGet32(PcdStatusCodeSubClassCapsule) | PcdGet32(PcdCapsuleStatusCodeProcessCapsulesEnd)));

  return Status;
}

/**
  Do reset system.
**/
VOID
DoResetSystem (
  VOID
  )
{
  UINTN                         Index;

  REPORT_STATUS_CODE(EFI_PROGRESS_CODE, (EFI_SOFTWARE | PcdGet32(PcdStatusCodeSubClassCapsule) | PcdGet32(PcdCapsuleStatusCodeResettingSystem)));

  Print(L"Capsule Request Cold Reboot.\n");
  DEBUG((DEBUG_INFO, "Capsule Request Cold Reboot."));

  for (Index = 5; Index > 0; Index--) {
    Print(L"\rResetting system in %d seconds ...", Index);
    DEBUG((DEBUG_INFO, "\rResetting system in %d seconds ...", Index));
    gBS->Stall(1000000);
  }

  gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

  CpuDeadLoop();
}

/**

  This routine is called to process capsules.

  Caution: This function may receive untrusted input.

  The capsules reported in EFI_HOB_UEFI_CAPSULE are processed.
  If there is no EFI_HOB_UEFI_CAPSULE, this routine does nothing.

  This routine should be called twice in BDS.
  1) The first call must be before EndOfDxe. The system capsules is processed.
     If device capsule FMP protocols are exposted at this time and device FMP
     capsule has zero EmbeddedDriverCount, the device capsules are processed.
     Each individual capsule result is recorded in capsule record variable.
     System may reset in this function, if reset is required by capsule and
     all capsules are processed.
     If not all capsules are processed, reset will be defered to second call.

  2) The second call must be after EndOfDxe and after ConnectAll, so that all
     device capsule FMP protocols are exposed.
     The system capsules are skipped. If the device capsules are NOT processed
     in first call, they are processed here.
     Each individual capsule result is recorded in capsule record variable.
     System may reset in this function, if reset is required by capsule
     processed in first call and second call.

  @retval EFI_SUCCESS             There is no error when processing capsules.
  @retval EFI_OUT_OF_RESOURCES    No enough resource to process capsules.

**/
EFI_STATUS
EFIAPI
ProcessCapsules (
  VOID
  )
{
  EFI_STATUS                    Status;

  if (!mDxeCapsuleLibEndOfDxe) {
    //
    // Initialize mAreAllImagesProcessed to be TRUE.
    //
    // It will be updated to FALSE in ProcessTheseCapsules()->ProcessCapsuleImage(),
    // if there is any FMP image in any FMP capsule not processed.
    //
    mAreAllImagesProcessed = TRUE;

    Status = ProcessTheseCapsules(TRUE);
    //
    // Reboot System if and only if all capsule processed.
    // If not, defer reset to 2nd process.
    //
    if (mNeedReset && mAreAllImagesProcessed) {
      DoResetSystem();
    }
  } else {
    Status = ProcessTheseCapsules(FALSE);
    //
    // Reboot System if required after all capsule processed
    //
    if (mNeedReset) {
      DoResetSystem();
    }
  }
  return Status;
}
