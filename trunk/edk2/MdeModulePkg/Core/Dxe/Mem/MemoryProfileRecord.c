/** @file
  Support routines for UEFI memory profile.

  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeMain.h"
#include "Imem.h"

#define IS_UEFI_MEMORY_PROFILE_ENABLED ((PcdGet8 (PcdMemoryProfilePropertyMask) & BIT0) != 0)

typedef struct {
  UINT32                        Signature;
  MEMORY_PROFILE_CONTEXT        Context;
  LIST_ENTRY                    *DriverInfoList;
} MEMORY_PROFILE_CONTEXT_DATA;

typedef struct {
  UINT32                        Signature;
  MEMORY_PROFILE_DRIVER_INFO    DriverInfo;
  LIST_ENTRY                    *AllocInfoList;
  LIST_ENTRY                    Link;
} MEMORY_PROFILE_DRIVER_INFO_DATA;

typedef struct {
  UINT32                        Signature;
  MEMORY_PROFILE_ALLOC_INFO     AllocInfo;
  LIST_ENTRY                    Link;
} MEMORY_PROFILE_ALLOC_INFO_DATA;


GLOBAL_REMOVE_IF_UNREFERENCED LIST_ENTRY  mImageQueue = INITIALIZE_LIST_HEAD_VARIABLE (mImageQueue);
GLOBAL_REMOVE_IF_UNREFERENCED MEMORY_PROFILE_CONTEXT_DATA mMemoryProfileContext = {
  MEMORY_PROFILE_CONTEXT_SIGNATURE,
  {
    {
      MEMORY_PROFILE_CONTEXT_SIGNATURE,
      sizeof (MEMORY_PROFILE_CONTEXT),
      MEMORY_PROFILE_CONTEXT_REVISION
    },
    0,
    0,
    {0},
    {0},
    0,
    0,
    0
  },
  &mImageQueue,
};
GLOBAL_REMOVE_IF_UNREFERENCED MEMORY_PROFILE_CONTEXT_DATA *mMemoryProfileContextPtr = NULL;

BOOLEAN mMemoryProfileRecordingStatus = FALSE;

/**
  Get memory profile data.

  @param[in]      This              The EDKII_MEMORY_PROFILE_PROTOCOL instance.
  @param[in, out] ProfileSize       On entry, points to the size in bytes of the ProfileBuffer.
                                    On return, points to the size of the data returned in ProfileBuffer.
  @param[out]     ProfileBuffer     Profile buffer.
                      
  @return EFI_SUCCESS               Get the memory profile data successfully.
  @return EFI_BUFFER_TO_SMALL       The ProfileSize is too small for the resulting data. 
                                    ProfileSize is updated with the size required.

**/
EFI_STATUS
EFIAPI
ProfileProtocolGetData (
  IN     EDKII_MEMORY_PROFILE_PROTOCOL  *This,
  IN OUT UINT64                         *ProfileSize,
     OUT VOID                           *ProfileBuffer
  );

/**
  Register image to memory profile.

  @param[in] This               The EDKII_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] FilePath           File path of the image.
  @param[in] ImageBase          Image base address.
  @param[in] ImageSize          Image size.
  @param[in] FileType           File type of the image.

  @return EFI_SUCCESS           Register success.
  @return EFI_OUT_OF_RESOURCE   No enough resource for this register.

**/
EFI_STATUS
EFIAPI
ProfileProtocolRegisterImage (
  IN EDKII_MEMORY_PROFILE_PROTOCOL      *This,
  IN EFI_DEVICE_PATH_PROTOCOL           *FilePath,
  IN PHYSICAL_ADDRESS                   ImageBase,
  IN UINT64                             ImageSize,
  IN EFI_FV_FILETYPE                    FileType
  );

/**
  Unregister image from memory profile.

  @param[in] This               The EDKII_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] FilePath           File path of the image.
  @param[in] ImageBase          Image base address.
  @param[in] ImageSize          Image size.

  @return EFI_SUCCESS           Unregister success.
  @return EFI_NOT_FOUND         The image is not found.

**/
EFI_STATUS
EFIAPI
ProfileProtocolUnregisterImage (
  IN EDKII_MEMORY_PROFILE_PROTOCOL      *This,
  IN EFI_DEVICE_PATH_PROTOCOL           *FilePath,
  IN PHYSICAL_ADDRESS                   ImageBase,
  IN UINT64                             ImageSize
  );

EDKII_MEMORY_PROFILE_PROTOCOL mProfileProtocol = {
  ProfileProtocolGetData,
  ProfileProtocolRegisterImage,
  ProfileProtocolUnregisterImage
};

/**
  Return memory profile context.

  @return Memory profile context.

**/
MEMORY_PROFILE_CONTEXT_DATA *
GetMemoryProfileContext (
  VOID
  )
{
  return mMemoryProfileContextPtr;
}

/**
  Retrieves the magic value from the PE/COFF header.

  @param Hdr    The buffer in which to return the PE32, PE32+, or TE header.

  @return EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC - Image is PE32
  @return EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC - Image is PE32+

**/
UINT16
InternalPeCoffGetPeHeaderMagicValue (
  IN  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr
  )
{
  //
  // NOTE: Some versions of Linux ELILO for Itanium have an incorrect magic value
  //       in the PE/COFF Header.  If the MachineType is Itanium(IA64) and the
  //       Magic value in the OptionalHeader is  EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC
  //       then override the returned value to EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC
  //
  if (Hdr.Pe32->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64 && Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    return EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
  }
  //
  // Return the magic value from the PC/COFF Optional Header
  //
  return Hdr.Pe32->OptionalHeader.Magic;
}

/**
  Retrieves and returns the Subsystem of a PE/COFF image that has been loaded into system memory.
  If Pe32Data is NULL, then ASSERT().

  @param Pe32Data   The pointer to the PE/COFF image that is loaded in system memory.

  @return The Subsystem of the PE/COFF image.

**/
UINT16
InternalPeCoffGetSubsystem (
  IN VOID  *Pe32Data
  )
{
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  EFI_IMAGE_DOS_HEADER                 *DosHdr;
  UINT16                               Magic;

  ASSERT (Pe32Data != NULL);

  DosHdr = (EFI_IMAGE_DOS_HEADER *) Pe32Data;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present, so read the PE header after the DOS image header.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *) ((UINTN) Pe32Data + (UINTN) ((DosHdr->e_lfanew) & 0x0ffff));
  } else {
    //
    // DOS image header is not present, so PE header is at the image base.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *) Pe32Data;
  }

  if (Hdr.Te->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    return Hdr.Te->Subsystem;
  } else if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE)  {
    Magic = InternalPeCoffGetPeHeaderMagicValue (Hdr);
    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      return Hdr.Pe32->OptionalHeader.Subsystem;
    } else if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
      return Hdr.Pe32Plus->OptionalHeader.Subsystem;
    }
  }

  return 0x0000;
}

/**
  Retrieves and returns a pointer to the entry point to a PE/COFF image that has been loaded
  into system memory with the PE/COFF Loader Library functions.

  Retrieves the entry point to the PE/COFF image specified by Pe32Data and returns this entry
  point in EntryPoint.  If the entry point could not be retrieved from the PE/COFF image, then
  return RETURN_INVALID_PARAMETER.  Otherwise return RETURN_SUCCESS.
  If Pe32Data is NULL, then ASSERT().
  If EntryPoint is NULL, then ASSERT().

  @param  Pe32Data                  The pointer to the PE/COFF image that is loaded in system memory.
  @param  EntryPoint                The pointer to entry point to the PE/COFF image to return.

  @retval RETURN_SUCCESS            EntryPoint was returned.
  @retval RETURN_INVALID_PARAMETER  The entry point could not be found in the PE/COFF image.

**/
RETURN_STATUS
InternalPeCoffGetEntryPoint (
  IN  VOID  *Pe32Data,
  OUT VOID  **EntryPoint
  )
{
  EFI_IMAGE_DOS_HEADER                  *DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;

  ASSERT (Pe32Data   != NULL);
  ASSERT (EntryPoint != NULL);

  DosHdr = (EFI_IMAGE_DOS_HEADER *) Pe32Data;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present, so read the PE header after the DOS image header.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *) ((UINTN) Pe32Data + (UINTN) ((DosHdr->e_lfanew) & 0x0ffff));
  } else {
    //
    // DOS image header is not present, so PE header is at the image base.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *) Pe32Data;
  }

  //
  // Calculate the entry point relative to the start of the image.
  // AddressOfEntryPoint is common for PE32 & PE32+
  //
  if (Hdr.Te->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    *EntryPoint = (VOID *) ((UINTN) Pe32Data + (UINTN) (Hdr.Te->AddressOfEntryPoint & 0x0ffffffff) + sizeof (EFI_TE_IMAGE_HEADER) - Hdr.Te->StrippedSize);
    return RETURN_SUCCESS;
  } else if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE) {
    *EntryPoint = (VOID *) ((UINTN) Pe32Data + (UINTN) (Hdr.Pe32->OptionalHeader.AddressOfEntryPoint & 0x0ffffffff));
    return RETURN_SUCCESS;
  }

  return RETURN_UNSUPPORTED;
}

/**
  Build driver info.

  @param ContextData    Memory profile context.
  @param FileName       File name of the image.
  @param ImageBase      Image base address.
  @param ImageSize      Image size.
  @param EntryPoint     Entry point of the image.
  @param ImageSubsystem Image subsystem of the image.
  @param FileType       File type of the image.

  @return Pointer to memory profile driver info.

**/
MEMORY_PROFILE_DRIVER_INFO_DATA *
BuildDriverInfo (
  IN MEMORY_PROFILE_CONTEXT_DATA    *ContextData,
  IN EFI_GUID                       *FileName,
  IN PHYSICAL_ADDRESS               ImageBase,
  IN UINT64                         ImageSize,
  IN PHYSICAL_ADDRESS               EntryPoint,
  IN UINT16                         ImageSubsystem,
  IN EFI_FV_FILETYPE                FileType
  )
{
  EFI_STATUS                        Status;
  MEMORY_PROFILE_DRIVER_INFO        *DriverInfo;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  VOID                              *EntryPointInImage;

  //
  // Use CoreInternalAllocatePool() that will not update profile for this AllocatePool action.
  //
  Status = CoreInternalAllocatePool (
             EfiBootServicesData,
             sizeof (*DriverInfoData) + sizeof (LIST_ENTRY),
             (VOID **) &DriverInfoData
             );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  ZeroMem (DriverInfoData, sizeof (*DriverInfoData));

  DriverInfo = &DriverInfoData->DriverInfo;
  DriverInfoData->Signature = MEMORY_PROFILE_DRIVER_INFO_SIGNATURE;
  DriverInfo->Header.Signature = MEMORY_PROFILE_DRIVER_INFO_SIGNATURE;
  DriverInfo->Header.Length = sizeof (MEMORY_PROFILE_DRIVER_INFO);
  DriverInfo->Header.Revision = MEMORY_PROFILE_DRIVER_INFO_REVISION;
  if (FileName != NULL) {
    CopyMem (&DriverInfo->FileName, FileName, sizeof (EFI_GUID));
  }
  DriverInfo->ImageBase = ImageBase;
  DriverInfo->ImageSize = ImageSize;
  DriverInfo->EntryPoint = EntryPoint;
  DriverInfo->ImageSubsystem = ImageSubsystem;
  if ((EntryPoint != 0) && ((EntryPoint < ImageBase) || (EntryPoint >= (ImageBase + ImageSize)))) {
    //
    // If the EntryPoint is not in the range of image buffer, it should come from emulation environment.
    // So patch ImageBuffer here to align the EntryPoint.
    //
    Status = InternalPeCoffGetEntryPoint ((VOID *) (UINTN) ImageBase, &EntryPointInImage);
    ASSERT_EFI_ERROR (Status);
    DriverInfo->ImageBase = ImageBase + EntryPoint - (PHYSICAL_ADDRESS) (UINTN) EntryPointInImage;
  }
  DriverInfo->FileType = FileType;
  DriverInfoData->AllocInfoList = (LIST_ENTRY *) (DriverInfoData + 1);
  InitializeListHead (DriverInfoData->AllocInfoList);
  DriverInfo->CurrentUsage = 0;
  DriverInfo->PeakUsage = 0;
  DriverInfo->AllocRecordCount = 0;

  InsertTailList (ContextData->DriverInfoList, &DriverInfoData->Link);
  ContextData->Context.ImageCount ++;
  ContextData->Context.TotalImageSize += DriverInfo->ImageSize;

  return DriverInfoData;
}

/**
  Register DXE Core to memory profile.

  @param HobStart       The start address of the HOB.
  @param ContextData    Memory profile context.

  @retval TRUE      Register success.
  @retval FALSE     Register fail.

**/
BOOLEAN
RegisterDxeCore (
  IN VOID                           *HobStart,
  IN MEMORY_PROFILE_CONTEXT_DATA    *ContextData
  )
{
  EFI_PEI_HOB_POINTERS              DxeCoreHob;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  PHYSICAL_ADDRESS                  ImageBase;

  ASSERT (ContextData != NULL);

  //
  // Searching for image hob
  //
  DxeCoreHob.Raw          = HobStart;
  while ((DxeCoreHob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, DxeCoreHob.Raw)) != NULL) {
    if (CompareGuid (&DxeCoreHob.MemoryAllocationModule->MemoryAllocationHeader.Name, &gEfiHobMemoryAllocModuleGuid)) {
      //
      // Find Dxe Core HOB
      //
      break;
    }
    DxeCoreHob.Raw = GET_NEXT_HOB (DxeCoreHob);
  }
  ASSERT (DxeCoreHob.Raw != NULL);

  ImageBase = DxeCoreHob.MemoryAllocationModule->MemoryAllocationHeader.MemoryBaseAddress;
  DriverInfoData = BuildDriverInfo (
                     ContextData,
                     &DxeCoreHob.MemoryAllocationModule->ModuleName,
                     ImageBase,
                     DxeCoreHob.MemoryAllocationModule->MemoryAllocationHeader.MemoryLength,
                     DxeCoreHob.MemoryAllocationModule->EntryPoint,
                     InternalPeCoffGetSubsystem ((VOID *) (UINTN) ImageBase),
                     EFI_FV_FILETYPE_DXE_CORE
                     );
  if (DriverInfoData == NULL) {
    return FALSE;
  }

  return TRUE;
}

/**
  Initialize memory profile.

  @param HobStart   The start address of the HOB.

**/
VOID
MemoryProfileInit (
  IN VOID   *HobStart
  )
{
  MEMORY_PROFILE_CONTEXT_DATA   *ContextData;

  if (!IS_UEFI_MEMORY_PROFILE_ENABLED) {
    return;
  }

  ContextData = GetMemoryProfileContext ();
  if (ContextData != NULL) {
    return;
  }

  mMemoryProfileRecordingStatus = TRUE;
  mMemoryProfileContextPtr = &mMemoryProfileContext;

  RegisterDxeCore (HobStart, &mMemoryProfileContext);

  DEBUG ((EFI_D_INFO, "MemoryProfileInit MemoryProfileContext - 0x%x\n", &mMemoryProfileContext));
}

/**
  Install memory profile protocol.

**/
VOID
MemoryProfileInstallProtocol (
  VOID
  )
{
  EFI_HANDLE    Handle;
  EFI_STATUS    Status;

  if (!IS_UEFI_MEMORY_PROFILE_ENABLED) {
    return;
  }

  Handle = NULL;
  Status = CoreInstallMultipleProtocolInterfaces (
             &Handle,
             &gEdkiiMemoryProfileGuid,
             &mProfileProtocol,
             NULL
             );
  ASSERT_EFI_ERROR (Status);
}

/**
  Get the GUID file name from the file path.

  @param FilePath  File path.

  @return The GUID file name from the file path.

**/
EFI_GUID *
GetFileNameFromFilePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *FilePath
  )
{
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH     *ThisFilePath;
  EFI_GUID                              *FileName;

  FileName = NULL;
  if (FilePath != NULL) {
    ThisFilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) FilePath;
    while (!IsDevicePathEnd (ThisFilePath)) {
      FileName = EfiGetNameGuidFromFwVolDevicePathNode (ThisFilePath);
      if (FileName != NULL) {
        break;
      }
      ThisFilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) NextDevicePathNode (ThisFilePath);
    }
  }

  return FileName;
}

/**
  Register image to memory profile.

  @param DriverEntry    Image info.
  @param FileType       Image file type.

  @retval TRUE          Register success.
  @retval FALSE         Register fail.

**/
BOOLEAN
RegisterMemoryProfileImage (
  IN LOADED_IMAGE_PRIVATE_DATA  *DriverEntry,
  IN EFI_FV_FILETYPE            FileType
  )
{
  MEMORY_PROFILE_CONTEXT_DATA       *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;

  if (!IS_UEFI_MEMORY_PROFILE_ENABLED) {
    return FALSE;
  }

  ContextData = GetMemoryProfileContext ();
  if (ContextData == NULL) {
    return FALSE;
  }

  DriverInfoData = BuildDriverInfo (
                     ContextData,
                     GetFileNameFromFilePath (DriverEntry->Info.FilePath),
                     DriverEntry->ImageContext.ImageAddress,
                     DriverEntry->ImageContext.ImageSize,
                     DriverEntry->ImageContext.EntryPoint,
                     DriverEntry->ImageContext.ImageType,
                     FileType
                     );
  if (DriverInfoData == NULL) {
    return FALSE;
  }

  return TRUE;
}

/**
  Search image from memory profile.

  @param ContextData    Memory profile context.
  @param FileName       Image file name.
  @param Address        Image Address.

  @return Pointer to memory profile driver info.

**/
MEMORY_PROFILE_DRIVER_INFO_DATA *
GetMemoryProfileDriverInfoByFileNameAndAddress (
  IN MEMORY_PROFILE_CONTEXT_DATA    *ContextData,
  IN EFI_GUID                       *FileName,
  IN PHYSICAL_ADDRESS               Address
  )
{
  MEMORY_PROFILE_DRIVER_INFO        *DriverInfo;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  LIST_ENTRY                        *DriverLink;
  LIST_ENTRY                        *DriverInfoList;

  DriverInfoList = ContextData->DriverInfoList;

  for (DriverLink = DriverInfoList->ForwardLink;
       DriverLink != DriverInfoList;
       DriverLink = DriverLink->ForwardLink) {
    DriverInfoData = CR (
                       DriverLink,
                       MEMORY_PROFILE_DRIVER_INFO_DATA,
                       Link,
                       MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                       );
    DriverInfo = &DriverInfoData->DriverInfo;
    if ((CompareGuid (&DriverInfo->FileName, FileName)) &&
        (Address >= DriverInfo->ImageBase) &&
        (Address < (DriverInfo->ImageBase + DriverInfo->ImageSize))) {
      return DriverInfoData;
    }
  }

  return NULL;
}

/**
  Search dummy image from memory profile.

  @param ContextData    Memory profile context.

  @return Pointer to memory profile driver info.

**/
MEMORY_PROFILE_DRIVER_INFO_DATA *
FindDummyImage (
  IN MEMORY_PROFILE_CONTEXT_DATA    *ContextData
  )
{
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  LIST_ENTRY                        *DriverLink;
  LIST_ENTRY                        *DriverInfoList;

  DriverInfoList = ContextData->DriverInfoList;

  for (DriverLink = DriverInfoList->ForwardLink;
       DriverLink != DriverInfoList;
       DriverLink = DriverLink->ForwardLink) {
    DriverInfoData = CR (
                   DriverLink,
                   MEMORY_PROFILE_DRIVER_INFO_DATA,
                   Link,
                   MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                   );
    if (CompareGuid (&gZeroGuid, &DriverInfoData->DriverInfo.FileName)) {
      return DriverInfoData;
    }
  }

  return BuildDriverInfo (ContextData, &gZeroGuid, 0, 0, 0, 0, 0);
}

/**
  Search image from memory profile.
  It will return image, if (Address >= ImageBuffer) AND (Address < ImageBuffer + ImageSize)

  @param ContextData    Memory profile context.
  @param Address        Image or Function address.

  @return Pointer to memory profile driver info.

**/
MEMORY_PROFILE_DRIVER_INFO_DATA *
GetMemoryProfileDriverInfoFromAddress (
  IN MEMORY_PROFILE_CONTEXT_DATA    *ContextData,
  IN PHYSICAL_ADDRESS               Address
  )
{
  MEMORY_PROFILE_DRIVER_INFO        *DriverInfo;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  LIST_ENTRY                        *DriverLink;
  LIST_ENTRY                        *DriverInfoList;

  DriverInfoList = ContextData->DriverInfoList;

  for (DriverLink = DriverInfoList->ForwardLink;
       DriverLink != DriverInfoList;
       DriverLink = DriverLink->ForwardLink) {
    DriverInfoData = CR (
                       DriverLink,
                       MEMORY_PROFILE_DRIVER_INFO_DATA,
                       Link,
                       MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                       );
    DriverInfo = &DriverInfoData->DriverInfo;
    if ((Address >= DriverInfo->ImageBase) &&
        (Address < (DriverInfo->ImageBase + DriverInfo->ImageSize))) {
      return DriverInfoData;
    }
  }

  //
  // Should never come here.
  //
  return FindDummyImage (ContextData);
}

/**
  Unregister image from memory profile.

  @param DriverEntry    Image info.

  @retval TRUE          Unregister success.
  @retval FALSE         Unregister fail.

**/
BOOLEAN
UnregisterMemoryProfileImage (
  IN LOADED_IMAGE_PRIVATE_DATA      *DriverEntry
  )
{
  EFI_STATUS                        Status;
  MEMORY_PROFILE_CONTEXT_DATA       *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  EFI_GUID                          *FileName;
  PHYSICAL_ADDRESS                  ImageAddress;
  VOID                              *EntryPointInImage;

  if (!IS_UEFI_MEMORY_PROFILE_ENABLED) {
    return FALSE;
  }

  ContextData = GetMemoryProfileContext ();
  if (ContextData == NULL) {
    return FALSE;
  }

  DriverInfoData = NULL;
  FileName = GetFileNameFromFilePath (DriverEntry->Info.FilePath);
  ImageAddress = DriverEntry->ImageContext.ImageAddress;
  if ((DriverEntry->ImageContext.EntryPoint < ImageAddress) || (DriverEntry->ImageContext.EntryPoint >= (ImageAddress + DriverEntry->ImageContext.ImageSize))) {
    //
    // If the EntryPoint is not in the range of image buffer, it should come from emulation environment.
    // So patch ImageAddress here to align the EntryPoint.
    //
    Status = InternalPeCoffGetEntryPoint ((VOID *) (UINTN) ImageAddress, &EntryPointInImage);
    ASSERT_EFI_ERROR (Status);
    ImageAddress = ImageAddress + (UINTN) DriverEntry->ImageContext.EntryPoint - (UINTN) EntryPointInImage;
  }
  if (FileName != NULL) {
    DriverInfoData = GetMemoryProfileDriverInfoByFileNameAndAddress (ContextData, FileName, ImageAddress);
  }
  if (DriverInfoData == NULL) {
    DriverInfoData = GetMemoryProfileDriverInfoFromAddress (ContextData, ImageAddress);
  }
  if (DriverInfoData == NULL) {
    return FALSE;
  }

  ContextData->Context.TotalImageSize -= DriverInfoData->DriverInfo.ImageSize;

  DriverInfoData->DriverInfo.ImageBase = 0;
  DriverInfoData->DriverInfo.ImageSize = 0;

  if (DriverInfoData->DriverInfo.PeakUsage == 0) {
    ContextData->Context.ImageCount --;
    RemoveEntryList (&DriverInfoData->Link);
    //
    // Use CoreInternalFreePool() that will not update profile for this FreePool action.
    //
    CoreInternalFreePool (DriverInfoData, NULL);
  }

  return TRUE;
}

/**
  Return if this memory type needs to be recorded into memory profile.
  If BIOS memory type (0 ~ EfiMaxMemoryType - 1), it checks bit (1 << MemoryType).
  If OS memory type (0x80000000 ~ 0xFFFFFFFF), it checks bit63 - 0x8000000000000000.
  If OEM memory type (0x70000000 ~ 0x7FFFFFFF), it checks bit62 - 0x4000000000000000.

  @param MemoryType     Memory type.

  @retval TRUE          This memory type need to be recorded.
  @retval FALSE         This memory type need not to be recorded.

**/
BOOLEAN
CoreNeedRecordProfile (
  IN EFI_MEMORY_TYPE    MemoryType
  )
{
  UINT64 TestBit;

  if ((UINT32) MemoryType >= MEMORY_TYPE_OS_RESERVED_MIN) {
    TestBit = BIT63;
  } else if ((UINT32) MemoryType >= MEMORY_TYPE_OEM_RESERVED_MIN) {
    TestBit = BIT62;
  } else {
    TestBit = LShiftU64 (1, MemoryType);
  }

  if ((PcdGet64 (PcdMemoryProfileMemoryType) & TestBit) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Convert EFI memory type to profile memory index. The rule is:
  If BIOS memory type (0 ~ EfiMaxMemoryType - 1), ProfileMemoryIndex = MemoryType.
  If OS memory type (0x80000000 ~ 0xFFFFFFFF), ProfileMemoryIndex = EfiMaxMemoryType.
  If OEM memory type (0x70000000 ~ 0x7FFFFFFF), ProfileMemoryIndex = EfiMaxMemoryType + 1.

  @param MemoryType     Memory type.

  @return Profile memory index.

**/
UINTN
GetProfileMemoryIndex (
  IN EFI_MEMORY_TYPE    MemoryType
  )
{
  if ((UINT32) MemoryType >= MEMORY_TYPE_OS_RESERVED_MIN) {
    return EfiMaxMemoryType;
  } else if ((UINT32) MemoryType >= MEMORY_TYPE_OEM_RESERVED_MIN) {
    return EfiMaxMemoryType + 1;
  } else {
    return MemoryType;
  }
}

/**
  Update memory profile Allocate information.

  @param CallerAddress  Address of caller who call Allocate.
  @param Action         This Allocate action.
  @param MemoryType     Memory type.
  @param Size           Buffer size.
  @param Buffer         Buffer address.

  @retval TRUE          Profile udpate success.
  @retval FALSE         Profile update fail.

**/
BOOLEAN
CoreUpdateProfileAllocate (
  IN PHYSICAL_ADDRESS       CallerAddress,
  IN MEMORY_PROFILE_ACTION  Action,
  IN EFI_MEMORY_TYPE        MemoryType,
  IN UINTN                  Size,
  IN VOID                   *Buffer
  )
{
  EFI_STATUS                        Status;
  MEMORY_PROFILE_CONTEXT           *Context;
  MEMORY_PROFILE_DRIVER_INFO       *DriverInfo;
  MEMORY_PROFILE_ALLOC_INFO        *AllocInfo;
  MEMORY_PROFILE_CONTEXT_DATA       *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  MEMORY_PROFILE_ALLOC_INFO_DATA    *AllocInfoData;
  UINTN                             ProfileMemoryIndex;

  AllocInfoData = NULL;

  ContextData = GetMemoryProfileContext ();
  if (ContextData == NULL) {
    return FALSE;
  }

  DriverInfoData = GetMemoryProfileDriverInfoFromAddress (ContextData, CallerAddress);
  ASSERT (DriverInfoData != NULL);

  //
  // Use CoreInternalAllocatePool() that will not update profile for this AllocatePool action.
  //
  Status = CoreInternalAllocatePool (
             EfiBootServicesData,
             sizeof (*AllocInfoData),
             (VOID **) &AllocInfoData
             );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  ASSERT (AllocInfoData != NULL);
  AllocInfo = &AllocInfoData->AllocInfo;
  AllocInfoData->Signature      = MEMORY_PROFILE_ALLOC_INFO_SIGNATURE;
  AllocInfo->Header.Signature   = MEMORY_PROFILE_ALLOC_INFO_SIGNATURE;
  AllocInfo->Header.Length      = sizeof (MEMORY_PROFILE_ALLOC_INFO);
  AllocInfo->Header.Revision    = MEMORY_PROFILE_ALLOC_INFO_REVISION;
  AllocInfo->CallerAddress      = CallerAddress;
  AllocInfo->SequenceId         = ContextData->Context.SequenceCount;
  AllocInfo->Action             = Action;
  AllocInfo->MemoryType         = MemoryType;
  AllocInfo->Buffer             = (PHYSICAL_ADDRESS) (UINTN) Buffer;
  AllocInfo->Size               = Size;

  InsertTailList (DriverInfoData->AllocInfoList, &AllocInfoData->Link);

  ProfileMemoryIndex = GetProfileMemoryIndex (MemoryType);

  DriverInfo = &DriverInfoData->DriverInfo;
  DriverInfo->CurrentUsage += Size;
  if (DriverInfo->PeakUsage < DriverInfo->CurrentUsage) {
    DriverInfo->PeakUsage = DriverInfo->CurrentUsage;
  }
  DriverInfo->CurrentUsageByType[ProfileMemoryIndex] += Size;
  if (DriverInfo->PeakUsageByType[ProfileMemoryIndex] < DriverInfo->CurrentUsageByType[ProfileMemoryIndex]) {
    DriverInfo->PeakUsageByType[ProfileMemoryIndex] = DriverInfo->CurrentUsageByType[ProfileMemoryIndex];
  }
  DriverInfo->AllocRecordCount ++;

  Context = &ContextData->Context;
  Context->CurrentTotalUsage += Size;
  if (Context->PeakTotalUsage < Context->CurrentTotalUsage) {
    Context->PeakTotalUsage = Context->CurrentTotalUsage;
  }
  Context->CurrentTotalUsageByType[ProfileMemoryIndex] += Size;
  if (Context->PeakTotalUsageByType[ProfileMemoryIndex] < Context->CurrentTotalUsageByType[ProfileMemoryIndex]) {
    Context->PeakTotalUsageByType[ProfileMemoryIndex] = Context->CurrentTotalUsageByType[ProfileMemoryIndex];
  }
  Context->SequenceCount ++;

  return TRUE;
}

/**
  Get memory profile alloc info from memory profile

  @param DriverInfoData     Driver info
  @param Action             This Free action
  @param Size               Buffer size
  @param Buffer             Buffer address

  @return Pointer to memory profile alloc info.
**/
MEMORY_PROFILE_ALLOC_INFO_DATA *
GetMemoryProfileAllocInfoFromAddress (
  IN MEMORY_PROFILE_DRIVER_INFO_DATA    *DriverInfoData,
  IN MEMORY_PROFILE_ACTION              Action,
  IN UINTN                              Size,
  IN VOID                               *Buffer
  )
{
  LIST_ENTRY                        *AllocInfoList;
  LIST_ENTRY                        *AllocLink;
  MEMORY_PROFILE_ALLOC_INFO         *AllocInfo;
  MEMORY_PROFILE_ALLOC_INFO_DATA    *AllocInfoData;

  AllocInfoList = DriverInfoData->AllocInfoList;

  for (AllocLink = AllocInfoList->ForwardLink;
       AllocLink != AllocInfoList;
       AllocLink = AllocLink->ForwardLink) {
    AllocInfoData = CR (
                      AllocLink,
                      MEMORY_PROFILE_ALLOC_INFO_DATA,
                      Link,
                      MEMORY_PROFILE_ALLOC_INFO_SIGNATURE
                      );
    AllocInfo = &AllocInfoData->AllocInfo;
    if (AllocInfo->Action != Action) {
      continue;
    }
    switch (Action) {
      case MemoryProfileActionAllocatePages:
        if ((AllocInfo->Buffer <= (PHYSICAL_ADDRESS) (UINTN) Buffer) &&
            ((AllocInfo->Buffer + AllocInfo->Size) >= ((PHYSICAL_ADDRESS) (UINTN) Buffer + Size))) {
          return AllocInfoData;
        }
        break;
      case MemoryProfileActionAllocatePool:
        if (AllocInfo->Buffer == (PHYSICAL_ADDRESS) (UINTN) Buffer) {
          return AllocInfoData;
        }
        break;
      default:
        ASSERT (FALSE);
        break;
    }
  }

  return NULL;
}

/**
  Update memory profile Free information.

  @param CallerAddress  Address of caller who call Free.
  @param Action         This Free action.
  @param Size           Buffer size.
  @param Buffer         Buffer address.

  @retval TRUE          Profile udpate success.
  @retval FALSE         Profile update fail.

**/
BOOLEAN
CoreUpdateProfileFree (
  IN PHYSICAL_ADDRESS       CallerAddress,
  IN MEMORY_PROFILE_ACTION  Action,
  IN UINTN                  Size,
  IN VOID                   *Buffer
  )
{
  MEMORY_PROFILE_CONTEXT           *Context;
  MEMORY_PROFILE_DRIVER_INFO       *DriverInfo;
  MEMORY_PROFILE_ALLOC_INFO        *AllocInfo;
  MEMORY_PROFILE_CONTEXT_DATA      *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA  *DriverInfoData;
  LIST_ENTRY                       *DriverLink;
  LIST_ENTRY                       *DriverInfoList;
  MEMORY_PROFILE_DRIVER_INFO_DATA  *ThisDriverInfoData;
  MEMORY_PROFILE_ALLOC_INFO_DATA   *AllocInfoData;
  UINTN                            ProfileMemoryIndex;

  ContextData = GetMemoryProfileContext ();
  if (ContextData == NULL) {
    return FALSE;
  }

  DriverInfoData = GetMemoryProfileDriverInfoFromAddress (ContextData, CallerAddress);
  ASSERT (DriverInfoData != NULL);

  switch (Action) {
    case MemoryProfileActionFreePages:
      AllocInfoData = GetMemoryProfileAllocInfoFromAddress (DriverInfoData, MemoryProfileActionAllocatePages, Size, Buffer);
      break;
    case MemoryProfileActionFreePool:
      AllocInfoData = GetMemoryProfileAllocInfoFromAddress (DriverInfoData, MemoryProfileActionAllocatePool, 0, Buffer);
      break;
    default:
      ASSERT (FALSE);
      AllocInfoData = NULL;
      break;
  }
  if (AllocInfoData == NULL) {
    //
    // Legal case, because driver A might free memory allocated by driver B, by some protocol.
    //
    DriverInfoList = ContextData->DriverInfoList;

    for (DriverLink = DriverInfoList->ForwardLink;
         DriverLink != DriverInfoList;
         DriverLink = DriverLink->ForwardLink) {
      ThisDriverInfoData = CR (
                             DriverLink,
                             MEMORY_PROFILE_DRIVER_INFO_DATA,
                             Link,
                             MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                             );
      switch (Action) {
        case MemoryProfileActionFreePages:
          AllocInfoData = GetMemoryProfileAllocInfoFromAddress (ThisDriverInfoData, MemoryProfileActionAllocatePages, Size, Buffer);
          break;
        case MemoryProfileActionFreePool:
          AllocInfoData = GetMemoryProfileAllocInfoFromAddress (ThisDriverInfoData, MemoryProfileActionAllocatePool, 0, Buffer);
          break;
        default:
          ASSERT (FALSE);
          AllocInfoData = NULL;
          break;
      }
      if (AllocInfoData != NULL) {
        DriverInfoData = ThisDriverInfoData;
        break;
      }
    }

    if (AllocInfoData == NULL) {
      //
      // No matched allocate operation is found for this free operation.
      // It is because the specified memory type allocate operation has been
      // filtered by CoreNeedRecordProfile(), but free operations have no
      // memory type information, they can not be filtered by CoreNeedRecordProfile().
      // Then, they will be filtered here.
      //
      return FALSE;
    }
  }

  Context = &ContextData->Context;
  DriverInfo = &DriverInfoData->DriverInfo;
  AllocInfo = &AllocInfoData->AllocInfo;

  ProfileMemoryIndex = GetProfileMemoryIndex (AllocInfo->MemoryType);

  Context->CurrentTotalUsage -= AllocInfo->Size;
  Context->CurrentTotalUsageByType[ProfileMemoryIndex] -= AllocInfo->Size;

  DriverInfo->CurrentUsage -= AllocInfo->Size;
  DriverInfo->CurrentUsageByType[ProfileMemoryIndex] -= AllocInfo->Size;
  DriverInfo->AllocRecordCount --;

  RemoveEntryList (&AllocInfoData->Link);

  if (Action == MemoryProfileActionFreePages) {
    if (AllocInfo->Buffer != (PHYSICAL_ADDRESS) (UINTN) Buffer) {
      CoreUpdateProfileAllocate (
        AllocInfo->CallerAddress,
        MemoryProfileActionAllocatePages,
        AllocInfo->MemoryType,
        (UINTN) ((PHYSICAL_ADDRESS) (UINTN) Buffer - AllocInfo->Buffer),
        (VOID *) (UINTN) AllocInfo->Buffer
        );
    }
    if (AllocInfo->Buffer + AllocInfo->Size != ((PHYSICAL_ADDRESS) (UINTN) Buffer + Size)) {
      CoreUpdateProfileAllocate (
        AllocInfo->CallerAddress,
        MemoryProfileActionAllocatePages,
        AllocInfo->MemoryType,
        (UINTN) ((AllocInfo->Buffer + AllocInfo->Size) - ((PHYSICAL_ADDRESS) (UINTN) Buffer + Size)),
        (VOID *) ((UINTN) Buffer + Size)
        );
    }
  }

  //
  // Use CoreInternalFreePool() that will not update profile for this FreePool action.
  //
  CoreInternalFreePool (AllocInfoData, NULL);

  return TRUE;
}

/**
  Update memory profile information.

  @param CallerAddress  Address of caller who call Allocate or Free.
  @param Action         This Allocate or Free action.
  @param MemoryType     Memory type.
  @param Size           Buffer size.
  @param Buffer         Buffer address.

  @retval TRUE          Profile udpate success.
  @retval FALSE         Profile update fail.

**/
BOOLEAN
CoreUpdateProfile (
  IN PHYSICAL_ADDRESS       CallerAddress,
  IN MEMORY_PROFILE_ACTION  Action,
  IN EFI_MEMORY_TYPE        MemoryType, // Valid for AllocatePages/AllocatePool
  IN UINTN                  Size,       // Valid for AllocatePages/FreePages/AllocatePool
  IN VOID                   *Buffer
  )
{
  MEMORY_PROFILE_CONTEXT_DATA   *ContextData;

  if (!IS_UEFI_MEMORY_PROFILE_ENABLED) {
    return FALSE;
  }

  if (!mMemoryProfileRecordingStatus) {
    return FALSE;
  }

  //
  // Free operations have no memory type information, so skip the check.
  //
  if ((Action == MemoryProfileActionAllocatePages) || (Action == MemoryProfileActionAllocatePool)) {
    //
    // Only record limited MemoryType.
    //
    if (!CoreNeedRecordProfile (MemoryType)) {
      return FALSE;
    }
  }

  ContextData = GetMemoryProfileContext ();
  if (ContextData == NULL) {
    return FALSE;
  }

  switch (Action) {
    case MemoryProfileActionAllocatePages:
      CoreUpdateProfileAllocate (CallerAddress, Action, MemoryType, Size, Buffer);
      break;
    case MemoryProfileActionFreePages:
      CoreUpdateProfileFree (CallerAddress, Action, Size, Buffer);
      break;
    case MemoryProfileActionAllocatePool:
      CoreUpdateProfileAllocate (CallerAddress, Action, MemoryType, Size, Buffer);
      break;
    case MemoryProfileActionFreePool:
      CoreUpdateProfileFree (CallerAddress, Action, 0, Buffer);
      break;
    default:
      ASSERT (FALSE);
      break;
  }
  return TRUE;
}

////////////////////

/**
  Get memory profile data size.

  @return Memory profile data size.

**/
UINTN
MemoryProfileGetDataSize (
  VOID
  )
{
  MEMORY_PROFILE_CONTEXT_DATA       *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  LIST_ENTRY                        *DriverInfoList;
  LIST_ENTRY                        *DriverLink;
  UINTN                             TotalSize;


  ContextData = GetMemoryProfileContext ();
  if (ContextData == NULL) {
    return 0;
  }

  TotalSize = sizeof (MEMORY_PROFILE_CONTEXT);
  TotalSize += sizeof (MEMORY_PROFILE_DRIVER_INFO) * (UINTN) ContextData->Context.ImageCount;

  DriverInfoList = ContextData->DriverInfoList;
  for (DriverLink = DriverInfoList->ForwardLink;
       DriverLink != DriverInfoList;
       DriverLink = DriverLink->ForwardLink) {
    DriverInfoData = CR (
                       DriverLink,
                       MEMORY_PROFILE_DRIVER_INFO_DATA,
                       Link,
                       MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                       );
    TotalSize += sizeof (MEMORY_PROFILE_ALLOC_INFO) * (UINTN) DriverInfoData->DriverInfo.AllocRecordCount;
  }

  return TotalSize;
}

/**
  Copy memory profile data.

  @param ProfileBuffer  The buffer to hold memory profile data.

**/
VOID
MemoryProfileCopyData (
  IN VOID   *ProfileBuffer
  )
{
  MEMORY_PROFILE_CONTEXT            *Context;
  MEMORY_PROFILE_DRIVER_INFO        *DriverInfo;
  MEMORY_PROFILE_ALLOC_INFO         *AllocInfo;
  MEMORY_PROFILE_CONTEXT_DATA       *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  MEMORY_PROFILE_ALLOC_INFO_DATA    *AllocInfoData;
  LIST_ENTRY                        *DriverInfoList;
  LIST_ENTRY                        *DriverLink;
  LIST_ENTRY                        *AllocInfoList;
  LIST_ENTRY                        *AllocLink;

  ContextData = GetMemoryProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  Context = ProfileBuffer;
  CopyMem (Context, &ContextData->Context, sizeof (MEMORY_PROFILE_CONTEXT));
  DriverInfo = (MEMORY_PROFILE_DRIVER_INFO *) (Context + 1);

  DriverInfoList = ContextData->DriverInfoList;
  for (DriverLink = DriverInfoList->ForwardLink;
       DriverLink != DriverInfoList;
       DriverLink = DriverLink->ForwardLink) {
    DriverInfoData = CR (
                       DriverLink,
                       MEMORY_PROFILE_DRIVER_INFO_DATA,
                       Link,
                       MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                       );
    CopyMem (DriverInfo, &DriverInfoData->DriverInfo, sizeof (MEMORY_PROFILE_DRIVER_INFO));
    AllocInfo = (MEMORY_PROFILE_ALLOC_INFO *) (DriverInfo + 1);

    AllocInfoList = DriverInfoData->AllocInfoList;
    for (AllocLink = AllocInfoList->ForwardLink;
         AllocLink != AllocInfoList;
         AllocLink = AllocLink->ForwardLink) {
      AllocInfoData = CR (
                        AllocLink,
                        MEMORY_PROFILE_ALLOC_INFO_DATA,
                        Link,
                        MEMORY_PROFILE_ALLOC_INFO_SIGNATURE
                        );
      CopyMem (AllocInfo, &AllocInfoData->AllocInfo, sizeof (MEMORY_PROFILE_ALLOC_INFO));
      AllocInfo += 1;
    }

    DriverInfo = (MEMORY_PROFILE_DRIVER_INFO *) ((UINTN) (DriverInfo + 1) + sizeof (MEMORY_PROFILE_ALLOC_INFO) * (UINTN) DriverInfo->AllocRecordCount);
  }
}

/**
  Get memory profile data.

  @param[in]      This              The EDKII_MEMORY_PROFILE_PROTOCOL instance.
  @param[in, out] ProfileSize       On entry, points to the size in bytes of the ProfileBuffer.
                                    On return, points to the size of the data returned in ProfileBuffer.
  @param[out]     ProfileBuffer     Profile buffer.
                      
  @return EFI_SUCCESS               Get the memory profile data successfully.
  @return EFI_BUFFER_TO_SMALL       The ProfileSize is too small for the resulting data. 
                                    ProfileSize is updated with the size required.

**/
EFI_STATUS
EFIAPI
ProfileProtocolGetData (
  IN     EDKII_MEMORY_PROFILE_PROTOCOL  *This,
  IN OUT UINT64                         *ProfileSize,
     OUT VOID                           *ProfileBuffer
  )
{
  UINTN                                 Size;
  MEMORY_PROFILE_CONTEXT_DATA           *ContextData;
  BOOLEAN                               MemoryProfileRecordingStatus;

  ContextData = GetMemoryProfileContext ();
  if (ContextData == NULL) {
    return EFI_UNSUPPORTED;
  }

  MemoryProfileRecordingStatus = mMemoryProfileRecordingStatus;
  mMemoryProfileRecordingStatus = FALSE;

  Size = MemoryProfileGetDataSize ();

  if (*ProfileSize < Size) {
    *ProfileSize = Size;
    mMemoryProfileRecordingStatus = MemoryProfileRecordingStatus;
    return EFI_BUFFER_TOO_SMALL;
  }

  *ProfileSize = Size;
  MemoryProfileCopyData (ProfileBuffer);

  mMemoryProfileRecordingStatus = MemoryProfileRecordingStatus;
  return EFI_SUCCESS;
}

/**
  Register image to memory profile.

  @param[in] This               The EDKII_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] FilePath           File path of the image.
  @param[in] ImageBase          Image base address.
  @param[in] ImageSize          Image size.
  @param[in] FileType           File type of the image.

  @return EFI_SUCCESS           Register success.
  @return EFI_OUT_OF_RESOURCE   No enough resource for this register.

**/
EFI_STATUS
EFIAPI
ProfileProtocolRegisterImage (
  IN EDKII_MEMORY_PROFILE_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL       *FilePath,
  IN PHYSICAL_ADDRESS               ImageBase,
  IN UINT64                         ImageSize,
  IN EFI_FV_FILETYPE                FileType
  )
{
  EFI_STATUS                        Status;
  LOADED_IMAGE_PRIVATE_DATA         DriverEntry;
  VOID                              *EntryPointInImage;

  ZeroMem (&DriverEntry, sizeof (DriverEntry));
  DriverEntry.Info.FilePath = FilePath;
  DriverEntry.ImageContext.ImageAddress = ImageBase;
  DriverEntry.ImageContext.ImageSize = ImageSize;
  Status = InternalPeCoffGetEntryPoint ((VOID *) (UINTN) ImageBase, &EntryPointInImage);
  ASSERT_EFI_ERROR (Status);
  DriverEntry.ImageContext.EntryPoint = (PHYSICAL_ADDRESS) (UINTN) EntryPointInImage;
  DriverEntry.ImageContext.ImageType = InternalPeCoffGetSubsystem ((VOID *) (UINTN) ImageBase);

  return RegisterMemoryProfileImage (&DriverEntry, FileType) ? EFI_SUCCESS: EFI_OUT_OF_RESOURCES;
}

/**
  Unregister image from memory profile.

  @param[in] This               The EDKII_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] FilePath           File path of the image.
  @param[in] ImageBase          Image base address.
  @param[in] ImageSize          Image size.

  @return EFI_SUCCESS           Unregister success.
  @return EFI_NOT_FOUND         The image is not found.

**/
EFI_STATUS
EFIAPI
ProfileProtocolUnregisterImage (
  IN EDKII_MEMORY_PROFILE_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL       *FilePath,
  IN PHYSICAL_ADDRESS               ImageBase,
  IN UINT64                         ImageSize
  )
{
  EFI_STATUS                        Status;
  LOADED_IMAGE_PRIVATE_DATA         DriverEntry;
  VOID                              *EntryPointInImage;

  ZeroMem (&DriverEntry, sizeof (DriverEntry));
  DriverEntry.Info.FilePath = FilePath;
  DriverEntry.ImageContext.ImageAddress = ImageBase;
  DriverEntry.ImageContext.ImageSize = ImageSize;
  Status = InternalPeCoffGetEntryPoint ((VOID *) (UINTN) ImageBase, &EntryPointInImage);
  ASSERT_EFI_ERROR (Status);
  DriverEntry.ImageContext.EntryPoint = (PHYSICAL_ADDRESS) (UINTN) EntryPointInImage;

  return UnregisterMemoryProfileImage (&DriverEntry) ? EFI_SUCCESS: EFI_NOT_FOUND;
}

////////////////////
