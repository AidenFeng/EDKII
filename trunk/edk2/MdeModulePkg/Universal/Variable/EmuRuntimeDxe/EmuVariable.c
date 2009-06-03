/** @file

  Emulation Variable services operate on the runtime volatile memory.
  The nonvolatile variable space doesn't exist.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Variable.h"

///
/// Don't use module globals after the SetVirtualAddress map is signaled
///
ESAL_VARIABLE_GLOBAL  *mVariableModuleGlobal;

VARIABLE_INFO_ENTRY *gVariableInfo = NULL;

///
/// The size of a 3 character ISO639 language code.
///
#define ISO_639_2_ENTRY_SIZE    3

/**
  Update the variable region with Variable information. These are the same 
  arguments as the EFI Variable services.

  @param[in] VariableName       Name of variable

  @param[in] VendorGuid         Guid of variable

  @param[in] Data               Variable data

  @param[in] DataSize           Size of data. 0 means delete

  @param[in] Attributes 	      Attribues of the variable

  @param[in] Variable           The variable information which is used to keep track of variable usage.

  @retval EFI_SUCCESS           The update operation is success.

  @retval EFI_OUT_OF_RESOURCES  Variable region is full, can not write other data into this region.

**/
EFI_STATUS
EFIAPI
UpdateVariable (
  IN      CHAR16                 *VariableName,
  IN      EFI_GUID               *VendorGuid,
  IN      VOID                   *Data,
  IN      UINTN                  DataSize,
  IN      UINT32                 Attributes OPTIONAL,
  IN      VARIABLE_POINTER_TRACK *Variable
  );

/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.
  Otherwise, VariableName and VendorGuid are compared.

  @param  VariableName                Name of the variable to be found.
  @param  VendorGuid                  Vendor GUID to be found.
  @param  PtrTrack                    VARIABLE_POINTER_TRACK structure for output,
                                      including the range searched and the target position.
  @param  Global                      Pointer to VARIABLE_GLOBAL structure, including
                                      base of volatile variable storage area, base of
                                      NV variable storage area, and a lock.

  @retval EFI_INVALID_PARAMETER       If VariableName is not an empty string, while
                                      VendorGuid is NULL.
  @retval EFI_SUCCESS                 Variable successfully found.
  @retval EFI_NOT_FOUND               Variable not found.

**/
EFI_STATUS
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN  VARIABLE_GLOBAL         *Global
  );

/**
  Acquires lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiAcquireLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiAcquireLock() at boot time, and simply returns
  at runtime

  @param  Lock         A pointer to the lock to acquire

**/
VOID
AcquireLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
  if (!EfiAtRuntime ()) {
    EfiAcquireLock (Lock);
  }
}

/**
  Releases lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiReleaseLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiReleaseLock() at boot time, and simply returns
  at runtime

  @param  Lock         A pointer to the lock to release

**/
VOID
ReleaseLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
  if (!EfiAtRuntime ()) {
    EfiReleaseLock (Lock);
  }
}

/**
  Gets pointer to the variable data.

  This function gets the pointer to the variable data according
  to the input pointer to the variable header.

  @param  Variable      Pointer to the variable header.

  @return Pointer to variable data

**/
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if (Variable->StartId != VARIABLE_DATA) {
    return NULL;
  }
  //
  // Be careful about pad size for alignment
  //
  return (UINT8 *) ((UINTN) GET_VARIABLE_NAME_PTR (Variable) + Variable->NameSize + GET_PAD_SIZE (Variable->NameSize));
}

/**
  Gets pointer to header of the next variable.

  This function gets the pointer to the next variable header according
  to the input point to the variable header.

  @param  Variable      Pointer to header of the next variable

  @return Pointer to next variable header.

**/
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_HEADER   *Variable
  )
{
  VARIABLE_HEADER *VarHeader;

  if (Variable->StartId != VARIABLE_DATA) {
    return NULL;
  }
  //
  // Be careful about pad size for alignment
  //
  VarHeader = (VARIABLE_HEADER *) (GetVariableDataPtr (Variable) + Variable->DataSize + GET_PAD_SIZE (Variable->DataSize));

  if (VarHeader->StartId != VARIABLE_DATA) {
    return NULL;
  }

  return VarHeader;
}

/**
  Gets pointer to the end of the variable storage area.

  This function gets pointer to the end of the variable storage
  area, according to the input variable store header.

  @param  VolHeader     Pointer to the variale store header

  @return Pointer to the end of the variable storage area.

**/
VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER       *VolHeader
  )
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *) ((UINTN) VolHeader + VolHeader->Size);
}

/**
  Routine used to track statistical information about variable usage. 
  The data is stored in the EFI system table so it can be accessed later.
  VariableInfo.efi can dump out the table. Only Boot Services variable 
  accesses are tracked by this code. The PcdVariableCollectStatistics
  build flag controls if this feature is enabled. 

  A read that hits in the cache will have Read and Cache true for 
  the transaction. Data is allocated by this routine, but never
  freed.

  @param[in] VariableName   Name of the Variable to track
  @param[in] VendorGuid     Guid of the Variable to track
  @param[in] Volatile       TRUE if volatile FALSE if non-volatile
  @param[in] Read           TRUE if GetVariable() was called
  @param[in] Write          TRUE if SetVariable() was called
  @param[in] Delete         TRUE if deleted via SetVariable()
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

    if (EfiAtRuntime ()) {
      // Don't collect statistics at runtime
      return;
    }

    if (gVariableInfo == NULL) {
      //
      // on the first call allocate a entry and place a pointer to it in
      // the EFI System Table
      //
      gVariableInfo = AllocateZeroPool (sizeof (VARIABLE_INFO_ENTRY));
      ASSERT (gVariableInfo != NULL);

      CopyGuid (&gVariableInfo->VendorGuid, VendorGuid);
      gVariableInfo->Name = AllocatePool (StrSize (VariableName));
      ASSERT (gVariableInfo->Name != NULL);
      StrCpy (gVariableInfo->Name, VariableName);
      gVariableInfo->Volatile = Volatile;

      gBS->InstallConfigurationTable (&gEfiVariableGuid, gVariableInfo);
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
        // Next iteration of the loop will fill in the data
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

  @retval the index of language in the language codes.

**/
UINTN
EFIAPI
GetIndexFromSupportedLangCodes(
  IN  CHAR8            *SupportedLang,
  IN  CHAR8            *Lang,
  IN  BOOLEAN          Iso639Language
  ) 
{
  UINTN    Index;
  UINT32   CompareLength;
  CHAR8    *Supported;

  Index = 0;
  Supported = SupportedLang;
  if (Iso639Language) {
    CompareLength = 3;
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
    while (*Supported != '\0') {
      //
      // take semicolon as delimitation, sequentially traverse supported language codes.
      //
      for (CompareLength = 0; *Supported != ';' && *Supported != '\0'; CompareLength++) {
        Supported++;
      }
      if (AsciiStrnCmp (Lang, Supported - CompareLength, CompareLength) == 0) {
        //
        // Successfully find the index of Lang string in SupportedLang string.
        //
        return Index;
      }
      Index++;
    }
    ASSERT (FALSE);
    return 0;
  }
}

/**
  Get language string from supported language codes according to index.

  This code is used to get corresponding language string in supported language codes. It can handle
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
  @param  Index                       the index in supported language codes.
  @param  Iso639Language              A bool value to signify if the handler is operated on ISO639 or RFC4646.

  @retval the language string in the language codes.

**/
CHAR8 *
EFIAPI
GetLangFromSupportedLangCodes (
  IN  CHAR8            *SupportedLang,
  IN  UINTN            Index,
  IN  BOOLEAN          Iso639Language
)
{
  UINTN    SubIndex;
  UINT32   CompareLength;
  CHAR8    *Supported;

  SubIndex  = 0;
  Supported = SupportedLang;
  if (Iso639Language) {
    //
    // according to the index of Lang string in SupportedLang string to get the language.
    // As this code will be invoked in RUNTIME, therefore there is not memory allocate/free operation.
    // In driver entry, it pre-allocates a runtime attribute memory to accommodate this string.
    //
    CompareLength = 3;
    SetMem (mVariableModuleGlobal->Lang, sizeof(mVariableModuleGlobal->Lang), 0);
    return CopyMem (mVariableModuleGlobal->Lang, SupportedLang + Index * CompareLength, CompareLength);
      
  } else {
    while (TRUE) {
      //
      // take semicolon as delimitation, sequentially traverse supported language codes.
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
        // according to the index of Lang string in SupportedLang string to get the language.
        // As this code will be invoked in RUNTIME, therefore there is not memory allocate/free operation.
        // In driver entry, it pre-allocates a runtime attribute memory to accommodate this string.
        //
        SetMem (mVariableModuleGlobal->PlatformLang, sizeof (mVariableModuleGlobal->PlatformLang), 0);
        return CopyMem (mVariableModuleGlobal->PlatformLang, Supported - CompareLength, CompareLength);
      }
      SubIndex++; 			 
    }
  }
}

/**
  Hook the operations in PlatformLangCodes, LangCodes, PlatformLang and Lang.

  When setting Lang/LangCodes, simultaneously update PlatformLang/PlatformLangCodes.

  According to UEFI spec, PlatformLangCodes/LangCodes are only set once in firmware initialization,
  and are read-only. Therefore, in variable driver, only store the original value for other use.

  @param[in] VariableName       Name of variable

  @param[in] Data               Variable data

  @param[in] DataSize           Size of data. 0 means delete

  @retval EFI_SUCCESS  auto update operation is successful.

**/
EFI_STATUS
EFIAPI
AutoUpdateLangVariable(
  IN  CHAR16             *VariableName,
  IN  VOID               *Data,
  IN  UINTN              DataSize
  )
{
  EFI_STATUS     Status;
  CHAR8          *BestPlatformLang;
  CHAR8          *BestLang;
  UINTN          Index;
  UINT32         Attributes;
  VARIABLE_POINTER_TRACK Variable;

  //
  // According to UEFI spec, "Lang" and "PlatformLang" is NV|BS|RT attributions.
  //
  Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;

  if (StrCmp (VariableName, L"PlatformLangCodes") == 0) {
    //
    // According to UEFI spec, PlatformLangCodes is only set once in firmware initialization, and is read-only
    // Therefore, in variable driver, only store the original value for other use.
    //
    AsciiStrnCpy (mVariableModuleGlobal->PlatformLangCodes, Data, DataSize);
  } else if (StrCmp (VariableName, L"LangCodes") == 0) {
    //
    // According to UEFI spec, LangCodes is only set once in firmware initialization, and is read-only
    // Therefore, in variable driver, only store the original value for other use.
    //
    AsciiStrnCpy (mVariableModuleGlobal->LangCodes, Data, DataSize);
  } else if (StrCmp (VariableName, L"PlatformLang") == 0) {
    ASSERT (AsciiStrLen (mVariableModuleGlobal->PlatformLangCodes) != 0);

    //
    // When setting PlatformLang, firstly get most matched language string from supported language codes.
    //
    BestPlatformLang = GetBestLanguage(mVariableModuleGlobal->PlatformLangCodes, FALSE, Data, NULL);

    //
    // Get the corresponding index in language codes.
    //
    Index = GetIndexFromSupportedLangCodes(mVariableModuleGlobal->PlatformLangCodes, BestPlatformLang, FALSE);

    //
    // Get the corresponding ISO639 language tag according to RFC4646 language tag.
    //
    BestLang = GetLangFromSupportedLangCodes(mVariableModuleGlobal->LangCodes, Index, TRUE);

    //
    // Successfully convert PlatformLang to Lang, and set the BestLang value into Lang variable simultaneously.
    //
    FindVariable(L"Lang", &gEfiGlobalVariableGuid, &Variable, (VARIABLE_GLOBAL *)mVariableModuleGlobal);

    Status = UpdateVariable(L"Lang", &gEfiGlobalVariableGuid, 
                    BestLang, ISO_639_2_ENTRY_SIZE + 1, Attributes, &Variable);

    DEBUG((EFI_D_INFO, "Variable Driver Auto Update PlatformLang, PlatformLang:%a, Lang:%a\n", BestPlatformLang, BestLang));

    ASSERT_EFI_ERROR(Status);

  } else if (StrCmp (VariableName, L"Lang") == 0) {
    ASSERT (AsciiStrLen (mVariableModuleGlobal->LangCodes) != 0);

    //
    // When setting Lang, firstly get most matched language string from supported language codes.
    //
    BestLang = GetBestLanguage(mVariableModuleGlobal->LangCodes, TRUE, Data, NULL);

    //
    // Get the corresponding index in language codes.
    //
    Index = GetIndexFromSupportedLangCodes(mVariableModuleGlobal->LangCodes, BestLang, TRUE);

    //
    // Get the corresponding RFC4646 language tag according to ISO639 language tag.
    //
    BestPlatformLang = GetLangFromSupportedLangCodes(mVariableModuleGlobal->PlatformLangCodes, Index, FALSE);

    //
    // Successfully convert Lang to PlatformLang, and set the BestPlatformLang value into PlatformLang variable simultaneously.
    //
    FindVariable(L"PlatformLang", &gEfiGlobalVariableGuid, &Variable, (VARIABLE_GLOBAL *)mVariableModuleGlobal);

    Status = UpdateVariable(L"PlatformLang", &gEfiGlobalVariableGuid, 
                    BestPlatformLang, AsciiStrSize (BestPlatformLang), Attributes, &Variable);

    DEBUG((EFI_D_INFO, "Variable Driver Auto Update Lang, Lang:%a, PlatformLang:%a\n", BestLang, BestPlatformLang));
    ASSERT_EFI_ERROR(Status);
  }
  return EFI_SUCCESS;
}

/**
  Update the variable region with Variable information. These are the same 
  arguments as the EFI Variable services.

  @param[in] VariableName       Name of variable

  @param[in] VendorGuid         Guid of variable

  @param[in] Data               Variable data

  @param[in] DataSize           Size of data. 0 means delete

  @param[in] Attributes         Attribues of the variable

  @param[in] Variable           The variable information which is used to keep track of variable usage.

  @retval EFI_SUCCESS           The update operation is success.

  @retval EFI_OUT_OF_RESOURCES  Variable region is full, can not write other data into this region.

**/
EFI_STATUS
EFIAPI
UpdateVariable (
  IN      CHAR16          *VariableName,
  IN      EFI_GUID        *VendorGuid,
  IN      VOID            *Data,
  IN      UINTN           DataSize,
  IN      UINT32          Attributes OPTIONAL,
  IN      VARIABLE_POINTER_TRACK *Variable
  )
{
  EFI_STATUS              Status;
  VARIABLE_HEADER         *NextVariable;
  UINTN                   VarNameSize;
  UINTN                   VarNameOffset;
  UINTN                   VarDataOffset;
  UINTN                   VarSize;
  VARIABLE_GLOBAL         *Global;
  UINTN                               NonVolatileVarableStoreSize;

  Global = &mVariableModuleGlobal->VariableGlobal[Physical];

  if (Variable->CurrPtr != NULL) {
    //
    // Update/Delete existing variable
    //

    if (EfiAtRuntime ()) {        
      //
      // If EfiAtRuntime and the variable is Volatile and Runtime Access,  
      // the volatile is ReadOnly, and SetVariable should be aborted and 
      // return EFI_WRITE_PROTECTED.
      //
      if (Variable->Volatile) {
        Status = EFI_WRITE_PROTECTED;
        goto Done;
      }
      //
      // Only variable have NV attribute can be updated/deleted in Runtime
      //
      if ((Variable->CurrPtr->Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
    }

    //
    // Setting a data variable with no access, or zero DataSize attributes
    // specified causes it to be deleted.
    //
    if (DataSize == 0 || (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0) {
      Variable->CurrPtr->State &= VAR_DELETED;
      UpdateVariableInfo (VariableName, VendorGuid, Variable->Volatile, FALSE, FALSE, TRUE, FALSE);
      Status = EFI_SUCCESS;
      goto Done;
    }

    //
    // If the variable is marked valid and the same data has been passed in
    // then return to the caller immediately.
    //
    if (Variable->CurrPtr->DataSize == DataSize &&
        CompareMem (Data, GetVariableDataPtr (Variable->CurrPtr), DataSize) == 0
          ) {
      Status = EFI_SUCCESS;
      goto Done;
    } else if (Variable->CurrPtr->State == VAR_ADDED) {
      //
      // Mark the old variable as in delete transition
      //
      Variable->CurrPtr->State &= VAR_IN_DELETED_TRANSITION;
    }
    
  } else {
    //
    // No found existing variable, Create a new variable
    //  
    
    //
    // Make sure we are trying to create a new variable.
    // Setting a data variable with no access, or zero DataSize attributes means to delete it.    
    //
    if (DataSize == 0 || (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }
        
    //
    // Only variable have NV|RT attribute can be created in Runtime
    //
    if (EfiAtRuntime () &&
        (((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0) || ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0))) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }         
  }
  
  //
  // Function part - create a new variable and copy the data.
  // Both update a variable and create a variable will come here.
  //
  
  VarNameOffset = sizeof (VARIABLE_HEADER);
  VarNameSize   = StrSize (VariableName);
  VarDataOffset = VarNameOffset + VarNameSize + GET_PAD_SIZE (VarNameSize);
  VarSize       = VarDataOffset + DataSize + GET_PAD_SIZE (DataSize);

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
    NonVolatileVarableStoreSize = ((VARIABLE_STORE_HEADER *)(UINTN)(Global->NonVolatileVariableBase))->Size;
    if ((((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) 
      && ((VarSize + mVariableModuleGlobal->HwErrVariableTotalSize) > FixedPcdGet32(PcdHwErrStorageSize)))
      || (((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == 0) 
      && ((VarSize + mVariableModuleGlobal->CommonVariableTotalSize) > NonVolatileVarableStoreSize - sizeof (VARIABLE_STORE_HEADER) - FixedPcdGet32(PcdHwErrStorageSize)))) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    NextVariable  = (VARIABLE_HEADER *) (UINT8 *) (mVariableModuleGlobal->NonVolatileLastVariableOffset
                      + (UINTN) Global->NonVolatileVariableBase);
    mVariableModuleGlobal->NonVolatileLastVariableOffset += VarSize;

    if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
      mVariableModuleGlobal->HwErrVariableTotalSize += VarSize;
    } else {
      mVariableModuleGlobal->CommonVariableTotalSize += VarSize;
    }
  } else {
    if ((UINT32) (VarSize + mVariableModuleGlobal->VolatileLastVariableOffset) >
          ((VARIABLE_STORE_HEADER *) ((UINTN) (Global->VolatileVariableBase)))->Size
          ) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    NextVariable    = (VARIABLE_HEADER *) (UINT8 *) (mVariableModuleGlobal->VolatileLastVariableOffset
                        + (UINTN) Global->VolatileVariableBase);
    mVariableModuleGlobal->VolatileLastVariableOffset += VarSize;
  }

  NextVariable->StartId     = VARIABLE_DATA;
  NextVariable->Attributes  = Attributes;
  NextVariable->State       = VAR_ADDED;
  NextVariable->Reserved    = 0;

  //
  // There will be pad bytes after Data, the NextVariable->NameSize and
  // NextVariable->NameSize should not include pad size so that variable
  // service can get actual size in GetVariable
  //
  NextVariable->NameSize  = (UINT32)VarNameSize;
  NextVariable->DataSize  = (UINT32)DataSize;

  CopyMem (&NextVariable->VendorGuid, VendorGuid, sizeof (EFI_GUID));
  CopyMem (
    (UINT8 *) ((UINTN) NextVariable + VarNameOffset),
    VariableName,
    VarNameSize
    );
  CopyMem (
    (UINT8 *) ((UINTN) NextVariable + VarDataOffset),
    Data,
    DataSize
    );

  //
  // Mark the old variable as deleted
  //
  Variable->CurrPtr->State &= VAR_DELETED;

  UpdateVariableInfo (VariableName, VendorGuid, Variable->Volatile, FALSE, TRUE, FALSE, FALSE);

  Status = EFI_SUCCESS;

Done:
  return Status;
}

/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.
  Otherwise, VariableName and VendorGuid are compared.

  @param  VariableName                Name of the variable to be found.
  @param  VendorGuid                  Vendor GUID to be found.
  @param  PtrTrack                    VARIABLE_POINTER_TRACK structure for output,
                                      including the range searched and the target position.
  @param  Global                      Pointer to VARIABLE_GLOBAL structure, including
                                      base of volatile variable storage area, base of
                                      NV variable storage area, and a lock.

  @retval EFI_INVALID_PARAMETER       If VariableName is not an empty string, while
                                      VendorGuid is NULL.
  @retval EFI_SUCCESS                 Variable successfully found.
  @retval EFI_NOT_FOUND               Variable not found.

**/
EFI_STATUS
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN  VARIABLE_GLOBAL         *Global
  )
{
  VARIABLE_HEADER       *Variable[2];
  VARIABLE_STORE_HEADER *VariableStoreHeader[2];
  UINTN                 Index;

  //
  // 0: Non-Volatile, 1: Volatile
  //
  VariableStoreHeader[0]  = (VARIABLE_STORE_HEADER *) ((UINTN) Global->NonVolatileVariableBase);
  VariableStoreHeader[1]  = (VARIABLE_STORE_HEADER *) ((UINTN) Global->VolatileVariableBase);

  //
  // Start Pointers for the variable.
  // Actual Data Pointer where data can be written.
  //
  Variable[0] = (VARIABLE_HEADER *) (VariableStoreHeader[0] + 1);
  Variable[1] = (VARIABLE_HEADER *) (VariableStoreHeader[1] + 1);

  if (VariableName[0] != 0 && VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Find the variable by walk through non-volatile and volatile variable store
  //
  for (Index = 0; Index < 2; Index++) {
    PtrTrack->StartPtr  = (VARIABLE_HEADER *) (VariableStoreHeader[Index] + 1);
    PtrTrack->EndPtr    = GetEndPointer (VariableStoreHeader[Index]);

    while ((Variable[Index] < GetEndPointer (VariableStoreHeader[Index])) && (Variable[Index] != NULL)) {
      if (Variable[Index]->StartId == VARIABLE_DATA && Variable[Index]->State == VAR_ADDED) {
        if (!(EfiAtRuntime () && ((Variable[Index]->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0))) {
          if (VariableName[0] == 0) {
            PtrTrack->CurrPtr   = Variable[Index];
            PtrTrack->Volatile  = (BOOLEAN) Index;
            return EFI_SUCCESS;
          } else {
            if (CompareGuid (VendorGuid, &Variable[Index]->VendorGuid)) {
              if (CompareMem (VariableName, GET_VARIABLE_NAME_PTR (Variable[Index]), Variable[Index]->NameSize) == 0) {
                PtrTrack->CurrPtr   = Variable[Index];
                PtrTrack->Volatile  = (BOOLEAN) Index;
                return EFI_SUCCESS;
              }
            }
          }
        }
      }

      Variable[Index] = GetNextVariablePtr (Variable[Index]);
    }
  }
  PtrTrack->CurrPtr = NULL;
  return EFI_NOT_FOUND;
}

/**
  This code finds variable in storage blocks (Volatile or Non-Volatile).
  
  @param  VariableName           A Null-terminated Unicode string that is the name of
                                 the vendor's variable.
  @param  VendorGuid             A unique identifier for the vendor.
  @param  Attributes             If not NULL, a pointer to the memory location to return the 
                                 attributes bitmask for the variable.
  @param  DataSize               Size of Data found. If size is less than the
                                 data, this value contains the required size.
  @param  Data                   On input, the size in bytes of the return Data buffer.  
                                 On output, the size of data returned in Data.
  @param  Global                 Pointer to VARIABLE_GLOBAL structure

  @retval EFI_SUCCESS            The function completed successfully. 
  @retval EFI_NOT_FOUND          The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   DataSize is too small for the result.  DataSize has 
                                 been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER  VariableName or VendorGuid or DataSize is NULL.

**/
EFI_STATUS
EFIAPI
EmuGetVariable (
  IN      CHAR16            *VariableName,
  IN      EFI_GUID          *VendorGuid,
  OUT     UINT32            *Attributes OPTIONAL,
  IN OUT  UINTN             *DataSize,
  OUT     VOID              *Data,
  IN      VARIABLE_GLOBAL   *Global
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarDataSize;
  EFI_STATUS              Status;

  if (VariableName == NULL || VendorGuid == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&Global->VariableServicesLock);

  //
  // Find existing variable
  //
  Status = FindVariable (VariableName, VendorGuid, &Variable, Global);

  if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Get data size
  //
  VarDataSize = Variable.CurrPtr->DataSize;
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
  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return Status;
}

/**

  This code Finds the Next available variable.

  @param  VariableNameSize       Size of the variable.
  @param  VariableName           On input, supplies the last VariableName that was returned by GetNextVariableName().
                                 On output, returns the Null-terminated Unicode string of the current variable.
  @param  VendorGuid             On input, supplies the last VendorGuid that was returned by GetNextVariableName().
                                 On output, returns the VendorGuid of the current variable.  
  @param  Global                 Pointer to VARIABLE_GLOBAL structure.

  @retval EFI_SUCCESS            The function completed successfully. 
  @retval EFI_NOT_FOUND          The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   VariableNameSize is too small for the result. 
                                 VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER  VariableNameSize or VariableName or VendorGuid is NULL.

**/
EFI_STATUS
EFIAPI
EmuGetNextVariableName (
  IN OUT  UINTN             *VariableNameSize,
  IN OUT  CHAR16            *VariableName,
  IN OUT  EFI_GUID          *VendorGuid,
  IN      VARIABLE_GLOBAL   *Global
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarNameSize;
  EFI_STATUS              Status;

  if (VariableNameSize == NULL || VariableName == NULL || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&Global->VariableServicesLock);

  Status = FindVariable (VariableName, VendorGuid, &Variable, Global);

  if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
    goto Done;
  }

  while (TRUE) {
    if (VariableName[0] != 0) {
      //
      // If variable name is not NULL, get next variable
      //
      Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
    }
    //
    // If both volatile and non-volatile variable store are parsed,
    // return not found
    //
    if (Variable.CurrPtr >= Variable.EndPtr || Variable.CurrPtr == NULL) {
      Variable.Volatile = (BOOLEAN) (Variable.Volatile ^ ((BOOLEAN) 0x1));
      if (Variable.Volatile) {
        Variable.StartPtr = (VARIABLE_HEADER *) ((UINTN) (Global->VolatileVariableBase + sizeof (VARIABLE_STORE_HEADER)));
        Variable.EndPtr = (VARIABLE_HEADER *) GetEndPointer ((VARIABLE_STORE_HEADER *) ((UINTN) Global->VolatileVariableBase));
      } else {
        Status = EFI_NOT_FOUND;
        goto Done;
      }

      Variable.CurrPtr = Variable.StartPtr;
      if (Variable.CurrPtr->StartId != VARIABLE_DATA) {
        continue;
      }
    }
    //
    // Variable is found
    //
    if (Variable.CurrPtr->StartId == VARIABLE_DATA && Variable.CurrPtr->State == VAR_ADDED) {
      if (!(EfiAtRuntime () && ((Variable.CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0))) {
        VarNameSize = Variable.CurrPtr->NameSize;
        if (VarNameSize <= *VariableNameSize) {
          CopyMem (
            VariableName,
            GET_VARIABLE_NAME_PTR (Variable.CurrPtr),
            VarNameSize
            );
          CopyMem (
            VendorGuid,
            &Variable.CurrPtr->VendorGuid,
            sizeof (EFI_GUID)
            );
          Status = EFI_SUCCESS;
        } else {
          Status = EFI_BUFFER_TOO_SMALL;
        }

        *VariableNameSize = VarNameSize;
        goto Done;
      }
    }
  }

Done:
  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return Status;

}

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  @param  VariableName           A Null-terminated Unicode string that is the name of the vendor's
                                 variable.  Each VariableName is unique for each 
                                 VendorGuid.  VariableName must contain 1 or more 
                                 Unicode characters.  If VariableName is an empty Unicode 
                                 string, then EFI_INVALID_PARAMETER is returned.
  @param  VendorGuid             A unique identifier for the vendor
  @param  Attributes             Attributes bitmask to set for the variable
  @param  DataSize               The size in bytes of the Data buffer.  A size of zero causes the
                                 variable to be deleted.
  @param  Data                   The contents for the variable
  @param  Global                 Pointer to VARIABLE_GLOBAL structure
  @param  VolatileOffset         The offset of last volatile variable
  @param  NonVolatileOffset      The offset of last non-volatile variable

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data as 
                                 defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits was supplied, or the 
                                 DataSize exceeds the maximum allowed, or VariableName is an empty 
                                 Unicode string, or VendorGuid is NULL.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved due to a hardware failure.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only or cannot be deleted.
  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.

**/
EFI_STATUS
EFIAPI
EmuSetVariable (
  IN CHAR16                  *VariableName,
  IN EFI_GUID                *VendorGuid,
  IN UINT32                  Attributes,
  IN UINTN                   DataSize,
  IN VOID                    *Data,
  IN VARIABLE_GLOBAL         *Global,
  IN UINTN                   *VolatileOffset,
  IN UINTN                   *NonVolatileOffset
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  EFI_STATUS              Status;

  //
  // Check input parameters
  //
  if (VariableName == NULL || VariableName[0] == 0 || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }  
  //
  //  Make sure if runtime bit is set, boot service bit is set also
  //
  if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    return EFI_INVALID_PARAMETER;
  }
  //
  //  The size of the VariableName, including the Unicode Null in bytes plus
  //  the DataSize is limited to maximum size of FixedPcdGet32(PcdMaxHardwareErrorVariableSize)
  //  bytes for HwErrRec, and FixedPcdGet32(PcdMaxVariableSize) bytes for the others.
  //
  if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    if ((DataSize > FixedPcdGet32(PcdMaxHardwareErrorVariableSize)) ||                                                       
        (sizeof (VARIABLE_HEADER) + StrSize (VariableName) + DataSize > FixedPcdGet32(PcdMaxHardwareErrorVariableSize))) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // According to UEFI spec, HARDWARE_ERROR_RECORD variable name convention should be L"HwErrRecXXXX"
    //
    if (StrnCmp(VariableName, L"HwErrRec", StrLen(L"HwErrRec")) != 0) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
  //
  //  The size of the VariableName, including the Unicode Null in bytes plus
  //  the DataSize is limited to maximum size of FixedPcdGet32(PcdMaxVariableSize) bytes.
  //
    if ((DataSize > FixedPcdGet32(PcdMaxVariableSize)) ||
        (sizeof (VARIABLE_HEADER) + StrSize (VariableName) + DataSize > FixedPcdGet32(PcdMaxVariableSize))) {
      return EFI_INVALID_PARAMETER;
    }  
  }

  AcquireLockOnlyAtBootTime(&Global->VariableServicesLock);

  //
  // Check whether the input variable is already existed
  //
  
  Status = FindVariable (VariableName, VendorGuid, &Variable, Global);

  //
  // Hook the operation of setting PlatformLangCodes/PlatformLang and LangCodes/Lang
  //
  AutoUpdateLangVariable (VariableName, Data, DataSize);

  Status = UpdateVariable (VariableName, VendorGuid, Data, DataSize, Attributes, &Variable);

  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return Status;
}

/**

  This code returns information about the EFI variables.

  @param  Attributes                   Attributes bitmask to specify the type of variables
                                       on which to return information.
  @param  MaximumVariableStorageSize   On output the maximum size of the storage space available for 
                                       the EFI variables associated with the attributes specified.  
  @param  RemainingVariableStorageSize Returns the remaining size of the storage space available for EFI 
                                       variables associated with the attributes specified.
  @param  MaximumVariableSize          Returns the maximum size of an individual EFI variable 
                                       associated with the attributes specified.
  @param  Global                       Pointer to VARIABLE_GLOBAL structure.

  @retval EFI_SUCCESS                  Valid answer returned.
  @retval EFI_INVALID_PARAMETER        An invalid combination of attribute bits was supplied
  @retval EFI_UNSUPPORTED              The attribute is not supported on this platform, and the 
                                       MaximumVariableStorageSize, RemainingVariableStorageSize, 
                                       MaximumVariableSize are undefined.

**/
EFI_STATUS
EFIAPI
EmuQueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize,
  IN  VARIABLE_GLOBAL        *Global
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
  } else if (EfiAtRuntime () && ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0)) {
    //
    //   Make sure RT Attribute is set if we are in Runtime phase.
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    //
    // Make sure Hw Attribute is set with NV.
    //
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&Global->VariableServicesLock);

  if((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
    //
    // Query is Volatile related.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) ((UINTN) Global->VolatileVariableBase);
  } else {
    //
    // Query is Non-Volatile related.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) ((UINTN) Global->NonVolatileVariableBase);
  }

  //
  // Now let's fill *MaximumVariableStorageSize *RemainingVariableStorageSize
  // with the storage size (excluding the storage header size)
  //
  *MaximumVariableStorageSize   = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER);

  //
  // Harware error record variable needs larger size.
  //
  if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
    *MaximumVariableStorageSize = FixedPcdGet32(PcdHwErrStorageSize);
    *MaximumVariableSize = FixedPcdGet32(PcdMaxHardwareErrorVariableSize) - sizeof (VARIABLE_HEADER);
  } else {
    if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      ASSERT (FixedPcdGet32(PcdHwErrStorageSize) < VariableStoreHeader->Size);
      *MaximumVariableStorageSize = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER) - FixedPcdGet32(PcdHwErrStorageSize);
    }

    //
    // Let *MaximumVariableSize be FixedPcdGet32(PcdMaxVariableSize) with the exception of the variable header size.
    //
    *MaximumVariableSize = FixedPcdGet32(PcdMaxVariableSize) - sizeof (VARIABLE_HEADER);
  }

  //
  // Point to the starting address of the variables.
  //
  Variable = (VARIABLE_HEADER *) (VariableStoreHeader + 1);

  //
  // Now walk through the related variable store.
  //
  while (Variable < GetEndPointer (VariableStoreHeader)) {
    NextVariable = GetNextVariablePtr(Variable);
    if (NextVariable == NULL) {
      break;
    }
    VariableSize = (UINT64) (UINTN) NextVariable - (UINT64) (UINTN) Variable;

    if ((NextVariable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
      HwErrVariableTotalSize += VariableSize;
    } else {
      CommonVariableTotalSize += VariableSize;
    }

    //
    // Go to the next one.
    //
    Variable = NextVariable;
  }

  if ((Attributes  & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD){
    *RemainingVariableStorageSize = *MaximumVariableStorageSize - HwErrVariableTotalSize;
  } else {
    *RemainingVariableStorageSize = *MaximumVariableStorageSize - CommonVariableTotalSize;
  }

  if (*RemainingVariableStorageSize < sizeof (VARIABLE_HEADER)) {
    *MaximumVariableSize = 0;
  } else if ((*RemainingVariableStorageSize - sizeof (VARIABLE_HEADER)) < *MaximumVariableSize) {
    *MaximumVariableSize = *RemainingVariableStorageSize - sizeof (VARIABLE_HEADER);
  }
  
  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return EFI_SUCCESS;
}

/**
  Initializes variable store area.

  This function allocates memory space for variable store area and initializes its attributes.

  @param  VariableBase         Base of the variable store area created
  @param  LastVariableOffset   Size of VARIABLE_STORE_HEADER

**/
EFI_STATUS
InitializeVariableStore (
  OUT EFI_PHYSICAL_ADDRESS  *VariableBase,
  OUT UINTN                 *LastVariableOffset
  )
{
  VARIABLE_STORE_HEADER *VariableStore;

  //
  // Allocate memory for volatile variable store
  //
  VariableStore = (VARIABLE_STORE_HEADER *) AllocateRuntimePool (
                                              FixedPcdGet32(PcdVariableStoreSize)
                                              );
  if (NULL == VariableStore) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (VariableStore, FixedPcdGet32(PcdVariableStoreSize), 0xff);

  //
  // Variable Specific Data
  //
  *VariableBase             = (EFI_PHYSICAL_ADDRESS) (UINTN) VariableStore;
  *LastVariableOffset       = sizeof (VARIABLE_STORE_HEADER);

  CopyGuid (&VariableStore->Signature, &gEfiVariableGuid);
  VariableStore->Size       = FixedPcdGet32(PcdVariableStoreSize);
  VariableStore->Format     = VARIABLE_STORE_FORMATTED;
  VariableStore->State      = VARIABLE_STORE_HEALTHY;
  VariableStore->Reserved   = 0;
  VariableStore->Reserved1  = 0;

  return EFI_SUCCESS;
}

/**
  Initializes variable store area for non-volatile and volatile variable.

  This function allocates and initializes memory space for global context of ESAL
  variable service and variable store area for non-volatile and volatile variable.

  @param  ImageHandle           The Image handle of this driver.
  @param  SystemTable           The pointer of EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
EFIAPI
VariableCommonInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Allocate memory for mVariableModuleGlobal
  //
  mVariableModuleGlobal = (ESAL_VARIABLE_GLOBAL *) AllocateRuntimeZeroPool (
                                                    sizeof (ESAL_VARIABLE_GLOBAL)
                                                   );
  if (NULL == mVariableModuleGlobal) {
    return EFI_OUT_OF_RESOURCES;
  }

  EfiInitializeLock(&mVariableModuleGlobal->VariableGlobal[Physical].VariableServicesLock, TPL_NOTIFY);

  //
  // Intialize volatile variable store
  //
  Status = InitializeVariableStore (
            &mVariableModuleGlobal->VariableGlobal[Physical].VolatileVariableBase,
            &mVariableModuleGlobal->VolatileLastVariableOffset
            );

  if (EFI_ERROR (Status)) {
    FreePool(mVariableModuleGlobal);
    return Status;
  }
  //
  // Intialize non volatile variable store
  //
  Status = InitializeVariableStore (
            &mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase,
            &mVariableModuleGlobal->NonVolatileLastVariableOffset
            );

  return Status;
}
