/** @file
Private functions used by PCD DXE driver.

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name: Service.h

**/

#ifndef _SERVICE_H
#define _SERVICE_H

#include <PiDxe.h>
#include <Protocol/Pcd.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

//
// Please make sure the PCD Serivce PEIM Version is consistent with
// the version of PCD Database generation tool
//
#define PCD_SERVICE_DXE_DRIVER_VERSION      2

//
// PCD_DXE_DATABASE_GENTOOL_VERSION is defined in Autogen.h
// and generated by PCD Database generation tool.
//
#if (PCD_SERVICE_DXE_DRIVER_VERSION != PCD_DXE_SERVICE_DRIVER_AUTOGEN_VERSION)
//  #error "Please make sure the version of PCD Service DXE Driver and PCD DXE Database Generation Tool matches"
#endif

//
// Protocol Interface function declaration.
//
VOID
EFIAPI
DxePcdSetSku (
  IN  UINTN                  SkuId
  )
;


UINT8
EFIAPI
DxePcdGet8 (
  IN UINTN             TokenNumber
  )
;


UINT16
EFIAPI
DxePcdGet16 (
  IN UINTN             TokenNumber
  )
;


UINT32
EFIAPI
DxePcdGet32 (
  IN UINTN             TokenNumber
  )
;


UINT64
EFIAPI
DxePcdGet64 (
  IN UINTN             TokenNumber
  )
;


VOID *
EFIAPI
DxePcdGetPtr (
  IN UINTN             TokenNumber
  )
;


BOOLEAN
EFIAPI
DxePcdGetBool (
  IN UINTN             TokenNumber
  )
;


UINTN
EFIAPI
DxePcdGetSize (
  IN UINTN             TokenNumber
  )
;


UINT8
EFIAPI
DxePcdGet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  )
;


UINT16
EFIAPI
DxePcdGet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  )
;


UINT32
EFIAPI
DxePcdGet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  )
;



UINT64
EFIAPI
DxePcdGet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  )
;



VOID *
EFIAPI
DxePcdGetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  )
;


BOOLEAN
EFIAPI
DxePcdGetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  )
;


UINTN
EFIAPI
DxePcdGetSizeEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  )
;


EFI_STATUS
EFIAPI
DxePcdSet8 (
  IN UINTN             TokenNumber,
  IN UINT8             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet16 (
  IN UINTN             TokenNumber,
  IN UINT16             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet32 (
  IN UINTN             TokenNumber,
  IN UINT32             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet64 (
  IN UINTN             TokenNumber,
  IN UINT64            Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSetPtr (
  IN        UINTN             TokenNumber,
  IN        UINTN             *SizeOfBuffer,
  IN        VOID              *Buffer
  )
;


EFI_STATUS
EFIAPI
DxePcdSetBool (
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT8             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT16            Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT32             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT64            Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSetPtrEx (
  IN        CONST EFI_GUID    *Guid,
  IN        UINTN             TokenNumber,
  IN OUT    UINTN             *SizeOfBuffer,
  IN        VOID              *Buffer
  )
;


EFI_STATUS
EFIAPI
DxePcdSetBoolEx (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  )
;



EFI_STATUS
EFIAPI
DxeRegisterCallBackOnSet (
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  UINTN                   TokenNumber,
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
;


EFI_STATUS
EFIAPI
DxeUnRegisterCallBackOnSet (
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  UINTN                   TokenNumber,
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
;


EFI_STATUS
EFIAPI
DxePcdGetNextToken (
  IN CONST EFI_GUID               *Guid, OPTIONAL
  IN OUT   UINTN                  *TokenNumber
  )
;



EFI_STATUS
EFIAPI
DxePcdGetNextTokenSpace (
  IN OUT CONST EFI_GUID               **Guid
  )
;


typedef struct {
  LIST_ENTRY              Node;
  PCD_PROTOCOL_CALLBACK   CallbackFn;
} CALLBACK_FN_ENTRY;

#define CR_FNENTRY_FROM_LISTNODE(Record, Type, Field) _CR(Record, Type, Field)

//
// Internal Functions
//

EFI_STATUS
SetValueWorker (
  IN UINTN                   TokenNumber,
  IN VOID                    *Data,
  IN UINTN                   Size
  )
;

EFI_STATUS
SetWorker (
  IN          UINTN                     TokenNumber,
  IN          VOID                      *Data,
  IN OUT      UINTN                     *Size,
  IN          BOOLEAN                   PtrType
  )
;


EFI_STATUS
ExSetValueWorker (
  IN          UINTN                ExTokenNumber,
  IN          CONST EFI_GUID       *Guid,
  IN          VOID                 *Data,
  IN          UINTN                SetSize
  )
;



EFI_STATUS
ExSetWorker (
  IN      UINTN                ExTokenNumber,
  IN      CONST EFI_GUID       *Guid,
  IN      VOID                 *Data,
  IN OUT  UINTN                *Size,
  IN      BOOLEAN              PtrType
  )
;


VOID *
GetWorker (
  IN UINTN             TokenNumber,
  IN UINTN             GetSize
  )
;

VOID *
ExGetWorker (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  ExTokenNumber,
  IN UINTN                  GetSize
  ) 
;

UINT32
GetSkuEnabledTokenNumber (
  UINT32 LocalTokenNumber,
  UINTN  Size,
  BOOLEAN IsPeiDb
  ) 
;

EFI_STATUS
GetHiiVariable (
  IN  EFI_GUID      *VariableGuid,
  IN  UINT16        *VariableName,
  OUT UINT8          **VariableData,
  OUT UINTN         *VariableSize
  )
;

EFI_STATUS
SetHiiVariable (
  IN  EFI_GUID     *VariableGuid,
  IN  UINT16       *VariableName,
  IN  CONST VOID   *Data,
  IN  UINTN        DataSize,
  IN  UINTN        Offset
  )
;

EFI_STATUS
DxeRegisterCallBackWorker (
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
);

EFI_STATUS
DxeUnRegisterCallBackWorker (
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
);

VOID
BuildPcdDxeDataBase (
  VOID
);


UINTN           
GetExPcdTokenNumber (
  IN CONST EFI_GUID             *Guid,
  IN UINT32                     ExTokenNumber
  )
;



EFI_STATUS           
ExGetNextTokeNumber (
  IN      CONST EFI_GUID    *Guid,
  IN OUT  UINTN             *TokenNumber,
  IN      EFI_GUID          *GuidTable,
  IN      UINTN             SizeOfGuidTable,
  IN      DYNAMICEX_MAPPING *ExMapTable,
  IN      UINTN             SizeOfExMapTable
  )
;


UINTN
GetPtrTypeSize (
  IN    UINTN             LocalTokenNumberTableIdx,
  OUT   UINTN             *MaxSize
  )
;



BOOLEAN
SetPtrTypeSize (
  IN          UINTN             LocalTokenNumberTableIdx,
  IN    OUT   UINTN             *CurrentSize
  )
;

extern EFI_GUID gPcdDataBaseHobGuid;

extern PCD_DATABASE * mPcdDatabase;

extern DXE_PCD_DATABASE_INIT gDXEPcdDbInit;

extern EFI_LOCK mPcdDatabaseLock;

#endif
