/** @file
  The common variable operation routines shared by DXE_RUNTIME variable
  module and DXE_SMM variable module.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable data. They may be input in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  VariableServiceGetNextVariableName () and VariableServiceQueryVariableInfo() are external API.
  They need check input parameter.

  VariableServiceGetVariable() and VariableServiceSetVariable() are external API
  to receive datasize and data buffer. The size should be checked carefully.

  VariableServiceSetVariable() should also check authenticate data to avoid buffer overflow,
  integer overflow. It should also check attribute to avoid authentication bypass.

Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Variable.h"
#include "AuthService.h"

VARIABLE_MODULE_GLOBAL  *mVariableModuleGlobal;

///
/// Define a memory cache that improves the search performance for a variable.
///
VARIABLE_STORE_HEADER  *mNvVariableCache      = NULL;

///
/// The memory entry used for variable statistics data.
///
VARIABLE_INFO_ENTRY    *gVariableInfo         = NULL;

///
/// The list to store the variables which cannot be set after the EFI_END_OF_DXE_EVENT_GROUP_GUID
/// or EVT_GROUP_READY_TO_BOOT event.
///
LIST_ENTRY             mLockedVariableList    = INITIALIZE_LIST_HEAD_VARIABLE (mLockedVariableList);

///
/// The flag to indicate whether the platform has left the DXE phase of execution.
///
BOOLEAN                mEndOfDxe              = FALSE;

///
/// The flag to indicate whether the variable storage locking is enabled.
///
BOOLEAN                mEnableLocking         = TRUE;

//
// To prevent name collisions with possible future globally defined variables,
// other internal firmware data variables that are not defined here must be
// saved with a unique VendorGuid other than EFI_GLOBAL_VARIABLE or
// any other GUID defined by the UEFI Specification. Implementations must
// only permit the creation of variables with a UEFI Specification-defined
// VendorGuid when these variables are documented in the UEFI Specification.
//
GLOBAL_VARIABLE_ENTRY mGlobalVariableList[] = {
  {EFI_LANG_CODES_VARIABLE_NAME,             VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_LANG_VARIABLE_NAME,                   VARIABLE_ATTRIBUTE_NV_BS_RT},
  {EFI_TIME_OUT_VARIABLE_NAME,               VARIABLE_ATTRIBUTE_NV_BS_RT},
  {EFI_PLATFORM_LANG_CODES_VARIABLE_NAME,    VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_PLATFORM_LANG_VARIABLE_NAME,          VARIABLE_ATTRIBUTE_NV_BS_RT},
  {EFI_CON_IN_VARIABLE_NAME,                 VARIABLE_ATTRIBUTE_NV_BS_RT},
  {EFI_CON_OUT_VARIABLE_NAME,                VARIABLE_ATTRIBUTE_NV_BS_RT},
  {EFI_ERR_OUT_VARIABLE_NAME,                VARIABLE_ATTRIBUTE_NV_BS_RT},
  {EFI_CON_IN_DEV_VARIABLE_NAME,             VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_CON_OUT_DEV_VARIABLE_NAME,            VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_ERR_OUT_DEV_VARIABLE_NAME,            VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_BOOT_ORDER_VARIABLE_NAME,             VARIABLE_ATTRIBUTE_NV_BS_RT},
  {EFI_BOOT_NEXT_VARIABLE_NAME,              VARIABLE_ATTRIBUTE_NV_BS_RT},
  {EFI_BOOT_CURRENT_VARIABLE_NAME,           VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_BOOT_OPTION_SUPPORT_VARIABLE_NAME,    VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_DRIVER_ORDER_VARIABLE_NAME,           VARIABLE_ATTRIBUTE_NV_BS_RT},
  {EFI_HW_ERR_REC_SUPPORT_VARIABLE_NAME,     VARIABLE_ATTRIBUTE_NV_BS_RT},
  {EFI_SETUP_MODE_NAME,                      VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_KEY_EXCHANGE_KEY_NAME,                VARIABLE_ATTRIBUTE_NV_BS_RT_AT},
  {EFI_PLATFORM_KEY_NAME,                    VARIABLE_ATTRIBUTE_NV_BS_RT_AT},
  {EFI_SIGNATURE_SUPPORT_NAME,               VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_SECURE_BOOT_MODE_NAME,                VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_KEK_DEFAULT_VARIABLE_NAME,            VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_PK_DEFAULT_VARIABLE_NAME,             VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_DB_DEFAULT_VARIABLE_NAME,             VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_DBX_DEFAULT_VARIABLE_NAME,            VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_DBT_DEFAULT_VARIABLE_NAME,            VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_OS_INDICATIONS_SUPPORT_VARIABLE_NAME, VARIABLE_ATTRIBUTE_BS_RT},
  {EFI_OS_INDICATIONS_VARIABLE_NAME,         VARIABLE_ATTRIBUTE_NV_BS_RT},
  {EFI_VENDOR_KEYS_VARIABLE_NAME,            VARIABLE_ATTRIBUTE_BS_RT},
};
GLOBAL_VARIABLE_ENTRY mGlobalVariableList2[] = {
  {L"Boot####",                              VARIABLE_ATTRIBUTE_NV_BS_RT},
  {L"Driver####",                            VARIABLE_ATTRIBUTE_NV_BS_RT},
  {L"Key####",                               VARIABLE_ATTRIBUTE_NV_BS_RT},
};

/**

  SecureBoot Hook for auth variable update.

  @param[in] VariableName                 Name of Variable to be found.
  @param[in] VendorGuid                   Variable vendor GUID.
**/
VOID
EFIAPI
SecureBootHook (
  IN CHAR16                                 *VariableName,
  IN EFI_GUID                               *VendorGuid
  );

/**
  Routine used to track statistical information about variable usage.
  The data is stored in the EFI system table so it can be accessed later.
  VariableInfo.efi can dump out the table. Only Boot Services variable
  accesses are tracked by this code. The PcdVariableCollectStatistics
  build flag controls if this feature is enabled.

  A read that hits in the cache will have Read and Cache true for
  the transaction. Data is allocated by this routine, but never
  freed.

  @param[in] VariableName   Name of the Variable to track.
  @param[in] VendorGuid     Guid of the Variable to track.
  @param[in] Volatile       TRUE if volatile FALSE if non-volatile.
  @param[in] Read           TRUE if GetVariable() was called.
  @param[in] Write          TRUE if SetVariable() was called.
  @param[in] Delete         TRUE if deleted via SetVariable().
  @param[in] Cache          TRUE for a cache hit.

**/
VOID
UpdateVariableInfo (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  IN  BOOLEAN                 Volatile,
  IN  BOOLEAN                 Read,
  IN  BOOLEAN                 Write,
  IN  BOOLEAN                 Delete,
  IN  BOOLEAN                 Cache
  )
{
  VARIABLE_INFO_ENTRY   *Entry;

  if (FeaturePcdGet (PcdVariableCollectStatistics)) {

    if (AtRuntime ()) {
      // Don't collect statistics at runtime.
      return;
    }

    if (gVariableInfo == NULL) {
      //
      // On the first call allocate a entry and place a pointer to it in
      // the EFI System Table.
      //
      gVariableInfo = AllocateZeroPool (sizeof (VARIABLE_INFO_ENTRY));
      ASSERT (gVariableInfo != NULL);

      CopyGuid (&gVariableInfo->VendorGuid, VendorGuid);
      gVariableInfo->Name = AllocatePool (StrSize (VariableName));
      ASSERT (gVariableInfo->Name != NULL);
      StrCpy (gVariableInfo->Name, VariableName);
      gVariableInfo->Volatile = Volatile;
    }


    for (Entry = gVariableInfo; Entry != NULL; Entry = Entry->Next) {
      if (CompareGuid (VendorGuid, &Entry->VendorGuid)) {
        if (StrCmp (VariableName, Entry->Name) == 0) {
          if (Read) {
            Entry->ReadCount++;
          }
          if (Write) {
            Entry->WriteCount++;
          }
          if (Delete) {
            Entry->DeleteCount++;
          }
          if (Cache) {
            Entry->CacheCount++;
          }

          return;
        }
      }

      if (Entry->Next == NULL) {
        //
        // If the entry is not in the table add it.
        // Next iteration of the loop will fill in the data.
        //
        Entry->Next = AllocateZeroPool (sizeof (VARIABLE_INFO_ENTRY));
        ASSERT (Entry->Next != NULL);

        CopyGuid (&Entry->Next->VendorGuid, VendorGuid);
        Entry->Next->Name = AllocatePool (StrSize (VariableName));
        ASSERT (Entry->Next->Name != NULL);
        StrCpy (Entry->Next->Name, VariableName);
        Entry->Next->Volatile = Volatile;
      }

    }
  }
}


/**

  This code checks if variable header is valid or not.

  @param Variable        Pointer to the Variable Header.

  @retval TRUE           Variable header is valid.
  @retval FALSE          Variable header is not valid.

**/
BOOLEAN
IsValidVariableHeader (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if (Variable == NULL || Variable->StartId != VARIABLE_DATA) {
    return FALSE;
  }

  return TRUE;
}


/**

  This function writes data to the FWH at the correct LBA even if the LBAs
  are fragmented.

  @param Global                  Pointer to VARAIBLE_GLOBAL structure.
  @param Volatile                Point out the Variable is Volatile or Non-Volatile.
  @param SetByIndex              TRUE if target pointer is given as index.
                                 FALSE if target pointer is absolute.
  @param Fvb                     Pointer to the writable FVB protocol.
  @param DataPtrIndex            Pointer to the Data from the end of VARIABLE_STORE_HEADER
                                 structure.
  @param DataSize                Size of data to be written.
  @param Buffer                  Pointer to the buffer from which data is written.

  @retval EFI_INVALID_PARAMETER  Parameters not valid.
  @retval EFI_SUCCESS            Variable store successfully updated.

**/
EFI_STATUS
UpdateVariableStore (
  IN  VARIABLE_GLOBAL                     *Global,
  IN  BOOLEAN                             Volatile,
  IN  BOOLEAN                             SetByIndex,
  IN  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb,
  IN  UINTN                               DataPtrIndex,
  IN  UINT32                              DataSize,
  IN  UINT8                               *Buffer
  )
{
  EFI_FV_BLOCK_MAP_ENTRY      *PtrBlockMapEntry;
  UINTN                       BlockIndex2;
  UINTN                       LinearOffset;
  UINTN                       CurrWriteSize;
  UINTN                       CurrWritePtr;
  UINT8                       *CurrBuffer;
  EFI_LBA                     LbaNumber;
  UINTN                       Size;
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  VARIABLE_STORE_HEADER       *VolatileBase;
  EFI_PHYSICAL_ADDRESS        FvVolHdr;
  EFI_PHYSICAL_ADDRESS        DataPtr;
  EFI_STATUS                  Status;

  FwVolHeader = NULL;
  DataPtr     = DataPtrIndex;

  //
  // Check if the Data is Volatile.
  //
  if (!Volatile) {
    if (Fvb == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    Status = Fvb->GetPhysicalAddress(Fvb, &FvVolHdr);
    ASSERT_EFI_ERROR (Status);

    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) FvVolHdr);
    //
    // Data Pointer should point to the actual Address where data is to be
    // written.
    //
    if (SetByIndex) {
      DataPtr += mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase;
    }

    if ((DataPtr + DataSize) >= ((EFI_PHYSICAL_ADDRESS) (UINTN) ((UINT8 *) FwVolHeader + FwVolHeader->FvLength))) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // Data Pointer should point to the actual Address where data is to be
    // written.
    //
    VolatileBase = (VARIABLE_STORE_HEADER *) ((UINTN) mVariableModuleGlobal->VariableGlobal.VolatileVariableBase);
    if (SetByIndex) {
      DataPtr += mVariableModuleGlobal->VariableGlobal.VolatileVariableBase;
    }

    if ((DataPtr + DataSize) >= ((UINTN) ((UINT8 *) VolatileBase + VolatileBase->Size))) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // If Volatile Variable just do a simple mem copy.
    //
    CopyMem ((UINT8 *)(UINTN)DataPtr, Buffer, DataSize);
    return EFI_SUCCESS;
  }

  //
  // If we are here we are dealing with Non-Volatile Variables.
  //
  LinearOffset  = (UINTN) FwVolHeader;
  CurrWritePtr  = (UINTN) DataPtr;
  CurrWriteSize = DataSize;
  CurrBuffer    = Buffer;
  LbaNumber     = 0;

  if (CurrWritePtr < LinearOffset) {
    return EFI_INVALID_PARAMETER;
  }

  for (PtrBlockMapEntry = FwVolHeader->BlockMap; PtrBlockMapEntry->NumBlocks != 0; PtrBlockMapEntry++) {
    for (BlockIndex2 = 0; BlockIndex2 < PtrBlockMapEntry->NumBlocks; BlockIndex2++) {
      //
      // Check to see if the Variable Writes are spanning through multiple
      // blocks.
      //
      if ((CurrWritePtr >= LinearOffset) && (CurrWritePtr < LinearOffset + PtrBlockMapEntry->Length)) {
        if ((CurrWritePtr + CurrWriteSize) <= (LinearOffset + PtrBlockMapEntry->Length)) {
          Status = Fvb->Write (
                    Fvb,
                    LbaNumber,
                    (UINTN) (CurrWritePtr - LinearOffset),
                    &CurrWriteSize,
                    CurrBuffer
                    );
          return Status;
        } else {
          Size = (UINT32) (LinearOffset + PtrBlockMapEntry->Length - CurrWritePtr);
          Status = Fvb->Write (
                    Fvb,
                    LbaNumber,
                    (UINTN) (CurrWritePtr - LinearOffset),
                    &Size,
                    CurrBuffer
                    );
          if (EFI_ERROR (Status)) {
            return Status;
          }

          CurrWritePtr  = LinearOffset + PtrBlockMapEntry->Length;
          CurrBuffer    = CurrBuffer + Size;
          CurrWriteSize = CurrWriteSize - Size;
        }
      }

      LinearOffset += PtrBlockMapEntry->Length;
      LbaNumber++;
    }
  }

  return EFI_SUCCESS;
}


/**

  This code gets the current status of Variable Store.

  @param VarStoreHeader  Pointer to the Variable Store Header.

  @retval EfiRaw         Variable store status is raw.
  @retval EfiValid       Variable store status is valid.
  @retval EfiInvalid     Variable store status is invalid.

**/
VARIABLE_STORE_STATUS
GetVariableStoreStatus (
  IN VARIABLE_STORE_HEADER *VarStoreHeader
  )
{
  if (CompareGuid (&VarStoreHeader->Signature, &gEfiAuthenticatedVariableGuid) &&
      VarStoreHeader->Format == VARIABLE_STORE_FORMATTED &&
      VarStoreHeader->State == VARIABLE_STORE_HEALTHY
      ) {

    return EfiValid;
  } else if (((UINT32 *)(&VarStoreHeader->Signature))[0] == 0xffffffff &&
             ((UINT32 *)(&VarStoreHeader->Signature))[1] == 0xffffffff &&
             ((UINT32 *)(&VarStoreHeader->Signature))[2] == 0xffffffff &&
             ((UINT32 *)(&VarStoreHeader->Signature))[3] == 0xffffffff &&
             VarStoreHeader->Size == 0xffffffff &&
             VarStoreHeader->Format == 0xff &&
             VarStoreHeader->State == 0xff
          ) {

    return EfiRaw;
  } else {
    return EfiInvalid;
  }
}


/**

  This code gets the size of name of variable.

  @param Variable        Pointer to the Variable Header.

  @return UINTN          Size of variable in bytes.

**/
UINTN
NameSizeOfVariable (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if (Variable->State    == (UINT8) (-1) ||
      Variable->DataSize == (UINT32) (-1) ||
      Variable->NameSize == (UINT32) (-1) ||
      Variable->Attributes == (UINT32) (-1)) {
    return 0;
  }
  return (UINTN) Variable->NameSize;
}

/**

  This code gets the size of variable data.

  @param Variable        Pointer to the Variable Header.

  @return Size of variable in bytes.

**/
UINTN
DataSizeOfVariable (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if (Variable->State    == (UINT8)  (-1) ||
      Variable->DataSize == (UINT32) (-1) ||
      Variable->NameSize == (UINT32) (-1) ||
      Variable->Attributes == (UINT32) (-1)) {
    return 0;
  }
  return (UINTN) Variable->DataSize;
}

/**

  This code gets the pointer to the variable name.

  @param Variable        Pointer to the Variable Header.

  @return Pointer to Variable Name which is Unicode encoding.

**/
CHAR16 *
GetVariableNamePtr (
  IN  VARIABLE_HEADER   *Variable
  )
{

  return (CHAR16 *) (Variable + 1);
}

/**

  This code gets the pointer to the variable data.

  @param Variable        Pointer to the Variable Header.

  @return Pointer to Variable Data.

**/
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER   *Variable
  )
{
  UINTN Value;

  //
  // Be careful about pad size for alignment.
  //
  Value =  (UINTN) GetVariableNamePtr (Variable);
  Value += NameSizeOfVariable (Variable);
  Value += GET_PAD_SIZE (NameSizeOfVariable (Variable));

  return (UINT8 *) Value;
}


/**

  This code gets the pointer to the next variable header.

  @param Variable        Pointer to the Variable Header.

  @return Pointer to next variable header.

**/
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_HEADER   *Variable
  )
{
  UINTN Value;

  if (!IsValidVariableHeader (Variable)) {
    return NULL;
  }

  Value =  (UINTN) GetVariableDataPtr (Variable);
  Value += DataSizeOfVariable (Variable);
  Value += GET_PAD_SIZE (DataSizeOfVariable (Variable));

  //
  // Be careful about pad size for alignment.
  //
  return (VARIABLE_HEADER *) HEADER_ALIGN (Value);
}

/**

  Gets the pointer to the first variable header in given variable store area.

  @param VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the first variable header.

**/
VARIABLE_HEADER *
GetStartPointer (
  IN VARIABLE_STORE_HEADER       *VarStoreHeader
  )
{
  //
  // The end of variable store.
  //
  return (VARIABLE_HEADER *) HEADER_ALIGN (VarStoreHeader + 1);
}

/**

  Gets the pointer to the end of the variable storage area.

  This function gets pointer to the end of the variable storage
  area, according to the input variable store header.

  @param VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the end of the variable storage area.

**/
VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER       *VarStoreHeader
  )
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *) HEADER_ALIGN ((UINTN) VarStoreHeader + VarStoreHeader->Size);
}

/**

  Check the PubKeyIndex is a valid key or not.

  This function will iterate the NV storage to see if this PubKeyIndex is still referenced 
  by any valid count-based auth variabe.
  
  @param[in]  PubKeyIndex     Index of the public key in public key store.

  @retval     TRUE            The PubKeyIndex is still in use.
  @retval     FALSE           The PubKeyIndex is not referenced by any count-based auth variabe.
  
**/
BOOLEAN
IsValidPubKeyIndex (
  IN   UINT32      PubKeyIndex
  )
{
  VARIABLE_HEADER          *Variable;

  if (PubKeyIndex > mPubKeyNumber) {
    return FALSE;
  }
  
  Variable = GetStartPointer (mNvVariableCache);
  
  while (IsValidVariableHeader (Variable)) {
    if ((Variable->State == VAR_ADDED || Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) && 
        Variable->PubKeyIndex == PubKeyIndex) {
      return TRUE;
    }
    Variable = GetNextVariablePtr (Variable);
  }
  
  return FALSE;
}

/**

  Get the number of valid public key in PubKeyStore.
  
  @param[in]  PubKeyNumber     Number of the public key in public key store.

  @return     Number of valid public key in PubKeyStore.

**/
UINT32
GetValidPubKeyNumber (
  IN   UINT32       PubKeyNumber
  )
{
  UINT32       PubKeyIndex;
  UINT32       Counter;

  Counter = 0;
  
  for (PubKeyIndex = 1; PubKeyIndex <= PubKeyNumber; PubKeyIndex++) {
    if (IsValidPubKeyIndex (PubKeyIndex)) {
      Counter++;
    }
  }
  
  return Counter;
}

/**

  Filter the useless key in public key store.

  This function will find out all valid public keys in public key database, save them in new allocated 
  buffer NewPubKeyStore, and give the new PubKeyIndex. The caller is responsible for freeing buffer
  NewPubKeyIndex and NewPubKeyStore with FreePool().

  @param[in]   PubKeyStore          Point to the public key database.
  @param[in]   PubKeyNumber         Number of the public key in PubKeyStore.
  @param[out]  NewPubKeyIndex       Point to an array of new PubKeyIndex corresponds to NewPubKeyStore.
  @param[out]  NewPubKeyStore       Saved all valid public keys in PubKeyStore.
  @param[out]  NewPubKeySize        Buffer size of the NewPubKeyStore.
  
  @retval  EFI_SUCCESS              Trim operation is complete successfully.
  @retval  EFI_OUT_OF_RESOURCES     No enough memory resources, or no useless key in PubKeyStore.
  
**/
EFI_STATUS
PubKeyStoreFilter (
  IN   UINT8         *PubKeyStore,
  IN   UINT32        PubKeyNumber,
  OUT  UINT32        **NewPubKeyIndex,
  OUT  UINT8         **NewPubKeyStore,
  OUT  UINT32        *NewPubKeySize
  )
{
  UINT32        PubKeyIndex;
  UINT32        CopiedKey;
  UINT32        NewPubKeyNumber;
  
  NewPubKeyNumber = GetValidPubKeyNumber (PubKeyNumber);
  if (NewPubKeyNumber == PubKeyNumber) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (NewPubKeyNumber != 0) {
    *NewPubKeySize = NewPubKeyNumber * EFI_CERT_TYPE_RSA2048_SIZE;
  } else {
    *NewPubKeySize = sizeof (UINT8);
  }

  *NewPubKeyStore = AllocatePool (*NewPubKeySize);
  if (*NewPubKeyStore == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *NewPubKeyIndex = AllocateZeroPool ((PubKeyNumber + 1) * sizeof (UINT32));
  if (*NewPubKeyIndex == NULL) {
    FreePool (*NewPubKeyStore);
    return EFI_OUT_OF_RESOURCES;
  }

  CopiedKey = 0;
  for (PubKeyIndex = 1; PubKeyIndex <= PubKeyNumber; PubKeyIndex++) {
    if (IsValidPubKeyIndex (PubKeyIndex)) {
      CopyMem (
        *NewPubKeyStore + CopiedKey * EFI_CERT_TYPE_RSA2048_SIZE,
        PubKeyStore + (PubKeyIndex - 1) * EFI_CERT_TYPE_RSA2048_SIZE,
        EFI_CERT_TYPE_RSA2048_SIZE
        );
      (*NewPubKeyIndex)[PubKeyIndex] = ++CopiedKey;
    }
  }
  return EFI_SUCCESS;
}

/**

  Variable store garbage collection and reclaim operation.

  If ReclaimPubKeyStore is FALSE, reclaim variable space by deleting the obsoleted varaibles.
  If ReclaimPubKeyStore is TRUE, reclaim invalid key in public key database and update the PubKeyIndex
  for all the count-based authenticate variable in NV storage.

  @param[in]      VariableBase            Base address of variable store.
  @param[out]     LastVariableOffset      Offset of last variable.
  @param[in]      IsVolatile              The variable store is volatile or not;
                                          if it is non-volatile, need FTW.
  @param[in, out] UpdatingPtrTrack        Pointer to updating variable pointer track structure.
  @param[in]      ReclaimPubKeyStore      Reclaim for public key database or not.
  @param[in]      ReclaimAnyway           If TRUE, do reclaim anyway.
  
  @return EFI_SUCCESS                  Reclaim operation has finished successfully.
  @return EFI_OUT_OF_RESOURCES         No enough memory resources.
  @return EFI_DEVICE_ERROR             The public key database doesn't exist.
  @return Others                       Unexpect error happened during reclaim operation.

**/
EFI_STATUS
Reclaim (
  IN     EFI_PHYSICAL_ADDRESS         VariableBase,
  OUT    UINTN                        *LastVariableOffset,
  IN     BOOLEAN                      IsVolatile,
  IN OUT VARIABLE_POINTER_TRACK       *UpdatingPtrTrack,
  IN     BOOLEAN                      ReclaimPubKeyStore,
  IN     BOOLEAN                      ReclaimAnyway
  )
{
  VARIABLE_HEADER       *Variable;
  VARIABLE_HEADER       *AddedVariable;
  VARIABLE_HEADER       *NextVariable;
  VARIABLE_HEADER       *NextAddedVariable;
  VARIABLE_STORE_HEADER *VariableStoreHeader;
  UINT8                 *ValidBuffer;
  UINTN                 MaximumBufferSize;
  UINTN                 VariableSize;
  UINTN                 VariableNameSize;
  UINTN                 UpdatingVariableNameSize;
  UINTN                 NameSize;
  UINT8                 *CurrPtr;
  VOID                  *Point0;
  VOID                  *Point1;
  BOOLEAN               FoundAdded;
  EFI_STATUS            Status;
  CHAR16                *VariableNamePtr;
  CHAR16                *UpdatingVariableNamePtr;
  UINTN                 CommonVariableTotalSize;
  UINTN                 HwErrVariableTotalSize;
  UINT32                *NewPubKeyIndex;
  UINT8                 *NewPubKeyStore;
  UINT32                NewPubKeySize;
  VARIABLE_HEADER       *PubKeyHeader;
  BOOLEAN               NeedDoReclaim;
  VARIABLE_HEADER       *UpdatingVariable;

  UpdatingVariable = NULL;
  if (UpdatingPtrTrack != NULL) {
    UpdatingVariable = UpdatingPtrTrack->CurrPtr;
  }

  NeedDoReclaim = FALSE;
  VariableStoreHeader = (VARIABLE_STORE_HEADER *) ((UINTN) VariableBase);

  CommonVariableTotalSize = 0;
  HwErrVariableTotalSize  = 0;
  NewPubKeyIndex = NULL;
  NewPubKeyStore = NULL;
  NewPubKeySize  = 0;
  PubKeyHeader   = NULL;
  
  //
  // Start Pointers for the variable.
  //
  Variable          = GetStartPointer (VariableStoreHeader);
  MaximumBufferSize = sizeof (VARIABLE_STORE_HEADER);

  while (IsValidVariableHeader (Variable)) {
    NextVariable = GetNextVariablePtr (Variable);
    if (Variable->State == VAR_ADDED ||
        Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)
       ) {
      VariableSize = (UINTN) NextVariable - (UINTN) Variable;
      MaximumBufferSize += VariableSize;
    } else {
      NeedDoReclaim = TRUE;
    }

    Variable = NextVariable;
  }

  if (!ReclaimAnyway && !NeedDoReclaim) {
    DEBUG ((EFI_D_INFO, "Variable driver: no DELETED variable found, so no variable space could be reclaimed.\n"));
    return EFI_SUCCESS;
  }

  //
  // Reserve the 1 Bytes with Oxff to identify the
  // end of the variable buffer.
  //
  MaximumBufferSize += 1;
  ValidBuffer = AllocatePool (MaximumBufferSize);
  if (ValidBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (ValidBuffer, MaximumBufferSize, 0xff);

  //
  // Copy variable store header.
  //
  CopyMem (ValidBuffer, VariableStoreHeader, sizeof (VARIABLE_STORE_HEADER));
  CurrPtr = (UINT8 *) GetStartPointer ((VARIABLE_STORE_HEADER *) ValidBuffer);

  if (ReclaimPubKeyStore) {
    //
    // Trim the PubKeyStore and get new PubKeyIndex.
    //
    Status = PubKeyStoreFilter (
               mPubKeyStore,
               mPubKeyNumber,
               &NewPubKeyIndex,
               &NewPubKeyStore,
               &NewPubKeySize
               );
    if (EFI_ERROR (Status)) {
      FreePool (ValidBuffer);
      return Status;
    }

    //
    // Refresh the PubKeyIndex for all valid variables (ADDED and IN_DELETED_TRANSITION).
    //
    Variable = GetStartPointer (mNvVariableCache);
    while (IsValidVariableHeader (Variable)) {
      NextVariable = GetNextVariablePtr (Variable);
      if (Variable->State == VAR_ADDED || Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
        if ((StrCmp (GetVariableNamePtr (Variable), AUTHVAR_KEYDB_NAME) == 0) && 
            (CompareGuid (&Variable->VendorGuid, &gEfiAuthenticatedVariableGuid))) {
          //
          // Skip the public key database, it will be reinstalled later.
          //
          PubKeyHeader = Variable;
          Variable = NextVariable;
          continue;
        }
        
        VariableSize = (UINTN) NextVariable - (UINTN) Variable;
        CopyMem (CurrPtr, (UINT8 *) Variable, VariableSize);
        ((VARIABLE_HEADER*) CurrPtr)->PubKeyIndex = NewPubKeyIndex[Variable->PubKeyIndex];
        CurrPtr += VariableSize;
        if ((!IsVolatile) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
          HwErrVariableTotalSize += VariableSize;
        } else if ((!IsVolatile) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
          CommonVariableTotalSize += VariableSize;
        }
      }
      Variable = NextVariable;
    }

    //
    // Reinstall the new public key database.
    //
    ASSERT (PubKeyHeader != NULL);
    if (PubKeyHeader == NULL) {
      FreePool (ValidBuffer);
      FreePool (NewPubKeyIndex);
      FreePool (NewPubKeyStore);
      return EFI_DEVICE_ERROR;
    }
    CopyMem (CurrPtr, (UINT8*) PubKeyHeader, sizeof (VARIABLE_HEADER));
    Variable = (VARIABLE_HEADER*) CurrPtr;
    Variable->DataSize = NewPubKeySize;
    StrCpy (GetVariableNamePtr (Variable), GetVariableNamePtr (PubKeyHeader));
    CopyMem (GetVariableDataPtr (Variable), NewPubKeyStore, NewPubKeySize);
    CurrPtr = (UINT8*) GetNextVariablePtr (Variable); 
    CommonVariableTotalSize += (UINTN) CurrPtr - (UINTN) Variable;
  } else {
    //
    // Reinstall all ADDED variables as long as they are not identical to Updating Variable.
    //
    Variable = GetStartPointer (VariableStoreHeader);
    while (IsValidVariableHeader (Variable)) {
      NextVariable = GetNextVariablePtr (Variable);
      if (Variable->State == VAR_ADDED) {
        if (UpdatingVariable != NULL) {
          if (UpdatingVariable == Variable) {
            Variable = NextVariable;
            continue;
          }

          VariableNameSize         = NameSizeOfVariable(Variable);
          UpdatingVariableNameSize = NameSizeOfVariable(UpdatingVariable);

          VariableNamePtr         = GetVariableNamePtr (Variable);
          UpdatingVariableNamePtr = GetVariableNamePtr (UpdatingVariable);
          if (CompareGuid (&Variable->VendorGuid, &UpdatingVariable->VendorGuid)    &&
              VariableNameSize == UpdatingVariableNameSize &&
              CompareMem (VariableNamePtr, UpdatingVariableNamePtr, VariableNameSize) == 0 ) {
            Variable = NextVariable;
            continue;
          }
        }
        VariableSize = (UINTN) NextVariable - (UINTN) Variable;
        CopyMem (CurrPtr, (UINT8 *) Variable, VariableSize);
        CurrPtr += VariableSize;
        if ((!IsVolatile) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
          HwErrVariableTotalSize += VariableSize;
        } else if ((!IsVolatile) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
          CommonVariableTotalSize += VariableSize;
        }
      }
      Variable = NextVariable;
    }

    //
    // Reinstall the variable being updated if it is not NULL.
    //
    if (UpdatingVariable != NULL) {
      VariableSize = (UINTN)(GetNextVariablePtr (UpdatingVariable)) - (UINTN)UpdatingVariable;
      CopyMem (CurrPtr, (UINT8 *) UpdatingVariable, VariableSize);
      UpdatingPtrTrack->CurrPtr = (VARIABLE_HEADER *)((UINTN)UpdatingPtrTrack->StartPtr + ((UINTN)CurrPtr - (UINTN)GetStartPointer ((VARIABLE_STORE_HEADER *) ValidBuffer)));
      UpdatingPtrTrack->InDeletedTransitionPtr = NULL;
      CurrPtr += VariableSize;
      if ((!IsVolatile) && ((UpdatingVariable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
          HwErrVariableTotalSize += VariableSize;
      } else if ((!IsVolatile) && ((UpdatingVariable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
          CommonVariableTotalSize += VariableSize;
      }
    }

    //
    // Reinstall all in delete transition variables.
    //
    Variable      = GetStartPointer (VariableStoreHeader);
    while (IsValidVariableHeader (Variable)) {
      NextVariable = GetNextVariablePtr (Variable);
      if (Variable != UpdatingVariable && Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {

        //
        // Buffer has cached all ADDED variable.
        // Per IN_DELETED variable, we have to guarantee that
        // no ADDED one in previous buffer.
        //

        FoundAdded = FALSE;
        AddedVariable = GetStartPointer ((VARIABLE_STORE_HEADER *) ValidBuffer);
        while (IsValidVariableHeader (AddedVariable)) {
          NextAddedVariable = GetNextVariablePtr (AddedVariable);
          NameSize = NameSizeOfVariable (AddedVariable);
          if (CompareGuid (&AddedVariable->VendorGuid, &Variable->VendorGuid) &&
              NameSize == NameSizeOfVariable (Variable)
             ) {
            Point0 = (VOID *) GetVariableNamePtr (AddedVariable);
            Point1 = (VOID *) GetVariableNamePtr (Variable);
            if (CompareMem (Point0, Point1, NameSize) == 0) {
              FoundAdded = TRUE;
              break;
            }
          }
          AddedVariable = NextAddedVariable;
        }
        if (!FoundAdded) {
          //
          // Promote VAR_IN_DELETED_TRANSITION to VAR_ADDED.
          //
          VariableSize = (UINTN) NextVariable - (UINTN) Variable;
          CopyMem (CurrPtr, (UINT8 *) Variable, VariableSize);
          ((VARIABLE_HEADER *) CurrPtr)->State = VAR_ADDED;
          CurrPtr += VariableSize;
          if ((!IsVolatile) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
            HwErrVariableTotalSize += VariableSize;
          } else if ((!IsVolatile) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
            CommonVariableTotalSize += VariableSize;
          }
        }
      }

      Variable = NextVariable;
    }
  }

  if (IsVolatile) {
    //
    // If volatile variable store, just copy valid buffer.
    //
    SetMem ((UINT8 *) (UINTN) VariableBase, VariableStoreHeader->Size, 0xff);
    CopyMem ((UINT8 *) (UINTN) VariableBase, ValidBuffer, (UINTN) (CurrPtr - (UINT8 *) ValidBuffer));
    Status  = EFI_SUCCESS;
  } else {
    //
    // If non-volatile variable store, perform FTW here.
    //
    Status = FtwVariableSpace (
              VariableBase,
              ValidBuffer,
              (UINTN) (CurrPtr - (UINT8 *) ValidBuffer)
              );
    CopyMem (mNvVariableCache, (CHAR8 *)(UINTN)VariableBase, VariableStoreHeader->Size);
  }
  if (!EFI_ERROR (Status)) {
    *LastVariableOffset = (UINTN) (CurrPtr - (UINT8 *) ValidBuffer);
    if (!IsVolatile) {
      mVariableModuleGlobal->HwErrVariableTotalSize = HwErrVariableTotalSize;
      mVariableModuleGlobal->CommonVariableTotalSize = CommonVariableTotalSize;
    }
  } else {
    NextVariable  = GetStartPointer ((VARIABLE_STORE_HEADER *)(UINTN)VariableBase);
    while (IsValidVariableHeader (NextVariable)) {
      VariableSize = NextVariable->NameSize + NextVariable->DataSize + sizeof (VARIABLE_HEADER);
      if ((!IsVolatile) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
        mVariableModuleGlobal->HwErrVariableTotalSize += HEADER_ALIGN (VariableSize);
      } else if ((!IsVolatile) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
        mVariableModuleGlobal->CommonVariableTotalSize += HEADER_ALIGN (VariableSize);
      }

      NextVariable = GetNextVariablePtr (NextVariable);
    }
    *LastVariableOffset = (UINTN) NextVariable - (UINTN) VariableBase;
  }

  if (NewPubKeyStore != NULL) {
    FreePool (NewPubKeyStore);
  }

  if (NewPubKeyIndex != NULL) {
    FreePool (NewPubKeyIndex);
  }
  
  FreePool (ValidBuffer);

  return Status;
}

/**
  Find the variable in the specified variable store.

  @param[in]       VariableName        Name of the variable to be found
  @param[in]       VendorGuid          Vendor GUID to be found.
  @param[in]       IgnoreRtCheck       Ignore EFI_VARIABLE_RUNTIME_ACCESS attribute
                                       check at runtime when searching variable.
  @param[in, out]  PtrTrack            Variable Track Pointer structure that contains Variable Information.

  @retval          EFI_SUCCESS         Variable found successfully
  @retval          EFI_NOT_FOUND       Variable not found
**/
EFI_STATUS
FindVariableEx (
  IN     CHAR16                  *VariableName,
  IN     EFI_GUID                *VendorGuid,
  IN     BOOLEAN                 IgnoreRtCheck,
  IN OUT VARIABLE_POINTER_TRACK  *PtrTrack
  )
{
  VARIABLE_HEADER                *InDeletedVariable;
  VOID                           *Point;

  PtrTrack->InDeletedTransitionPtr = NULL;

  //
  // Find the variable by walk through HOB, volatile and non-volatile variable store.
  //
  InDeletedVariable  = NULL;

  for ( PtrTrack->CurrPtr = PtrTrack->StartPtr
      ; (PtrTrack->CurrPtr < PtrTrack->EndPtr) && IsValidVariableHeader (PtrTrack->CurrPtr)
      ; PtrTrack->CurrPtr = GetNextVariablePtr (PtrTrack->CurrPtr)
      ) {
    if (PtrTrack->CurrPtr->State == VAR_ADDED ||
        PtrTrack->CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)
       ) {
      if (IgnoreRtCheck || !AtRuntime () || ((PtrTrack->CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) != 0)) {
        if (VariableName[0] == 0) {
          if (PtrTrack->CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
            InDeletedVariable   = PtrTrack->CurrPtr;
          } else {
            PtrTrack->InDeletedTransitionPtr = InDeletedVariable;
            return EFI_SUCCESS;
          }
        } else {
          if (CompareGuid (VendorGuid, &PtrTrack->CurrPtr->VendorGuid)) {
            Point = (VOID *) GetVariableNamePtr (PtrTrack->CurrPtr);

            ASSERT (NameSizeOfVariable (PtrTrack->CurrPtr) != 0);
            if (CompareMem (VariableName, Point, NameSizeOfVariable (PtrTrack->CurrPtr)) == 0) {
              if (PtrTrack->CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
                InDeletedVariable     = PtrTrack->CurrPtr;
              } else {
                PtrTrack->InDeletedTransitionPtr = InDeletedVariable;
                return EFI_SUCCESS;
              }
            }
          }
        }
      }
    }
  }

  PtrTrack->CurrPtr = InDeletedVariable;
  return (PtrTrack->CurrPtr  == NULL) ? EFI_NOT_FOUND : EFI_SUCCESS;
}


/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.
  If IgnoreRtCheck is TRUE, then we ignore the EFI_VARIABLE_RUNTIME_ACCESS attribute check
  at runtime when searching existing variable, only VariableName and VendorGuid are compared.
  Otherwise, variables without EFI_VARIABLE_RUNTIME_ACCESS are not visible at runtime.

  @param[in]   VariableName           Name of the variable to be found.
  @param[in]   VendorGuid             Vendor GUID to be found.
  @param[out]  PtrTrack               VARIABLE_POINTER_TRACK structure for output,
                                      including the range searched and the target position.
  @param[in]   Global                 Pointer to VARIABLE_GLOBAL structure, including
                                      base of volatile variable storage area, base of
                                      NV variable storage area, and a lock.
  @param[in]   IgnoreRtCheck          Ignore EFI_VARIABLE_RUNTIME_ACCESS attribute
                                      check at runtime when searching variable.

  @retval EFI_INVALID_PARAMETER       If VariableName is not an empty string, while
                                      VendorGuid is NULL.
  @retval EFI_SUCCESS                 Variable successfully found.
  @retval EFI_NOT_FOUND               Variable not found

**/
EFI_STATUS
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN  VARIABLE_GLOBAL         *Global,
  IN  BOOLEAN                 IgnoreRtCheck
  )
{
  EFI_STATUS              Status;
  VARIABLE_STORE_HEADER   *VariableStoreHeader[VariableStoreTypeMax];
  VARIABLE_STORE_TYPE     Type;

  if (VariableName[0] != 0 && VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // 0: Volatile, 1: HOB, 2: Non-Volatile.
  // The index and attributes mapping must be kept in this order as RuntimeServiceGetNextVariableName
  // make use of this mapping to implement search algorithm.
  //
  VariableStoreHeader[VariableStoreTypeVolatile] = (VARIABLE_STORE_HEADER *) (UINTN) Global->VolatileVariableBase;
  VariableStoreHeader[VariableStoreTypeHob]      = (VARIABLE_STORE_HEADER *) (UINTN) Global->HobVariableBase;
  VariableStoreHeader[VariableStoreTypeNv]       = mNvVariableCache;

  //
  // Find the variable by walk through HOB, volatile and non-volatile variable store.
  //
  for (Type = (VARIABLE_STORE_TYPE) 0; Type < VariableStoreTypeMax; Type++) {
    if (VariableStoreHeader[Type] == NULL) {
      continue;
    }

    PtrTrack->StartPtr = GetStartPointer (VariableStoreHeader[Type]);
    PtrTrack->EndPtr   = GetEndPointer   (VariableStoreHeader[Type]);
    PtrTrack->Volatile = (BOOLEAN) (Type == VariableStoreTypeVolatile);

    Status = FindVariableEx (VariableName, VendorGuid, IgnoreRtCheck, PtrTrack);
    if (!EFI_ERROR (Status)) {
      return Status;
    }
  }
  return EFI_NOT_FOUND;
}

/**
  Get index from supported language codes according to language string.

  This code is used to get corresponding index in supported language codes. It can handle
  RFC4646 and ISO639 language tags.
  In ISO639 language tags, take 3-characters as a delimitation to find matched string and calculate the index.
  In RFC4646 language tags, take semicolon as a delimitation to find matched string and calculate the index.

  For example:
    SupportedLang  = "engfraengfra"
    Lang           = "eng"
    Iso639Language = TRUE
  The return value is "0".
  Another example:
    SupportedLang  = "en;fr;en-US;fr-FR"
    Lang           = "fr-FR"
    Iso639Language = FALSE
  The return value is "3".

  @param  SupportedLang               Platform supported language codes.
  @param  Lang                        Configured language.
  @param  Iso639Language              A bool value to signify if the handler is operated on ISO639 or RFC4646.

  @retval The index of language in the language codes.

**/
UINTN
GetIndexFromSupportedLangCodes(
  IN  CHAR8            *SupportedLang,
  IN  CHAR8            *Lang,
  IN  BOOLEAN          Iso639Language
  )
{
  UINTN    Index;
  UINTN    CompareLength;
  UINTN    LanguageLength;

  if (Iso639Language) {
    CompareLength = ISO_639_2_ENTRY_SIZE;
    for (Index = 0; Index < AsciiStrLen (SupportedLang); Index += CompareLength) {
      if (AsciiStrnCmp (Lang, SupportedLang + Index, CompareLength) == 0) {
        //
        // Successfully find the index of Lang string in SupportedLang string.
        //
        Index = Index / CompareLength;
        return Index;
      }
    }
    ASSERT (FALSE);
    return 0;
  } else {
    //
    // Compare RFC4646 language code
    //
    Index = 0;
    for (LanguageLength = 0; Lang[LanguageLength] != '\0'; LanguageLength++);

    for (Index = 0; *SupportedLang != '\0'; Index++, SupportedLang += CompareLength) {
      //
      // Skip ';' characters in SupportedLang
      //
      for (; *SupportedLang != '\0' && *SupportedLang == ';'; SupportedLang++);
      //
      // Determine the length of the next language code in SupportedLang
      //
      for (CompareLength = 0; SupportedLang[CompareLength] != '\0' && SupportedLang[CompareLength] != ';'; CompareLength++);

      if ((CompareLength == LanguageLength) &&
          (AsciiStrnCmp (Lang, SupportedLang, CompareLength) == 0)) {
        //
        // Successfully find the index of Lang string in SupportedLang string.
        //
        return Index;
      }
    }
    ASSERT (FALSE);
    return 0;
  }
}

/**
  Get language string from supported language codes according to index.

  This code is used to get corresponding language strings in supported language codes. It can handle
  RFC4646 and ISO639 language tags.
  In ISO639 language tags, take 3-characters as a delimitation. Find language string according to the index.
  In RFC4646 language tags, take semicolon as a delimitation. Find language string according to the index.

  For example:
    SupportedLang  = "engfraengfra"
    Index          = "1"
    Iso639Language = TRUE
  The return value is "fra".
  Another example:
    SupportedLang  = "en;fr;en-US;fr-FR"
    Index          = "1"
    Iso639Language = FALSE
  The return value is "fr".

  @param  SupportedLang               Platform supported language codes.
  @param  Index                       The index in supported language codes.
  @param  Iso639Language              A bool value to signify if the handler is operated on ISO639 or RFC4646.

  @retval The language string in the language codes.

**/
CHAR8 *
GetLangFromSupportedLangCodes (
  IN  CHAR8            *SupportedLang,
  IN  UINTN            Index,
  IN  BOOLEAN          Iso639Language
)
{
  UINTN    SubIndex;
  UINTN    CompareLength;
  CHAR8    *Supported;

  SubIndex  = 0;
  Supported = SupportedLang;
  if (Iso639Language) {
    //
    // According to the index of Lang string in SupportedLang string to get the language.
    // This code will be invoked in RUNTIME, therefore there is not a memory allocate/free operation.
    // In driver entry, it pre-allocates a runtime attribute memory to accommodate this string.
    //
    CompareLength = ISO_639_2_ENTRY_SIZE;
    mVariableModuleGlobal->Lang[CompareLength] = '\0';
    return CopyMem (mVariableModuleGlobal->Lang, SupportedLang + Index * CompareLength, CompareLength);

  } else {
    while (TRUE) {
      //
      // Take semicolon as delimitation, sequentially traverse supported language codes.
      //
      for (CompareLength = 0; *Supported != ';' && *Supported != '\0'; CompareLength++) {
        Supported++;
      }
      if ((*Supported == '\0') && (SubIndex != Index)) {
        //
        // Have completed the traverse, but not find corrsponding string.
        // This case is not allowed to happen.
        //
        ASSERT(FALSE);
        return NULL;
      }
      if (SubIndex == Index) {
        //
        // According to the index of Lang string in SupportedLang string to get the language.
        // As this code will be invoked in RUNTIME, therefore there is not memory allocate/free operation.
        // In driver entry, it pre-allocates a runtime attribute memory to accommodate this string.
        //
        mVariableModuleGlobal->PlatformLang[CompareLength] = '\0';
        return CopyMem (mVariableModuleGlobal->PlatformLang, Supported - CompareLength, CompareLength);
      }
      SubIndex++;

      //
      // Skip ';' characters in Supported
      //
      for (; *Supported != '\0' && *Supported == ';'; Supported++);
    }
  }
}

/**
  Returns a pointer to an allocated buffer that contains the best matching language
  from a set of supported languages.

  This function supports both ISO 639-2 and RFC 4646 language codes, but language
  code types may not be mixed in a single call to this function. This function
  supports a variable argument list that allows the caller to pass in a prioritized
  list of language codes to test against all the language codes in SupportedLanguages.

  If SupportedLanguages is NULL, then ASSERT().

  @param[in]  SupportedLanguages  A pointer to a Null-terminated ASCII string that
                                  contains a set of language codes in the format
                                  specified by Iso639Language.
  @param[in]  Iso639Language      If TRUE, then all language codes are assumed to be
                                  in ISO 639-2 format.  If FALSE, then all language
                                  codes are assumed to be in RFC 4646 language format
  @param[in]  ...                 A variable argument list that contains pointers to
                                  Null-terminated ASCII strings that contain one or more
                                  language codes in the format specified by Iso639Language.
                                  The first language code from each of these language
                                  code lists is used to determine if it is an exact or
                                  close match to any of the language codes in
                                  SupportedLanguages.  Close matches only apply to RFC 4646
                                  language codes, and the matching algorithm from RFC 4647
                                  is used to determine if a close match is present.  If
                                  an exact or close match is found, then the matching
                                  language code from SupportedLanguages is returned.  If
                                  no matches are found, then the next variable argument
                                  parameter is evaluated.  The variable argument list
                                  is terminated by a NULL.

  @retval NULL   The best matching language could not be found in SupportedLanguages.
  @retval NULL   There are not enough resources available to return the best matching
                 language.
  @retval Other  A pointer to a Null-terminated ASCII string that is the best matching
                 language in SupportedLanguages.

**/
CHAR8 *
EFIAPI
VariableGetBestLanguage (
  IN CONST CHAR8  *SupportedLanguages,
  IN BOOLEAN      Iso639Language,
  ...
  )
{
  VA_LIST      Args;
  CHAR8        *Language;
  UINTN        CompareLength;
  UINTN        LanguageLength;
  CONST CHAR8  *Supported;
  CHAR8        *Buffer;

  if (SupportedLanguages == NULL) {
    return NULL;
  }

  VA_START (Args, Iso639Language);
  while ((Language = VA_ARG (Args, CHAR8 *)) != NULL) {
    //
    // Default to ISO 639-2 mode
    //
    CompareLength  = 3;
    LanguageLength = MIN (3, AsciiStrLen (Language));

    //
    // If in RFC 4646 mode, then determine the length of the first RFC 4646 language code in Language
    //
    if (!Iso639Language) {
      for (LanguageLength = 0; Language[LanguageLength] != 0 && Language[LanguageLength] != ';'; LanguageLength++);
    }

    //
    // Trim back the length of Language used until it is empty
    //
    while (LanguageLength > 0) {
      //
      // Loop through all language codes in SupportedLanguages
      //
      for (Supported = SupportedLanguages; *Supported != '\0'; Supported += CompareLength) {
        //
        // In RFC 4646 mode, then Loop through all language codes in SupportedLanguages
        //
        if (!Iso639Language) {
          //
          // Skip ';' characters in Supported
          //
          for (; *Supported != '\0' && *Supported == ';'; Supported++);
          //
          // Determine the length of the next language code in Supported
          //
          for (CompareLength = 0; Supported[CompareLength] != 0 && Supported[CompareLength] != ';'; CompareLength++);
          //
          // If Language is longer than the Supported, then skip to the next language
          //
          if (LanguageLength > CompareLength) {
            continue;
          }
        }
        //
        // See if the first LanguageLength characters in Supported match Language
        //
        if (AsciiStrnCmp (Supported, Language, LanguageLength) == 0) {
          VA_END (Args);

          Buffer = Iso639Language ? mVariableModuleGlobal->Lang : mVariableModuleGlobal->PlatformLang;
          Buffer[CompareLength] = '\0';
          return CopyMem (Buffer, Supported, CompareLength);
        }
      }

      if (Iso639Language) {
        //
        // If ISO 639 mode, then each language can only be tested once
        //
        LanguageLength = 0;
      } else {
        //
        // If RFC 4646 mode, then trim Language from the right to the next '-' character
        //
        for (LanguageLength--; LanguageLength > 0 && Language[LanguageLength] != '-'; LanguageLength--);
      }
    }
  }
  VA_END (Args);

  //
  // No matches were found
  //
  return NULL;
}

/**
  Hook the operations in PlatformLangCodes, LangCodes, PlatformLang and Lang.

  When setting Lang/LangCodes, simultaneously update PlatformLang/PlatformLangCodes.

  According to UEFI spec, PlatformLangCodes/LangCodes are only set once in firmware initialization,
  and are read-only. Therefore, in variable driver, only store the original value for other use.

  @param[in] VariableName       Name of variable.

  @param[in] Data               Variable data.

  @param[in] DataSize           Size of data. 0 means delete.

**/
VOID
AutoUpdateLangVariable (
  IN  CHAR16             *VariableName,
  IN  VOID               *Data,
  IN  UINTN              DataSize
  )
{
  EFI_STATUS             Status;
  CHAR8                  *BestPlatformLang;
  CHAR8                  *BestLang;
  UINTN                  Index;
  UINT32                 Attributes;
  VARIABLE_POINTER_TRACK Variable;
  BOOLEAN                SetLanguageCodes;

  //
  // Don't do updates for delete operation
  //
  if (DataSize == 0) {
    return;
  }

  SetLanguageCodes = FALSE;

  if (StrCmp (VariableName, EFI_PLATFORM_LANG_CODES_VARIABLE_NAME) == 0) {
    //
    // PlatformLangCodes is a volatile variable, so it can not be updated at runtime.
    //
    if (AtRuntime ()) {
      return;
    }

    SetLanguageCodes = TRUE;

    //
    // According to UEFI spec, PlatformLangCodes is only set once in firmware initialization, and is read-only
    // Therefore, in variable driver, only store the original value for other use.
    //
    if (mVariableModuleGlobal->PlatformLangCodes != NULL) {
      FreePool (mVariableModuleGlobal->PlatformLangCodes);
    }
    mVariableModuleGlobal->PlatformLangCodes = AllocateRuntimeCopyPool (DataSize, Data);
    ASSERT (mVariableModuleGlobal->PlatformLangCodes != NULL);

    //
    // PlatformLang holds a single language from PlatformLangCodes,
    // so the size of PlatformLangCodes is enough for the PlatformLang.
    //
    if (mVariableModuleGlobal->PlatformLang != NULL) {
      FreePool (mVariableModuleGlobal->PlatformLang);
    }
    mVariableModuleGlobal->PlatformLang = AllocateRuntimePool (DataSize);
    ASSERT (mVariableModuleGlobal->PlatformLang != NULL);

  } else if (StrCmp (VariableName, EFI_LANG_CODES_VARIABLE_NAME) == 0) {
    //
    // LangCodes is a volatile variable, so it can not be updated at runtime.
    //
    if (AtRuntime ()) {
      return;
    }

    SetLanguageCodes = TRUE;

    //
    // According to UEFI spec, LangCodes is only set once in firmware initialization, and is read-only
    // Therefore, in variable driver, only store the original value for other use.
    //
    if (mVariableModuleGlobal->LangCodes != NULL) {
      FreePool (mVariableModuleGlobal->LangCodes);
    }
    mVariableModuleGlobal->LangCodes = AllocateRuntimeCopyPool (DataSize, Data);
    ASSERT (mVariableModuleGlobal->LangCodes != NULL);
  }

  if (SetLanguageCodes
      && (mVariableModuleGlobal->PlatformLangCodes != NULL)
      && (mVariableModuleGlobal->LangCodes != NULL)) {
    //
    // Update Lang if PlatformLang is already set
    // Update PlatformLang if Lang is already set
    //
    Status = FindVariable (EFI_PLATFORM_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
    if (!EFI_ERROR (Status)) {
      //
      // Update Lang
      //
      VariableName = EFI_PLATFORM_LANG_VARIABLE_NAME;
      Data         = GetVariableDataPtr (Variable.CurrPtr);
      DataSize     = Variable.CurrPtr->DataSize;
    } else {
      Status = FindVariable (EFI_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
      if (!EFI_ERROR (Status)) {
        //
        // Update PlatformLang
        //
        VariableName = EFI_LANG_VARIABLE_NAME;
        Data         = GetVariableDataPtr (Variable.CurrPtr);
        DataSize     = Variable.CurrPtr->DataSize;
      } else {
        //
        // Neither PlatformLang nor Lang is set, directly return
        //
        return;
      }
    }
  }

  //
  // According to UEFI spec, "Lang" and "PlatformLang" is NV|BS|RT attributions.
  //
  Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;

  if (StrCmp (VariableName, EFI_PLATFORM_LANG_VARIABLE_NAME) == 0) {
    //
    // Update Lang when PlatformLangCodes/LangCodes were set.
    //
    if ((mVariableModuleGlobal->PlatformLangCodes != NULL) && (mVariableModuleGlobal->LangCodes != NULL)) {
      //
      // When setting PlatformLang, firstly get most matched language string from supported language codes.
      //
      BestPlatformLang = VariableGetBestLanguage (mVariableModuleGlobal->PlatformLangCodes, FALSE, Data, NULL);
      if (BestPlatformLang != NULL) {
        //
        // Get the corresponding index in language codes.
        //
        Index = GetIndexFromSupportedLangCodes (mVariableModuleGlobal->PlatformLangCodes, BestPlatformLang, FALSE);

        //
        // Get the corresponding ISO639 language tag according to RFC4646 language tag.
        //
        BestLang = GetLangFromSupportedLangCodes (mVariableModuleGlobal->LangCodes, Index, TRUE);

        //
        // Successfully convert PlatformLang to Lang, and set the BestLang value into Lang variable simultaneously.
        //
        FindVariable (EFI_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);

        Status = UpdateVariable (EFI_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, BestLang,
                                 ISO_639_2_ENTRY_SIZE + 1, Attributes, 0, 0, &Variable, NULL);

        DEBUG ((EFI_D_INFO, "Variable Driver Auto Update PlatformLang, PlatformLang:%a, Lang:%a\n", BestPlatformLang, BestLang));

        ASSERT_EFI_ERROR(Status);
      }
    }

  } else if (StrCmp (VariableName, EFI_LANG_VARIABLE_NAME) == 0) {
    //
    // Update PlatformLang when PlatformLangCodes/LangCodes were set.
    //
    if ((mVariableModuleGlobal->PlatformLangCodes != NULL) && (mVariableModuleGlobal->LangCodes != NULL)) {
      //
      // When setting Lang, firstly get most matched language string from supported language codes.
      //
      BestLang = VariableGetBestLanguage (mVariableModuleGlobal->LangCodes, TRUE, Data, NULL);
      if (BestLang != NULL) {
        //
        // Get the corresponding index in language codes.
        //
        Index = GetIndexFromSupportedLangCodes (mVariableModuleGlobal->LangCodes, BestLang, TRUE);

        //
        // Get the corresponding RFC4646 language tag according to ISO639 language tag.
        //
        BestPlatformLang = GetLangFromSupportedLangCodes (mVariableModuleGlobal->PlatformLangCodes, Index, FALSE);

        //
        // Successfully convert Lang to PlatformLang, and set the BestPlatformLang value into PlatformLang variable simultaneously.
        //
        FindVariable (EFI_PLATFORM_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);

        Status = UpdateVariable (EFI_PLATFORM_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, BestPlatformLang,
                                 AsciiStrSize (BestPlatformLang), Attributes, 0, 0, &Variable, NULL);

        DEBUG ((EFI_D_INFO, "Variable Driver Auto Update Lang, Lang:%a, PlatformLang:%a\n", BestLang, BestPlatformLang));
        ASSERT_EFI_ERROR (Status);
      }
    }
  }
}

/**
  Update the variable region with Variable information. If EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is set,
  index of associated public key is needed.

  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Data               Variable data.
  @param[in] DataSize           Size of data. 0 means delete.
  @param[in] Attributes         Attributes of the variable.
  @param[in] KeyIndex           Index of associated public key.
  @param[in] MonotonicCount     Value of associated monotonic count.
  @param[in, out] CacheVariable The variable information which is used to keep track of variable usage.
  @param[in] TimeStamp          Value of associated TimeStamp.

  @retval EFI_SUCCESS           The update operation is success.
  @retval EFI_OUT_OF_RESOURCES  Variable region is full, can not write other data into this region.

**/
EFI_STATUS
UpdateVariable (
  IN      CHAR16                      *VariableName,
  IN      EFI_GUID                    *VendorGuid,
  IN      VOID                        *Data,
  IN      UINTN                       DataSize,
  IN      UINT32                      Attributes      OPTIONAL,
  IN      UINT32                      KeyIndex        OPTIONAL,
  IN      UINT64                      MonotonicCount  OPTIONAL,
  IN OUT  VARIABLE_POINTER_TRACK      *CacheVariable,
  IN      EFI_TIME                    *TimeStamp      OPTIONAL
  )
{
  EFI_STATUS                          Status;
  VARIABLE_HEADER                     *NextVariable;
  UINTN                               ScratchSize;
  UINTN                               MaxDataSize;
  UINTN                               NonVolatileVarableStoreSize;
  UINTN                               VarNameOffset;
  UINTN                               VarDataOffset;
  UINTN                               VarNameSize;
  UINTN                               VarSize;
  BOOLEAN                             Volatile;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  UINT8                               State;
  VARIABLE_POINTER_TRACK              *Variable;
  VARIABLE_POINTER_TRACK              NvVariable;
  VARIABLE_STORE_HEADER               *VariableStoreHeader;
  UINTN                               CacheOffset;
  UINTN                               BufSize;
  UINTN                               DataOffset;

  if (mVariableModuleGlobal->FvbInstance == NULL) {
    //
    // The FVB protocol is not installed, so the EFI_VARIABLE_WRITE_ARCH_PROTOCOL is not installed.
    //
    if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      //
      // Trying to update NV variable prior to the installation of EFI_VARIABLE_WRITE_ARCH_PROTOCOL
      //
      return EFI_NOT_AVAILABLE_YET;
    } else if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
      //
      // Trying to update volatile authenticated variable prior to the installation of EFI_VARIABLE_WRITE_ARCH_PROTOCOL
      // The authenticated variable perhaps is not initialized, just return here.
      //
      return EFI_NOT_AVAILABLE_YET;
    }
  }

  if ((CacheVariable->CurrPtr == NULL) || CacheVariable->Volatile) {
    Variable = CacheVariable;
  } else {
    //
    // Update/Delete existing NV variable.
    // CacheVariable points to the variable in the memory copy of Flash area
    // Now let Variable points to the same variable in Flash area.
    //
    VariableStoreHeader  = (VARIABLE_STORE_HEADER *) ((UINTN) mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase);
    Variable = &NvVariable;
    Variable->StartPtr = GetStartPointer (VariableStoreHeader);
    Variable->EndPtr   = GetEndPointer (VariableStoreHeader);
    Variable->CurrPtr  = (VARIABLE_HEADER *)((UINTN)Variable->StartPtr + ((UINTN)CacheVariable->CurrPtr - (UINTN)CacheVariable->StartPtr));
    if (CacheVariable->InDeletedTransitionPtr != NULL) {
      Variable->InDeletedTransitionPtr = (VARIABLE_HEADER *)((UINTN)Variable->StartPtr + ((UINTN)CacheVariable->InDeletedTransitionPtr - (UINTN)CacheVariable->StartPtr));
    } else {
      Variable->InDeletedTransitionPtr = NULL;
    }
    Variable->Volatile = FALSE;
  }

  Fvb       = mVariableModuleGlobal->FvbInstance;

  //
  // Tricky part: Use scratch data area at the end of volatile variable store
  // as a temporary storage.
  //
  NextVariable = GetEndPointer ((VARIABLE_STORE_HEADER *) ((UINTN) mVariableModuleGlobal->VariableGlobal.VolatileVariableBase));
  ScratchSize = MAX (PcdGet32 (PcdMaxVariableSize), PcdGet32 (PcdMaxHardwareErrorVariableSize));


  if (Variable->CurrPtr != NULL) {
    //
    // Update/Delete existing variable.
    //
    if (AtRuntime ()) {
      //
      // If AtRuntime and the variable is Volatile and Runtime Access,
      // the volatile is ReadOnly, and SetVariable should be aborted and
      // return EFI_WRITE_PROTECTED.
      //
      if (Variable->Volatile) {
        Status = EFI_WRITE_PROTECTED;
        goto Done;
      }
      //
      // Only variable that have NV attributes can be updated/deleted in Runtime.
      //
      if ((Variable->CurrPtr->Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
      
      //
      // Only variable that have RT attributes can be updated/deleted in Runtime.
      //
      if ((Variable->CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
    }

    //
    // Setting a data variable with no access, or zero DataSize attributes
    // causes it to be deleted.
    // When the EFI_VARIABLE_APPEND_WRITE attribute is set, DataSize of zero will
    // not delete the variable.
    //
    if ((((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) && (DataSize == 0))|| ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0)) {
      if (Variable->InDeletedTransitionPtr != NULL) {
        //
        // Both ADDED and IN_DELETED_TRANSITION variable are present,
        // set IN_DELETED_TRANSITION one to DELETED state first.
        //
        State = Variable->InDeletedTransitionPtr->State;
        State &= VAR_DELETED;
        Status = UpdateVariableStore (
                   &mVariableModuleGlobal->VariableGlobal,
                   Variable->Volatile,
                   FALSE,
                   Fvb,
                   (UINTN) &Variable->InDeletedTransitionPtr->State,
                   sizeof (UINT8),
                   &State
                   );
        if (!EFI_ERROR (Status)) {
          if (!Variable->Volatile) {
            ASSERT (CacheVariable->InDeletedTransitionPtr != NULL);
            CacheVariable->InDeletedTransitionPtr->State = State;
          }
        } else {
          goto Done;
        }
      }

      State = Variable->CurrPtr->State;
      State &= VAR_DELETED;

      Status = UpdateVariableStore (
                 &mVariableModuleGlobal->VariableGlobal,
                 Variable->Volatile,
                 FALSE,
                 Fvb,
                 (UINTN) &Variable->CurrPtr->State,
                 sizeof (UINT8),
                 &State
                 );
      if (!EFI_ERROR (Status)) {
        UpdateVariableInfo (VariableName, VendorGuid, Variable->Volatile, FALSE, FALSE, TRUE, FALSE);
        if (!Variable->Volatile) {
          CacheVariable->CurrPtr->State = State;
          FlushHobVariableToFlash (VariableName, VendorGuid);
        }
      }
      goto Done;
    }
    //
    // If the variable is marked valid, and the same data has been passed in,
    // then return to the caller immediately.
    //
    if (DataSizeOfVariable (Variable->CurrPtr) == DataSize &&
        (CompareMem (Data, GetVariableDataPtr (Variable->CurrPtr), DataSize) == 0)  &&
        ((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) &&
        (TimeStamp == NULL)) {
      //
      // Variable content unchanged and no need to update timestamp, just return.
      //
      UpdateVariableInfo (VariableName, VendorGuid, Variable->Volatile, FALSE, TRUE, FALSE, FALSE);
      Status = EFI_SUCCESS;
      goto Done;
    } else if ((Variable->CurrPtr->State == VAR_ADDED) ||
               (Variable->CurrPtr->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION))) {

      //
      // EFI_VARIABLE_APPEND_WRITE attribute only effects for existing variable
      //
      if ((Attributes & EFI_VARIABLE_APPEND_WRITE) != 0) {
        //
        // Cache the previous variable data into StorageArea.
        //
        DataOffset = sizeof (VARIABLE_HEADER) + Variable->CurrPtr->NameSize + GET_PAD_SIZE (Variable->CurrPtr->NameSize);
        CopyMem (mStorageArea, (UINT8*)((UINTN) Variable->CurrPtr + DataOffset), Variable->CurrPtr->DataSize);

        //
        // Set Max Common Variable Data Size as default MaxDataSize 
        //
        MaxDataSize = PcdGet32 (PcdMaxVariableSize) - sizeof (VARIABLE_HEADER) - StrSize (VariableName) - GET_PAD_SIZE (StrSize (VariableName));


        if ((CompareGuid (VendorGuid, &gEfiImageSecurityDatabaseGuid) &&
            ((StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE) == 0) || (StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE1) == 0))) ||
            (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) && (StrCmp (VariableName, EFI_KEY_EXCHANGE_KEY_NAME) == 0))) {

          //
          // For variables with formatted as EFI_SIGNATURE_LIST, the driver shall not perform an append of
          // EFI_SIGNATURE_DATA values that are already part of the existing variable value.
          //
          Status = AppendSignatureList (
                     mStorageArea, 
                     Variable->CurrPtr->DataSize, 
                     MaxDataSize - Variable->CurrPtr->DataSize,
                     Data, 
                     DataSize, 
                     &BufSize
                     );
          if (Status == EFI_BUFFER_TOO_SMALL) {
            //
            // Signture List is too long, Failed to Append
            //
            Status = EFI_INVALID_PARAMETER;
            goto Done;
          }

          if (BufSize == Variable->CurrPtr->DataSize) {
            if ((TimeStamp == NULL) || CompareTimeStamp (TimeStamp, &Variable->CurrPtr->TimeStamp)) {
              //
              // New EFI_SIGNATURE_DATA is not found and timestamp is not later
              // than current timestamp, return EFI_SUCCESS directly.
              //
              UpdateVariableInfo (VariableName, VendorGuid, Variable->Volatile, FALSE, TRUE, FALSE, FALSE);
              Status = EFI_SUCCESS;
              goto Done;
            }
          }
        } else {
          //
          // For other Variables, append the new data to the end of previous data.
          // Max Harware error record variable data size is different from common variable
          //
          if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
            MaxDataSize = PcdGet32 (PcdMaxHardwareErrorVariableSize) - sizeof (VARIABLE_HEADER) - StrSize (VariableName) - GET_PAD_SIZE (StrSize (VariableName));
          }

          if (Variable->CurrPtr->DataSize + DataSize > MaxDataSize) {
            //
            // Exsiting data + Appended data exceed maximum variable size limitation 
            //
            Status = EFI_INVALID_PARAMETER;
            goto Done;
          }
          CopyMem ((UINT8*)((UINTN) mStorageArea + Variable->CurrPtr->DataSize), Data, DataSize);
          BufSize = Variable->CurrPtr->DataSize + DataSize;
        }

        //
        // Override Data and DataSize which are used for combined data area including previous and new data.
        //
        Data     = mStorageArea;
        DataSize = BufSize;
      }

      //
      // Mark the old variable as in delete transition.
      //
      State = Variable->CurrPtr->State;
      State &= VAR_IN_DELETED_TRANSITION;

      Status = UpdateVariableStore (
                 &mVariableModuleGlobal->VariableGlobal,
                 Variable->Volatile,
                 FALSE,
                 Fvb,
                 (UINTN) &Variable->CurrPtr->State,
                 sizeof (UINT8),
                 &State
                 );
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      if (!Variable->Volatile) {
        CacheVariable->CurrPtr->State = State;
      }
    }
  } else {
    //
    // Not found existing variable. Create a new variable.
    //

    if ((DataSize == 0) && ((Attributes & EFI_VARIABLE_APPEND_WRITE) != 0)) {
      Status = EFI_SUCCESS;
      goto Done;
    }

    //
    // Make sure we are trying to create a new variable.
    // Setting a data variable with zero DataSize or no access attributes means to delete it.
    //
    if (DataSize == 0 || (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }

    //
    // Only variable have NV|RT attribute can be created in Runtime.
    //
    if (AtRuntime () &&
        (((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0) || ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0))) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }
  }

  //
  // Function part - create a new variable and copy the data.
  // Both update a variable and create a variable will come here.

  SetMem (NextVariable, ScratchSize, 0xff);

  NextVariable->StartId     = VARIABLE_DATA;
  //
  // NextVariable->State = VAR_ADDED;
  //
  NextVariable->Reserved        = 0;
  NextVariable->PubKeyIndex     = KeyIndex;
  NextVariable->MonotonicCount  = MonotonicCount;
  ZeroMem (&NextVariable->TimeStamp, sizeof (EFI_TIME));

  if (((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) &&
      (TimeStamp != NULL)) {
    if ((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) {
      CopyMem (&NextVariable->TimeStamp, TimeStamp, sizeof (EFI_TIME));
    } else {
      //
      // In the case when the EFI_VARIABLE_APPEND_WRITE attribute is set, only
      // when the new TimeStamp value is later than the current timestamp associated
      // with the variable, we need associate the new timestamp with the updated value.
      //
      if (Variable->CurrPtr != NULL) {
        if (CompareTimeStamp (&Variable->CurrPtr->TimeStamp, TimeStamp)) {
          CopyMem (&NextVariable->TimeStamp, TimeStamp, sizeof (EFI_TIME));
        }
      }
    }
  }

  //
  // The EFI_VARIABLE_APPEND_WRITE attribute will never be set in the returned
  // Attributes bitmask parameter of a GetVariable() call.
  //
  NextVariable->Attributes  = Attributes & (~EFI_VARIABLE_APPEND_WRITE);

  VarNameOffset                 = sizeof (VARIABLE_HEADER);
  VarNameSize                   = StrSize (VariableName);
  CopyMem (
    (UINT8 *) ((UINTN) NextVariable + VarNameOffset),
    VariableName,
    VarNameSize
    );
  VarDataOffset = VarNameOffset + VarNameSize + GET_PAD_SIZE (VarNameSize);
  CopyMem (
    (UINT8 *) ((UINTN) NextVariable + VarDataOffset),
    Data,
    DataSize
    );
  CopyMem (&NextVariable->VendorGuid, VendorGuid, sizeof (EFI_GUID));
  //
  // There will be pad bytes after Data, the NextVariable->NameSize and
  // NextVariable->DataSize should not include pad size so that variable
  // service can get actual size in GetVariable.
  //
  NextVariable->NameSize  = (UINT32)VarNameSize;
  NextVariable->DataSize  = (UINT32)DataSize;

  //
  // The actual size of the variable that stores in storage should
  // include pad size.
  //
  VarSize = VarDataOffset + DataSize + GET_PAD_SIZE (DataSize);
  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
    //
    // Create a nonvolatile variable.
    //
    Volatile = FALSE;
    NonVolatileVarableStoreSize = ((VARIABLE_STORE_HEADER *)(UINTN)(mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase))->Size;
    if ((((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0)
      && ((VarSize + mVariableModuleGlobal->HwErrVariableTotalSize) > PcdGet32 (PcdHwErrStorageSize)))
      || (((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == 0)
      && ((VarSize + mVariableModuleGlobal->CommonVariableTotalSize) > NonVolatileVarableStoreSize - sizeof (VARIABLE_STORE_HEADER) - PcdGet32 (PcdHwErrStorageSize)))) {
      if (AtRuntime ()) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      //
      // Perform garbage collection & reclaim operation.
      //
      Status = Reclaim (
                 mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase,
                 &mVariableModuleGlobal->NonVolatileLastVariableOffset,
                 FALSE,
                 Variable,
                 FALSE,
                 FALSE
                 );
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      //
      // If still no enough space, return out of resources.
      //
      if ((((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0)
        && ((VarSize + mVariableModuleGlobal->HwErrVariableTotalSize) > PcdGet32 (PcdHwErrStorageSize)))
        || (((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == 0)
        && ((VarSize + mVariableModuleGlobal->CommonVariableTotalSize) > NonVolatileVarableStoreSize - sizeof (VARIABLE_STORE_HEADER) - PcdGet32 (PcdHwErrStorageSize)))) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      if (Variable->CurrPtr != NULL) {
        CacheVariable->CurrPtr = (VARIABLE_HEADER *)((UINTN) CacheVariable->StartPtr + ((UINTN) Variable->CurrPtr - (UINTN) Variable->StartPtr));
        CacheVariable->InDeletedTransitionPtr = NULL;
      }
    }
    //
    // Four steps
    // 1. Write variable header
    // 2. Set variable state to header valid
    // 3. Write variable data
    // 4. Set variable state to valid
    //
    //
    // Step 1:
    //
    CacheOffset = mVariableModuleGlobal->NonVolatileLastVariableOffset;
    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               FALSE,
               TRUE,
               Fvb,
               mVariableModuleGlobal->NonVolatileLastVariableOffset,
               sizeof (VARIABLE_HEADER),
               (UINT8 *) NextVariable
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Step 2:
    //
    NextVariable->State = VAR_HEADER_VALID_ONLY;
    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               FALSE,
               TRUE,
               Fvb,
               mVariableModuleGlobal->NonVolatileLastVariableOffset + OFFSET_OF (VARIABLE_HEADER, State),
               sizeof (UINT8),
               &NextVariable->State
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }
    //
    // Step 3:
    //
    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               FALSE,
               TRUE,
               Fvb,
               mVariableModuleGlobal->NonVolatileLastVariableOffset + sizeof (VARIABLE_HEADER),
               (UINT32) VarSize - sizeof (VARIABLE_HEADER),
               (UINT8 *) NextVariable + sizeof (VARIABLE_HEADER)
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }
    //
    // Step 4:
    //
    NextVariable->State = VAR_ADDED;
    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               FALSE,
               TRUE,
               Fvb,
               mVariableModuleGlobal->NonVolatileLastVariableOffset + OFFSET_OF (VARIABLE_HEADER, State),
               sizeof (UINT8),
               &NextVariable->State
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    mVariableModuleGlobal->NonVolatileLastVariableOffset += HEADER_ALIGN (VarSize);

    if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
      mVariableModuleGlobal->HwErrVariableTotalSize += HEADER_ALIGN (VarSize);
    } else {
      mVariableModuleGlobal->CommonVariableTotalSize += HEADER_ALIGN (VarSize);
    }
    //
    // update the memory copy of Flash region.
    //
    CopyMem ((UINT8 *)mNvVariableCache + CacheOffset, (UINT8 *)NextVariable, VarSize);
  } else {
    //
    // Create a volatile variable.
    //
    Volatile = TRUE;

    if ((UINT32) (VarSize + mVariableModuleGlobal->VolatileLastVariableOffset) >
        ((VARIABLE_STORE_HEADER *) ((UINTN) (mVariableModuleGlobal->VariableGlobal.VolatileVariableBase)))->Size) {
      //
      // Perform garbage collection & reclaim operation.
      //
      Status = Reclaim (
                 mVariableModuleGlobal->VariableGlobal.VolatileVariableBase,
                 &mVariableModuleGlobal->VolatileLastVariableOffset,
                 TRUE,
                 Variable,
                 FALSE,
                 FALSE
                 );
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      //
      // If still no enough space, return out of resources.
      //
      if ((UINT32) (VarSize + mVariableModuleGlobal->VolatileLastVariableOffset) >
            ((VARIABLE_STORE_HEADER *) ((UINTN) (mVariableModuleGlobal->VariableGlobal.VolatileVariableBase)))->Size
            ) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      if (Variable->CurrPtr != NULL) {
        CacheVariable->CurrPtr = (VARIABLE_HEADER *)((UINTN) CacheVariable->StartPtr + ((UINTN) Variable->CurrPtr - (UINTN) Variable->StartPtr));
        CacheVariable->InDeletedTransitionPtr = NULL;
      }
    }

    NextVariable->State = VAR_ADDED;
    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               TRUE,
               TRUE,
               Fvb,
               mVariableModuleGlobal->VolatileLastVariableOffset,
               (UINT32) VarSize,
               (UINT8 *) NextVariable
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    mVariableModuleGlobal->VolatileLastVariableOffset += HEADER_ALIGN (VarSize);
  }

  //
  // Mark the old variable as deleted.
  //
  if (!EFI_ERROR (Status) && Variable->CurrPtr != NULL) {
    if (Variable->InDeletedTransitionPtr != NULL) {
      //
      // Both ADDED and IN_DELETED_TRANSITION old variable are present,
      // set IN_DELETED_TRANSITION one to DELETED state first.
      //
      State = Variable->InDeletedTransitionPtr->State;
      State &= VAR_DELETED;
      Status = UpdateVariableStore (
                 &mVariableModuleGlobal->VariableGlobal,
                 Variable->Volatile,
                 FALSE,
                 Fvb,
                 (UINTN) &Variable->InDeletedTransitionPtr->State,
                 sizeof (UINT8),
                 &State
                 );
      if (!EFI_ERROR (Status)) {
        if (!Variable->Volatile) {
          ASSERT (CacheVariable->InDeletedTransitionPtr != NULL);
          CacheVariable->InDeletedTransitionPtr->State = State;
        }
      } else {
        goto Done;
      }
    }

    State = Variable->CurrPtr->State;
    State &= VAR_DELETED;

    Status = UpdateVariableStore (
             &mVariableModuleGlobal->VariableGlobal,
             Variable->Volatile,
             FALSE,
             Fvb,
             (UINTN) &Variable->CurrPtr->State,
             sizeof (UINT8),
             &State
             );
    if (!EFI_ERROR (Status) && !Variable->Volatile) {
      CacheVariable->CurrPtr->State = State;
    }
  }

  if (!EFI_ERROR (Status)) {
    UpdateVariableInfo (VariableName, VendorGuid, Volatile, FALSE, TRUE, FALSE, FALSE);
    if (!Volatile) {
      FlushHobVariableToFlash (VariableName, VendorGuid);
    }
  }

Done:
  return Status;
}

/**
  Check if a Unicode character is a hexadecimal character.

  This function checks if a Unicode character is a 
  hexadecimal character.  The valid hexadecimal character is 
  L'0' to L'9', L'a' to L'f', or L'A' to L'F'.


  @param Char           The character to check against.

  @retval TRUE          If the Char is a hexadecmial character.
  @retval FALSE         If the Char is not a hexadecmial character.

**/
BOOLEAN
EFIAPI
IsHexaDecimalDigitCharacter (
  IN CHAR16             Char
  )
{
  return (BOOLEAN) ((Char >= L'0' && Char <= L'9') || (Char >= L'A' && Char <= L'F') || (Char >= L'a' && Char <= L'f'));
}

/**

  This code checks if variable is hardware error record variable or not.

  According to UEFI spec, hardware error record variable should use the EFI_HARDWARE_ERROR_VARIABLE VendorGuid
  and have the L"HwErrRec####" name convention, #### is a printed hex value and no 0x or h is included in the hex value.

  @param VariableName   Pointer to variable name.
  @param VendorGuid     Variable Vendor Guid.

  @retval TRUE          Variable is hardware error record variable.
  @retval FALSE         Variable is not hardware error record variable.

**/
BOOLEAN
EFIAPI
IsHwErrRecVariable (
  IN CHAR16             *VariableName,
  IN EFI_GUID           *VendorGuid
  )
{
  if (!CompareGuid (VendorGuid, &gEfiHardwareErrorVariableGuid) ||
      (StrLen (VariableName) != StrLen (L"HwErrRec####")) ||
      (StrnCmp(VariableName, L"HwErrRec", StrLen (L"HwErrRec")) != 0) ||
      !IsHexaDecimalDigitCharacter (VariableName[0x8]) ||
      !IsHexaDecimalDigitCharacter (VariableName[0x9]) ||
      !IsHexaDecimalDigitCharacter (VariableName[0xA]) ||
      !IsHexaDecimalDigitCharacter (VariableName[0xB])) {
    return FALSE;
  }

  return TRUE;
}

/**
  This code checks if variable guid is global variable guid first.
  If yes, further check if variable name is in mGlobalVariableList or mGlobalVariableList2 and attributes matched.

  @param[in] VariableName       Pointer to variable name.
  @param[in] VendorGuid         Variable Vendor Guid.
  @param[in] Attributes         Attributes of the variable.

  @retval EFI_SUCCESS           Variable is not global variable, or Variable is global variable, variable name is in the lists and attributes matched.
  @retval EFI_INVALID_PARAMETER Variable is global variable, but variable name is not in the lists or attributes unmatched.

**/
EFI_STATUS
EFIAPI
CheckEfiGlobalVariable (
  IN CHAR16             *VariableName,
  IN EFI_GUID           *VendorGuid,
  IN UINT32             Attributes
  )
{
  UINTN     Index;
  UINTN     NameLength;

  if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid)){
    //
    // Try list 1, exactly match.
    //
    for (Index = 0; Index < sizeof (mGlobalVariableList)/sizeof (mGlobalVariableList[0]); Index++) {
      if ((StrCmp (mGlobalVariableList[Index].Name, VariableName) == 0) &&
          (Attributes == 0 || (Attributes & (~EFI_VARIABLE_APPEND_WRITE)) == mGlobalVariableList[Index].Attributes)) {
        return EFI_SUCCESS;
      }
    }

    //
    // Try list 2.
    //
    NameLength = StrLen (VariableName) - 4;
    for (Index = 0; Index < sizeof (mGlobalVariableList2)/sizeof (mGlobalVariableList2[0]); Index++) {
      if ((StrLen (VariableName) == StrLen (mGlobalVariableList2[Index].Name)) &&
          (StrnCmp (mGlobalVariableList2[Index].Name, VariableName, NameLength) == 0) &&
          IsHexaDecimalDigitCharacter (VariableName[NameLength]) &&
          IsHexaDecimalDigitCharacter (VariableName[NameLength + 1]) &&
          IsHexaDecimalDigitCharacter (VariableName[NameLength + 2]) &&
          IsHexaDecimalDigitCharacter (VariableName[NameLength + 3]) &&
          (Attributes == 0 || (Attributes & (~EFI_VARIABLE_APPEND_WRITE)) == mGlobalVariableList2[Index].Attributes)) {
        return EFI_SUCCESS;
      }
    }

    DEBUG ((EFI_D_INFO, "[Variable]: set global variable with invalid variable name or attributes - %g:%s:%x\n", VendorGuid, VariableName, Attributes));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Mark a variable that will become read-only after leaving the DXE phase of execution.

  @param[in] This          The VARIABLE_LOCK_PROTOCOL instance.
  @param[in] VariableName  A pointer to the variable name that will be made read-only subsequently.
  @param[in] VendorGuid    A pointer to the vendor GUID that will be made read-only subsequently.

  @retval EFI_SUCCESS           The variable specified by the VariableName and the VendorGuid was marked
                                as pending to be read-only.
  @retval EFI_INVALID_PARAMETER VariableName or VendorGuid is NULL.
                                Or VariableName is an empty string.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource to hold the lock request.
**/
EFI_STATUS
EFIAPI
VariableLockRequestToLock (
  IN CONST EDKII_VARIABLE_LOCK_PROTOCOL *This,
  IN       CHAR16                       *VariableName,
  IN       EFI_GUID                     *VendorGuid
  )
{
  VARIABLE_ENTRY                  *Entry;

  if (VariableName == NULL || VariableName[0] == 0 || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mEndOfDxe) {
    return EFI_ACCESS_DENIED;
  }

  Entry = AllocateRuntimePool (sizeof (*Entry) + StrSize (VariableName));
  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((EFI_D_INFO, "[Variable] Lock: %g:%s\n", VendorGuid, VariableName));

  AcquireLockOnlyAtBootTime(&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  Entry->Name = (CHAR16 *) (Entry + 1);
  StrCpy   (Entry->Name, VariableName);
  CopyGuid (&Entry->Guid, VendorGuid);
  InsertTailList (&mLockedVariableList, &Entry->Link);

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  return EFI_SUCCESS;
}

/**
  This code checks if variable should be treated as read-only variable.

  @param[in]      VariableName            Name of the Variable.
  @param[in]      VendorGuid              GUID of the Variable.

  @retval TRUE      This variable is read-only variable.
  @retval FALSE     This variable is NOT read-only variable.
  
**/
BOOLEAN
IsReadOnlyVariable (
  IN     CHAR16         *VariableName,
  IN     EFI_GUID       *VendorGuid
  )
{
  if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid)) {
    if ((StrCmp (VariableName, EFI_SETUP_MODE_NAME) == 0) ||
        (StrCmp (VariableName, EFI_SIGNATURE_SUPPORT_NAME) == 0) ||
        (StrCmp (VariableName, EFI_SECURE_BOOT_MODE_NAME) == 0) ||
        (StrCmp (VariableName, EFI_VENDOR_KEYS_VARIABLE_NAME) == 0) ||
        (StrCmp (VariableName, EFI_KEK_DEFAULT_VARIABLE_NAME) == 0) ||
        (StrCmp (VariableName, EFI_PK_DEFAULT_VARIABLE_NAME) == 0) ||
        (StrCmp (VariableName, EFI_DB_DEFAULT_VARIABLE_NAME) == 0) ||
        (StrCmp (VariableName, EFI_DBX_DEFAULT_VARIABLE_NAME) == 0) ||
        (StrCmp (VariableName, EFI_DBT_DEFAULT_VARIABLE_NAME) == 0)) {
      return TRUE;
    }
  }
  
  return FALSE;
}

/**

  This code finds variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize is external input.
  This function will do basic validation, before parse the data.

  @param VariableName               Name of Variable to be found.
  @param VendorGuid                 Variable vendor GUID.
  @param Attributes                 Attribute value of the variable found.
  @param DataSize                   Size of Data found. If size is less than the
                                    data, this value contains the required size.
  @param Data                       Data pointer.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Find the specified variable.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
VariableServiceGetVariable (
  IN      CHAR16            *VariableName,
  IN      EFI_GUID          *VendorGuid,
  OUT     UINT32            *Attributes OPTIONAL,
  IN OUT  UINTN             *DataSize,
  OUT     VOID              *Data
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarDataSize;

  if (VariableName == NULL || VendorGuid == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  Status = FindVariable (VariableName, VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
  if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Get data size
  //
  VarDataSize = DataSizeOfVariable (Variable.CurrPtr);
  ASSERT (VarDataSize != 0);

  if (*DataSize >= VarDataSize) {
    if (Data == NULL) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    CopyMem (Data, GetVariableDataPtr (Variable.CurrPtr), VarDataSize);
    if (Attributes != NULL) {
      *Attributes = Variable.CurrPtr->Attributes;
    }

    *DataSize = VarDataSize;
    UpdateVariableInfo (VariableName, VendorGuid, Variable.Volatile, TRUE, FALSE, FALSE, FALSE);

    Status = EFI_SUCCESS;
    goto Done;
  } else {
    *DataSize = VarDataSize;
    Status = EFI_BUFFER_TOO_SMALL;
    goto Done;
  }

Done:
  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
  return Status;
}



/**

  This code Finds the Next available variable.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param VariableNameSize           Size of the variable name.
  @param VariableName               Pointer to variable name.
  @param VendorGuid                 Variable Vendor Guid.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Find the specified variable.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
VariableServiceGetNextVariableName (
  IN OUT  UINTN             *VariableNameSize,
  IN OUT  CHAR16            *VariableName,
  IN OUT  EFI_GUID          *VendorGuid
  )
{
  VARIABLE_STORE_TYPE     Type;
  VARIABLE_POINTER_TRACK  Variable;
  VARIABLE_POINTER_TRACK  VariableInHob;
  VARIABLE_POINTER_TRACK  VariablePtrTrack;
  UINTN                   VarNameSize;
  EFI_STATUS              Status;
  VARIABLE_STORE_HEADER   *VariableStoreHeader[VariableStoreTypeMax];

  if (VariableNameSize == NULL || VariableName == NULL || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  Status = FindVariable (VariableName, VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
  if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
    goto Done;
  }

  if (VariableName[0] != 0) {
    //
    // If variable name is not NULL, get next variable.
    //
    Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
  }

  //
  // 0: Volatile, 1: HOB, 2: Non-Volatile.
  // The index and attributes mapping must be kept in this order as FindVariable
  // makes use of this mapping to implement search algorithm.
  //
  VariableStoreHeader[VariableStoreTypeVolatile] = (VARIABLE_STORE_HEADER *) (UINTN) mVariableModuleGlobal->VariableGlobal.VolatileVariableBase;
  VariableStoreHeader[VariableStoreTypeHob]      = (VARIABLE_STORE_HEADER *) (UINTN) mVariableModuleGlobal->VariableGlobal.HobVariableBase;
  VariableStoreHeader[VariableStoreTypeNv]       = mNvVariableCache;

  while (TRUE) {
    //
    // Switch from Volatile to HOB, to Non-Volatile.
    //
    while ((Variable.CurrPtr >= Variable.EndPtr) ||
           (Variable.CurrPtr == NULL)            ||
           !IsValidVariableHeader (Variable.CurrPtr)
          ) {
      //
      // Find current storage index
      //
      for (Type = (VARIABLE_STORE_TYPE) 0; Type < VariableStoreTypeMax; Type++) {
        if ((VariableStoreHeader[Type] != NULL) && (Variable.StartPtr == GetStartPointer (VariableStoreHeader[Type]))) {
          break;
        }
      }
      ASSERT (Type < VariableStoreTypeMax);
      //
      // Switch to next storage
      //
      for (Type++; Type < VariableStoreTypeMax; Type++) {
        if (VariableStoreHeader[Type] != NULL) {
          break;
        }
      }
      //
      // Capture the case that
      // 1. current storage is the last one, or
      // 2. no further storage
      //
      if (Type == VariableStoreTypeMax) {
        Status = EFI_NOT_FOUND;
        goto Done;
      }
      Variable.StartPtr = GetStartPointer (VariableStoreHeader[Type]);
      Variable.EndPtr   = GetEndPointer   (VariableStoreHeader[Type]);
      Variable.CurrPtr  = Variable.StartPtr;
    }

    //
    // Variable is found
    //
    if (Variable.CurrPtr->State == VAR_ADDED || Variable.CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
      if (!AtRuntime () || ((Variable.CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) != 0)) {
        if (Variable.CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
          //
          // If it is a IN_DELETED_TRANSITION variable,
          // and there is also a same ADDED one at the same time,
          // don't return it.
          //
          VariablePtrTrack.StartPtr = Variable.StartPtr;
          VariablePtrTrack.EndPtr = Variable.EndPtr;
          Status = FindVariableEx (
                     GetVariableNamePtr (Variable.CurrPtr),
                     &Variable.CurrPtr->VendorGuid,
                     FALSE,
                     &VariablePtrTrack
                     );
          if (!EFI_ERROR (Status) && VariablePtrTrack.CurrPtr->State == VAR_ADDED) {
            Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
            continue;
          }
        }

        //
        // Don't return NV variable when HOB overrides it
        //
        if ((VariableStoreHeader[VariableStoreTypeHob] != NULL) && (VariableStoreHeader[VariableStoreTypeNv] != NULL) &&
            (Variable.StartPtr == GetStartPointer (VariableStoreHeader[VariableStoreTypeNv]))
           ) {
          VariableInHob.StartPtr = GetStartPointer (VariableStoreHeader[VariableStoreTypeHob]);
          VariableInHob.EndPtr   = GetEndPointer   (VariableStoreHeader[VariableStoreTypeHob]);
          Status = FindVariableEx (
                     GetVariableNamePtr (Variable.CurrPtr),
                     &Variable.CurrPtr->VendorGuid,
                     FALSE,
                     &VariableInHob
                     );
          if (!EFI_ERROR (Status)) {
            Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
            continue;
          }
        }

        VarNameSize = NameSizeOfVariable (Variable.CurrPtr);
        ASSERT (VarNameSize != 0);

        if (VarNameSize <= *VariableNameSize) {
          CopyMem (VariableName, GetVariableNamePtr (Variable.CurrPtr), VarNameSize);
          CopyMem (VendorGuid, &Variable.CurrPtr->VendorGuid, sizeof (EFI_GUID));
          Status = EFI_SUCCESS;
        } else {
          Status = EFI_BUFFER_TOO_SMALL;
        }

        *VariableNameSize = VarNameSize;
        goto Done;
      }
    }

    Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
  }

Done:
  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
  return Status;
}

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.
  This function will check attribute carefully to avoid authentication bypass.

  @param VariableName                     Name of Variable to be found.
  @param VendorGuid                       Variable vendor GUID.
  @param Attributes                       Attribute value of the variable found
  @param DataSize                         Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param Data                             Data pointer.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SUCCESS                     Set successfully.
  @return EFI_OUT_OF_RESOURCES            Resource not enough to set variable.
  @return EFI_NOT_FOUND                   Not found.
  @return EFI_WRITE_PROTECTED             Variable is read-only.

**/
EFI_STATUS
EFIAPI
VariableServiceSetVariable (
  IN CHAR16                  *VariableName,
  IN EFI_GUID                *VendorGuid,
  IN UINT32                  Attributes,
  IN UINTN                   DataSize,
  IN VOID                    *Data
  )
{
  VARIABLE_POINTER_TRACK              Variable;
  EFI_STATUS                          Status;
  VARIABLE_HEADER                     *NextVariable;
  EFI_PHYSICAL_ADDRESS                Point;
  UINTN                               PayloadSize;
  LIST_ENTRY                          *Link;
  VARIABLE_ENTRY                      *Entry;

  //
  // Check input parameters.
  //
  if (VariableName == NULL || VariableName[0] == 0 || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsReadOnlyVariable (VariableName, VendorGuid)) {
    return EFI_WRITE_PROTECTED;
  }

  if (DataSize != 0 && Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for reserverd bit in variable attribute.
  //
  if ((Attributes & (~EFI_VARIABLE_ATTRIBUTES_MASK)) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  //  Make sure if runtime bit is set, boot service bit is set also.
  //
  if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS and EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute
  // cannot be set both.
  //
  if (((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
     && ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) {
    if (DataSize < AUTHINFO_SIZE) {
      //
      // Try to write Authenticated Variable without AuthInfo.
      //
      return EFI_SECURITY_VIOLATION;
    }
    PayloadSize = DataSize - AUTHINFO_SIZE;
  } else if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) {
    //
    // Sanity check for EFI_VARIABLE_AUTHENTICATION_2 descriptor.
    //
    if (DataSize < OFFSET_OF_AUTHINFO2_CERT_DATA ||
      ((EFI_VARIABLE_AUTHENTICATION_2 *) Data)->AuthInfo.Hdr.dwLength > DataSize - (OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo)) ||
      ((EFI_VARIABLE_AUTHENTICATION_2 *) Data)->AuthInfo.Hdr.dwLength < OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData)) {
      return EFI_SECURITY_VIOLATION;
    }
    PayloadSize = DataSize - AUTHINFO2_SIZE (Data);
  } else {
    PayloadSize = DataSize;
  }

  if ((UINTN)(~0) - PayloadSize < StrSize(VariableName)){
    //
    // Prevent whole variable size overflow 
    // 
    return EFI_INVALID_PARAMETER;
  }

  //
  //  The size of the VariableName, including the Unicode Null in bytes plus
  //  the DataSize is limited to maximum size of PcdGet32 (PcdMaxHardwareErrorVariableSize)
  //  bytes for HwErrRec, and PcdGet32 (PcdMaxVariableSize) bytes for the others.
  //
  if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    if (StrSize (VariableName) + PayloadSize > PcdGet32 (PcdMaxHardwareErrorVariableSize) - sizeof (VARIABLE_HEADER)) {
      return EFI_INVALID_PARAMETER;
    }
    if (!IsHwErrRecVariable(VariableName, VendorGuid)) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    //  The size of the VariableName, including the Unicode Null in bytes plus
    //  the DataSize is limited to maximum size of PcdGet32 (PcdMaxVariableSize) bytes.
    //
    if (StrSize (VariableName) + PayloadSize > PcdGet32 (PcdMaxVariableSize) - sizeof (VARIABLE_HEADER)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  Status = CheckEfiGlobalVariable (VariableName, VendorGuid, Attributes);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  AcquireLockOnlyAtBootTime(&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  //
  // Consider reentrant in MCA/INIT/NMI. It needs be reupdated.
  //
  if (1 < InterlockedIncrement (&mVariableModuleGlobal->VariableGlobal.ReentrantState)) {
    Point = mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase;
    //
    // Parse non-volatile variable data and get last variable offset.
    //
    NextVariable  = GetStartPointer ((VARIABLE_STORE_HEADER *) (UINTN) Point);
    while ((NextVariable < GetEndPointer ((VARIABLE_STORE_HEADER *) (UINTN) Point))
        && IsValidVariableHeader (NextVariable)) {
      NextVariable = GetNextVariablePtr (NextVariable);
    }
    mVariableModuleGlobal->NonVolatileLastVariableOffset = (UINTN) NextVariable - (UINTN) Point;
  }

  if (mEndOfDxe && mEnableLocking) {
    //
    // Treat the variables listed in the forbidden variable list as read-only after leaving DXE phase.
    //
    for ( Link = GetFirstNode (&mLockedVariableList)
        ; !IsNull (&mLockedVariableList, Link)
        ; Link = GetNextNode (&mLockedVariableList, Link)
        ) {
      Entry = BASE_CR (Link, VARIABLE_ENTRY, Link);
      if (CompareGuid (&Entry->Guid, VendorGuid) && (StrCmp (Entry->Name, VariableName) == 0)) {
        Status = EFI_WRITE_PROTECTED;
        DEBUG ((EFI_D_INFO, "[Variable]: Changing readonly variable after leaving DXE phase - %g:%s\n", VendorGuid, VariableName));
        goto Done;
      }
    }
  }

  //
  // Check whether the input variable is already existed.
  //
  Status = FindVariable (VariableName, VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, TRUE);
  if (!EFI_ERROR (Status)) {
    if (((Variable.CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0) && AtRuntime ()) {
      Status = EFI_WRITE_PROTECTED;
      goto Done;
    }
    if (Attributes != 0 && (Attributes & (~EFI_VARIABLE_APPEND_WRITE)) != Variable.CurrPtr->Attributes) {
      //
      // If a preexisting variable is rewritten with different attributes, SetVariable() shall not
      // modify the variable and shall return EFI_INVALID_PARAMETER. Two exceptions to this rule:
      // 1. No access attributes specified
      // 2. The only attribute differing is EFI_VARIABLE_APPEND_WRITE
      //
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }
  }
  
  //
  // Hook the operation of setting PlatformLangCodes/PlatformLang and LangCodes/Lang.
  //
  AutoUpdateLangVariable (VariableName, Data, DataSize);
  //
  // Process PK, KEK, Sigdb seperately.
  //
  if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) && (StrCmp (VariableName, EFI_PLATFORM_KEY_NAME) == 0)){
    Status = ProcessVarWithPk (VariableName, VendorGuid, Data, DataSize, &Variable, Attributes, TRUE);
  } else if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) && (StrCmp (VariableName, EFI_KEY_EXCHANGE_KEY_NAME) == 0)) {
    Status = ProcessVarWithPk (VariableName, VendorGuid, Data, DataSize, &Variable, Attributes, FALSE);
  } else if (CompareGuid (VendorGuid, &gEfiImageSecurityDatabaseGuid) && 
          ((StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE) == 0) || (StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE1) == 0))) {
    Status = ProcessVarWithPk (VariableName, VendorGuid, Data, DataSize, &Variable, Attributes, FALSE);
    if (EFI_ERROR (Status)) {
      Status = ProcessVarWithKek (VariableName, VendorGuid, Data, DataSize, &Variable, Attributes);
    }
  } else {
    Status = ProcessVariable (VariableName, VendorGuid, Data, DataSize, &Variable, Attributes);
  }

Done:
  InterlockedDecrement (&mVariableModuleGlobal->VariableGlobal.ReentrantState);
  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  if (!AtRuntime ()) {
    if (!EFI_ERROR (Status)) {
      SecureBootHook (
        VariableName,
        VendorGuid
        );
    }
  }

  return Status;
}

/**

  This code returns information about the EFI variables.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param Attributes                     Attributes bitmask to specify the type of variables
                                        on which to return information.
  @param MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                        for the EFI variables associated with the attributes specified.
  @param RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                        for EFI variables associated with the attributes specified.
  @param MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                        associated with the attributes specified.

  @return EFI_INVALID_PARAMETER         An invalid combination of attribute bits was supplied.
  @return EFI_SUCCESS                   Query successfully.
  @return EFI_UNSUPPORTED               The attribute is not supported on this platform.

**/
EFI_STATUS
EFIAPI
VariableServiceQueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize
  )
{
  VARIABLE_HEADER        *Variable;
  VARIABLE_HEADER        *NextVariable;
  UINT64                 VariableSize;
  VARIABLE_STORE_HEADER  *VariableStoreHeader;
  UINT64                 CommonVariableTotalSize;
  UINT64                 HwErrVariableTotalSize;

  CommonVariableTotalSize = 0;
  HwErrVariableTotalSize = 0;

  if(MaximumVariableStorageSize == NULL || RemainingVariableStorageSize == NULL || MaximumVariableSize == NULL || Attributes == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == 0) {
    //
    // Make sure the Attributes combination is supported by the platform.
    //
    return EFI_UNSUPPORTED;
  } else if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    //
    // Make sure if runtime bit is set, boot service bit is set also.
    //
    return EFI_INVALID_PARAMETER;
  } else if (AtRuntime () && ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0)) {
    //
    // Make sure RT Attribute is set if we are in Runtime phase.
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    //
    // Make sure Hw Attribute is set with NV.
    //
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  if((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
    //
    // Query is Volatile related.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) ((UINTN) mVariableModuleGlobal->VariableGlobal.VolatileVariableBase);
  } else {
    //
    // Query is Non-Volatile related.
    //
    VariableStoreHeader = mNvVariableCache;
  }

  //
  // Now let's fill *MaximumVariableStorageSize *RemainingVariableStorageSize
  // with the storage size (excluding the storage header size).
  //
  *MaximumVariableStorageSize   = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER);

  //
  // Harware error record variable needs larger size.
  //
  if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
    *MaximumVariableStorageSize = PcdGet32 (PcdHwErrStorageSize);
    *MaximumVariableSize = PcdGet32 (PcdMaxHardwareErrorVariableSize) - sizeof (VARIABLE_HEADER);
  } else {
    if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      ASSERT (PcdGet32 (PcdHwErrStorageSize) < VariableStoreHeader->Size);
      *MaximumVariableStorageSize = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER) - PcdGet32 (PcdHwErrStorageSize);
    }

    //
    // Let *MaximumVariableSize be PcdGet32 (PcdMaxVariableSize) with the exception of the variable header size.
    //
    *MaximumVariableSize = PcdGet32 (PcdMaxVariableSize) - sizeof (VARIABLE_HEADER);
  }

  //
  // Point to the starting address of the variables.
  //
  Variable = GetStartPointer (VariableStoreHeader);

  //
  // Now walk through the related variable store.
  //
  while ((Variable < GetEndPointer (VariableStoreHeader)) && IsValidVariableHeader (Variable)) {
    NextVariable = GetNextVariablePtr (Variable);
    VariableSize = (UINT64) (UINTN) NextVariable - (UINT64) (UINTN) Variable;

    if (AtRuntime ()) {
      //
      // We don't take the state of the variables in mind
      // when calculating RemainingVariableStorageSize,
      // since the space occupied by variables not marked with
      // VAR_ADDED is not allowed to be reclaimed in Runtime.
      //
      if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
        HwErrVariableTotalSize += VariableSize;
      } else {
        CommonVariableTotalSize += VariableSize;
      }
    } else {
      //
      // Only care about Variables with State VAR_ADDED, because
      // the space not marked as VAR_ADDED is reclaimable now.
      //
      if (Variable->State == VAR_ADDED) {
        if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
          HwErrVariableTotalSize += VariableSize;
        } else {
          CommonVariableTotalSize += VariableSize;
        }
      }
    }

    //
    // Go to the next one.
    //
    Variable = NextVariable;
  }

  if ((Attributes  & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD){
    *RemainingVariableStorageSize = *MaximumVariableStorageSize - HwErrVariableTotalSize;
  }else {
    *RemainingVariableStorageSize = *MaximumVariableStorageSize - CommonVariableTotalSize;
  }

  if (*RemainingVariableStorageSize < sizeof (VARIABLE_HEADER)) {
    *MaximumVariableSize = 0;
  } else if ((*RemainingVariableStorageSize - sizeof (VARIABLE_HEADER)) < *MaximumVariableSize) {
    *MaximumVariableSize = *RemainingVariableStorageSize - sizeof (VARIABLE_HEADER);
  }

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
  return EFI_SUCCESS;
}


/**
  This function reclaims variable storage if free size is below the threshold.

  Caution: This function may be invoked at SMM mode.
  Care must be taken to make sure not security issue.

**/
VOID
ReclaimForOS(
  VOID
  )
{
  EFI_STATUS                     Status;
  UINTN                          CommonVariableSpace;
  UINTN                          RemainingCommonVariableSpace;
  UINTN                          RemainingHwErrVariableSpace;

  Status  = EFI_SUCCESS;

  CommonVariableSpace = ((VARIABLE_STORE_HEADER *) ((UINTN) (mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase)))->Size - sizeof (VARIABLE_STORE_HEADER) - PcdGet32(PcdHwErrStorageSize); //Allowable max size of common variable storage space

  RemainingCommonVariableSpace = CommonVariableSpace - mVariableModuleGlobal->CommonVariableTotalSize;

  RemainingHwErrVariableSpace = PcdGet32 (PcdHwErrStorageSize) - mVariableModuleGlobal->HwErrVariableTotalSize;
  //
  // Check if the free area is blow a threshold.
  //
  if ((RemainingCommonVariableSpace < PcdGet32 (PcdMaxVariableSize))
    || ((PcdGet32 (PcdHwErrStorageSize) != 0) &&
       (RemainingHwErrVariableSpace < PcdGet32 (PcdMaxHardwareErrorVariableSize)))){
    Status = Reclaim (
            mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase,
            &mVariableModuleGlobal->NonVolatileLastVariableOffset,
            FALSE,
            NULL,
            FALSE,
            FALSE
            );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Init non-volatile variable store.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.
  @retval EFI_VOLUME_CORRUPTED  Variable Store or Firmware Volume for Variable Store is corrupted.

**/
EFI_STATUS
InitNonVolatileVariableStore (
  VOID
  )
{
  EFI_FIRMWARE_VOLUME_HEADER            *FvHeader;
  VARIABLE_HEADER                       *NextVariable;
  EFI_PHYSICAL_ADDRESS                  VariableStoreBase;
  UINT64                                VariableStoreLength;
  UINTN                                 VariableSize;
  EFI_HOB_GUID_TYPE                     *GuidHob;
  EFI_PHYSICAL_ADDRESS                  NvStorageBase;
  UINT8                                 *NvStorageData;
  UINT32                                NvStorageSize;
  FAULT_TOLERANT_WRITE_LAST_WRITE_DATA  *FtwLastWriteData;
  UINT32                                BackUpOffset;
  UINT32                                BackUpSize;

  mVariableModuleGlobal->FvbInstance = NULL;

  //
  // Note that in EdkII variable driver implementation, Hardware Error Record type variable
  // is stored with common variable in the same NV region. So the platform integrator should
  // ensure that the value of PcdHwErrStorageSize is less than or equal to the value of
  // PcdFlashNvStorageVariableSize.
  //
  ASSERT (PcdGet32 (PcdHwErrStorageSize) <= PcdGet32 (PcdFlashNvStorageVariableSize));

  //
  // Allocate runtime memory used for a memory copy of the FLASH region.
  // Keep the memory and the FLASH in sync as updates occur.
  //
  NvStorageSize = PcdGet32 (PcdFlashNvStorageVariableSize);
  NvStorageData = AllocateRuntimeZeroPool (NvStorageSize);
  if (NvStorageData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NvStorageBase = (EFI_PHYSICAL_ADDRESS) PcdGet64 (PcdFlashNvStorageVariableBase64);
  if (NvStorageBase == 0) {
    NvStorageBase = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashNvStorageVariableBase);
  }
  //
  // Copy NV storage data to the memory buffer.
  //
  CopyMem (NvStorageData, (UINT8 *) (UINTN) NvStorageBase, NvStorageSize);

  //
  // Check the FTW last write data hob.
  //
  GuidHob = GetFirstGuidHob (&gEdkiiFaultTolerantWriteGuid);
  if (GuidHob != NULL) {
    FtwLastWriteData = (FAULT_TOLERANT_WRITE_LAST_WRITE_DATA *) GET_GUID_HOB_DATA (GuidHob);
    if (FtwLastWriteData->TargetAddress == NvStorageBase) {
      DEBUG ((EFI_D_INFO, "Variable: NV storage is backed up in spare block: 0x%x\n", (UINTN) FtwLastWriteData->SpareAddress));
      //
      // Copy the backed up NV storage data to the memory buffer from spare block.
      //
      CopyMem (NvStorageData, (UINT8 *) (UINTN) (FtwLastWriteData->SpareAddress), NvStorageSize);
    } else if ((FtwLastWriteData->TargetAddress > NvStorageBase) &&
               (FtwLastWriteData->TargetAddress < (NvStorageBase + NvStorageSize))) {
      //
      // Flash NV storage from the Offset is backed up in spare block.
      //
      BackUpOffset = (UINT32) (FtwLastWriteData->TargetAddress - NvStorageBase);
      BackUpSize = NvStorageSize - BackUpOffset;
      DEBUG ((EFI_D_INFO, "Variable: High partial NV storage from offset: %x is backed up in spare block: 0x%x\n", BackUpOffset, (UINTN) FtwLastWriteData->SpareAddress));
      //
      // Copy the partial backed up NV storage data to the memory buffer from spare block.
      //
      CopyMem (NvStorageData + BackUpOffset, (UINT8 *) (UINTN) FtwLastWriteData->SpareAddress, BackUpSize);
    }
  }

  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) NvStorageData;

  //
  // Check if the Firmware Volume is not corrupted
  //
  if ((FvHeader->Signature != EFI_FVH_SIGNATURE) || (!CompareGuid (&gEfiSystemNvDataFvGuid, &FvHeader->FileSystemGuid))) {
    FreePool (NvStorageData);
    DEBUG ((EFI_D_ERROR, "Firmware Volume for Variable Store is corrupted\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  VariableStoreBase = (EFI_PHYSICAL_ADDRESS) ((UINTN) FvHeader + FvHeader->HeaderLength);
  VariableStoreLength = (UINT64) (NvStorageSize - FvHeader->HeaderLength);

  mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase = VariableStoreBase;
  mNvVariableCache = (VARIABLE_STORE_HEADER *) (UINTN) VariableStoreBase;
  if (GetVariableStoreStatus (mNvVariableCache) != EfiValid) {
    FreePool (NvStorageData);
    DEBUG((EFI_D_ERROR, "Variable Store header is corrupted\n"));
    return EFI_VOLUME_CORRUPTED;
  }
  ASSERT(mNvVariableCache->Size == VariableStoreLength);

  //
  // The max variable or hardware error variable size should be < variable store size.
  //
  ASSERT(MAX (PcdGet32 (PcdMaxVariableSize), PcdGet32 (PcdMaxHardwareErrorVariableSize)) < VariableStoreLength);

  //
  // Parse non-volatile variable data and get last variable offset.
  //
  NextVariable  = GetStartPointer ((VARIABLE_STORE_HEADER *)(UINTN)VariableStoreBase);
  while (IsValidVariableHeader (NextVariable)) {
    VariableSize = NextVariable->NameSize + NextVariable->DataSize + sizeof (VARIABLE_HEADER);
    if ((NextVariable->Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
      mVariableModuleGlobal->HwErrVariableTotalSize += HEADER_ALIGN (VariableSize);
    } else {
      mVariableModuleGlobal->CommonVariableTotalSize += HEADER_ALIGN (VariableSize);
    }

    NextVariable = GetNextVariablePtr (NextVariable);
  }
  mVariableModuleGlobal->NonVolatileLastVariableOffset = (UINTN) NextVariable - (UINTN) VariableStoreBase;

  return EFI_SUCCESS;
}

/**
  Flush the HOB variable to flash.

  @param[in] VariableName       Name of variable has been updated or deleted.
  @param[in] VendorGuid         Guid of variable has been updated or deleted.

**/
VOID
FlushHobVariableToFlash (
  IN CHAR16                     *VariableName,
  IN EFI_GUID                   *VendorGuid
  )
{
  EFI_STATUS                    Status;
  VARIABLE_STORE_HEADER         *VariableStoreHeader;
  VARIABLE_HEADER               *Variable;
  VOID                          *VariableData;
  BOOLEAN                       ErrorFlag;

  ErrorFlag = FALSE;

  //
  // Flush the HOB variable to flash.
  //
  if (mVariableModuleGlobal->VariableGlobal.HobVariableBase != 0) {
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) (UINTN) mVariableModuleGlobal->VariableGlobal.HobVariableBase;
    //
    // Set HobVariableBase to 0, it can avoid SetVariable to call back.
    //
    mVariableModuleGlobal->VariableGlobal.HobVariableBase = 0;
    for ( Variable = GetStartPointer (VariableStoreHeader)
        ; (Variable < GetEndPointer (VariableStoreHeader) && IsValidVariableHeader (Variable))
        ; Variable = GetNextVariablePtr (Variable)
        ) {
      if (Variable->State != VAR_ADDED) {
        //
        // The HOB variable has been set to DELETED state in local.
        //
        continue;
      }
      ASSERT ((Variable->Attributes & EFI_VARIABLE_NON_VOLATILE) != 0);
      if (VendorGuid == NULL || VariableName == NULL ||
          !CompareGuid (VendorGuid, &Variable->VendorGuid) ||
          StrCmp (VariableName, GetVariableNamePtr (Variable)) != 0) {
        VariableData = GetVariableDataPtr (Variable);
        Status = VariableServiceSetVariable (
                   GetVariableNamePtr (Variable),
                   &Variable->VendorGuid,
                   Variable->Attributes,
                   Variable->DataSize,
                   VariableData
                   );
        DEBUG ((EFI_D_INFO, "Variable driver flush the HOB variable to flash: %g %s %r\n", &Variable->VendorGuid, GetVariableNamePtr (Variable), Status));
      } else {
        //
        // The updated or deleted variable is matched with the HOB variable.
        // Don't break here because we will try to set other HOB variables
        // since this variable could be set successfully.
        //
        Status = EFI_SUCCESS;
      }
      if (!EFI_ERROR (Status)) {
        //
        // If set variable successful, or the updated or deleted variable is matched with the HOB variable,
        // set the HOB variable to DELETED state in local.
        //
        DEBUG ((EFI_D_INFO, "Variable driver set the HOB variable to DELETED state in local: %g %s\n", &Variable->VendorGuid, GetVariableNamePtr (Variable)));
        Variable->State &= VAR_DELETED;
      } else {
        ErrorFlag = TRUE;
      }
    }
    if (ErrorFlag) {
      //
      // We still have HOB variable(s) not flushed in flash.
      //
      mVariableModuleGlobal->VariableGlobal.HobVariableBase = (EFI_PHYSICAL_ADDRESS) (UINTN) VariableStoreHeader;
    } else {
      //
      // All HOB variables have been flushed in flash.
      //
      DEBUG ((EFI_D_INFO, "Variable driver: all HOB variables have been flushed in flash.\n"));
      if (!AtRuntime ()) {
        FreePool ((VOID *) VariableStoreHeader);
      }
    }
  }

}

/**
  Initializes variable write service after FTW was ready.

  @retval EFI_SUCCESS          Function successfully executed.
  @retval Others               Fail to initialize the variable service.

**/
EFI_STATUS
VariableWriteServiceInitialize (
  VOID
  )
{
  EFI_STATUS                      Status;
  VARIABLE_STORE_HEADER           *VariableStoreHeader;
  UINTN                           Index;
  UINT8                           Data;
  EFI_PHYSICAL_ADDRESS            VariableStoreBase;
  EFI_PHYSICAL_ADDRESS            NvStorageBase;

  NvStorageBase = (EFI_PHYSICAL_ADDRESS) PcdGet64 (PcdFlashNvStorageVariableBase64);
  if (NvStorageBase == 0) {
    NvStorageBase = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashNvStorageVariableBase);
  }
  VariableStoreBase = NvStorageBase + (((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)(NvStorageBase))->HeaderLength);

  //
  // Let NonVolatileVariableBase point to flash variable store base directly after FTW ready.
  //
  mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase = VariableStoreBase;
  VariableStoreHeader = (VARIABLE_STORE_HEADER *)(UINTN)VariableStoreBase;

  //
  // Check if the free area is really free.
  //
  for (Index = mVariableModuleGlobal->NonVolatileLastVariableOffset; Index < VariableStoreHeader->Size; Index++) {
    Data = ((UINT8 *) mNvVariableCache)[Index];
    if (Data != 0xff) {
      //
      // There must be something wrong in variable store, do reclaim operation.
      //
      Status = Reclaim (
                 mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase,
                 &mVariableModuleGlobal->NonVolatileLastVariableOffset,
                 FALSE,
                 NULL,
                 FALSE,
                 TRUE
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }
      break;
    }
  }

  FlushHobVariableToFlash (NULL, NULL);

  //
  // Authenticated variable initialize.
  //
  Status = AutenticatedVariableServiceInitialize ();

  return Status;
}


/**
  Initializes variable store area for non-volatile and volatile variable.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
VariableCommonInitialize (
  VOID
  )
{
  EFI_STATUS                      Status;
  VARIABLE_STORE_HEADER           *VolatileVariableStore;
  VARIABLE_STORE_HEADER           *VariableStoreHeader;
  UINT64                          VariableStoreLength;
  UINTN                           ScratchSize;
  EFI_HOB_GUID_TYPE               *GuidHob;

  //
  // Allocate runtime memory for variable driver global structure.
  //
  mVariableModuleGlobal = AllocateRuntimeZeroPool (sizeof (VARIABLE_MODULE_GLOBAL));
  if (mVariableModuleGlobal == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeLock (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock, TPL_NOTIFY);

  //
  // Get HOB variable store.
  //
  GuidHob = GetFirstGuidHob (&gEfiAuthenticatedVariableGuid);
  if (GuidHob != NULL) {
    VariableStoreHeader = GET_GUID_HOB_DATA (GuidHob);
    VariableStoreLength = (UINT64) (GuidHob->Header.HobLength - sizeof (EFI_HOB_GUID_TYPE));
    if (GetVariableStoreStatus (VariableStoreHeader) == EfiValid) {
      mVariableModuleGlobal->VariableGlobal.HobVariableBase = (EFI_PHYSICAL_ADDRESS) (UINTN) AllocateRuntimeCopyPool ((UINTN) VariableStoreLength, (VOID *) VariableStoreHeader);
      if (mVariableModuleGlobal->VariableGlobal.HobVariableBase == 0) {
        FreePool (mVariableModuleGlobal);
        return EFI_OUT_OF_RESOURCES;
      }
    } else {
      DEBUG ((EFI_D_ERROR, "HOB Variable Store header is corrupted!\n"));
    }
  }

  //
  // Allocate memory for volatile variable store, note that there is a scratch space to store scratch data.
  //
  ScratchSize = MAX (PcdGet32 (PcdMaxVariableSize), PcdGet32 (PcdMaxHardwareErrorVariableSize));
  VolatileVariableStore = AllocateRuntimePool (PcdGet32 (PcdVariableStoreSize) + ScratchSize);
  if (VolatileVariableStore == NULL) {
    if (mVariableModuleGlobal->VariableGlobal.HobVariableBase != 0) {
      FreePool ((VOID *) (UINTN) mVariableModuleGlobal->VariableGlobal.HobVariableBase);
    }
    FreePool (mVariableModuleGlobal);
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (VolatileVariableStore, PcdGet32 (PcdVariableStoreSize) + ScratchSize, 0xff);

  //
  // Initialize Variable Specific Data.
  //
  mVariableModuleGlobal->VariableGlobal.VolatileVariableBase = (EFI_PHYSICAL_ADDRESS) (UINTN) VolatileVariableStore;
  mVariableModuleGlobal->VolatileLastVariableOffset = (UINTN) GetStartPointer (VolatileVariableStore) - (UINTN) VolatileVariableStore;

  CopyGuid (&VolatileVariableStore->Signature, &gEfiAuthenticatedVariableGuid);
  VolatileVariableStore->Size        = PcdGet32 (PcdVariableStoreSize);
  VolatileVariableStore->Format      = VARIABLE_STORE_FORMATTED;
  VolatileVariableStore->State       = VARIABLE_STORE_HEALTHY;
  VolatileVariableStore->Reserved    = 0;
  VolatileVariableStore->Reserved1   = 0;

  //
  // Init non-volatile variable store.
  //
  Status = InitNonVolatileVariableStore ();
  if (EFI_ERROR (Status)) {
    if (mVariableModuleGlobal->VariableGlobal.HobVariableBase != 0) {
      FreePool ((VOID *) (UINTN) mVariableModuleGlobal->VariableGlobal.HobVariableBase);
    }
    FreePool (mVariableModuleGlobal);
    FreePool (VolatileVariableStore);
  }

  return Status;
}


/**
  Get the proper fvb handle and/or fvb protocol by the given Flash address.

  @param[in]  Address       The Flash address.
  @param[out] FvbHandle     In output, if it is not NULL, it points to the proper FVB handle.
  @param[out] FvbProtocol   In output, if it is not NULL, it points to the proper FVB protocol.

**/
EFI_STATUS
GetFvbInfoByAddress (
  IN  EFI_PHYSICAL_ADDRESS                Address,
  OUT EFI_HANDLE                          *FvbHandle OPTIONAL,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvbProtocol OPTIONAL
  )
{
  EFI_STATUS                              Status;
  EFI_HANDLE                              *HandleBuffer;
  UINTN                                   HandleCount;
  UINTN                                   Index;
  EFI_PHYSICAL_ADDRESS                    FvbBaseAddress;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL      *Fvb;
  EFI_FIRMWARE_VOLUME_HEADER              *FwVolHeader;
  EFI_FVB_ATTRIBUTES_2                    Attributes;

  //
  // Get all FVB handles.
  //
  Status = GetFvbCountAndBuffer (&HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get the FVB to access variable store.
  //
  Fvb = NULL;
  for (Index = 0; Index < HandleCount; Index += 1, Status = EFI_NOT_FOUND, Fvb = NULL) {
    Status = GetFvbByHandle (HandleBuffer[Index], &Fvb);
    if (EFI_ERROR (Status)) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    // Ensure this FVB protocol supported Write operation.
    //
    Status = Fvb->GetAttributes (Fvb, &Attributes);
    if (EFI_ERROR (Status) || ((Attributes & EFI_FVB2_WRITE_STATUS) == 0)) {
      continue;
    }

    //
    // Compare the address and select the right one.
    //
    Status = Fvb->GetPhysicalAddress (Fvb, &FvbBaseAddress);
    if (EFI_ERROR (Status)) {
      continue;
    }

    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) FvbBaseAddress);
    if ((Address >= FvbBaseAddress) && (Address < (FvbBaseAddress + FwVolHeader->FvLength))) {
      if (FvbHandle != NULL) {
        *FvbHandle  = HandleBuffer[Index];
      }
      if (FvbProtocol != NULL) {
        *FvbProtocol = Fvb;
      }
      Status = EFI_SUCCESS;
      break;
    }
  }
  FreePool (HandleBuffer);

  if (Fvb == NULL) {
    Status = EFI_NOT_FOUND;
  }

  return Status;
}

