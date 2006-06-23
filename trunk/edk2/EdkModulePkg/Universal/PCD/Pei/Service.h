/** @file
Private functions used by PCD PEIM.

Copyright (c) 2006, Intel Corporation                                                         
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

//
// Please make sure the PCD Serivce PEIM Version is consistent with
// the version of PCD Database generation tool
//
#define PCD_PEI_SERVICE_DRIVER_VERSION      2

//
// PCD_PEI_DATABASE_GENTOOL_VERSION is defined in Autogen.h
// and generated by PCD Database generation tool.
//
#if (PCD_PEI_SERVICE_PEIM_VERSION != PCD_PEI_DATABASE_GENTOOL_VERSION)
  #error "Please make sure the version of PCD Service PEIM and PCD PEI Database Generation Tool matches"
#endif

/* Internal Function definitions */

PEI_PCD_DATABASE *
GetPcdDatabase (
  VOID
  )
;

EFI_STATUS
SetWorker (
  IN UINTN              TokenNumber,
  IN VOID               *Data,
  IN UINTN              Size,
  IN BOOLEAN            PtrType
  )
;

EFI_STATUS
ExSetWorker (
  IN UINTN                ExTokenNumber,
  IN CONST EFI_GUID       *Guid,
  IN VOID                 *Data,
  IN UINTN                Size,
  IN BOOLEAN              PtrType
  )
;

VOID *
GetWorker (
  IN UINTN                TokenNumber,
  IN UINTN                GetSize
  )
;

VOID *
ExGetWorker (
  IN CONST EFI_GUID   *Guid,
  IN UINTN            ExTokenNumber,
  IN UINTN            GetSize
  )
;

typedef struct {
  UINTN   TokenNumber;
  UINTN   Size;
  UINT32  LocalTokenNumberAlias;
} EX_PCD_ENTRY_ATTRIBUTE;


UINTN           
GetExPcdTokenNumber (
  IN CONST EFI_GUID             *Guid,
  IN UINTN                      ExTokenNumber
  )
;




EFI_STATUS
PeiRegisterCallBackWorker (
  IN  UINTN              TokenNumber,
  IN  CONST GUID         *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK   CallBackFunction,
  IN  BOOLEAN            Register
);




VOID
BuildPcdDatabase (
  VOID
  )
;


//
// PPI Interface Implementation Declaration.
//
VOID
EFIAPI
PeiPcdSetSku (
  IN  UINTN                  SkuId
  )
;


UINT8
EFIAPI
PeiPcdGet8 (
  IN UINTN             TokenNumber
  )
;


UINT16
EFIAPI
PeiPcdGet16 (
  IN UINTN             TokenNumber
  )
;


UINT32
EFIAPI
PeiPcdGet32 (
  IN UINTN             TokenNumber
  )
;


UINT64
EFIAPI
PeiPcdGet64 (
  IN UINTN             TokenNumber
  )
;


VOID *
EFIAPI
PeiPcdGetPtr (
  IN UINTN             TokenNumber
  )
;


BOOLEAN
EFIAPI
PeiPcdGetBool (
  IN UINTN             TokenNumber
  )
;


UINTN
EFIAPI
PeiPcdGetSize (
  IN UINTN             TokenNumber
  )
;


UINT8
EFIAPI
PeiPcdGet8Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  )
;


UINT16
EFIAPI
PeiPcdGet16Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  )
;

UINT32
EFIAPI
PeiPcdGet32Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  )
;


UINT64
EFIAPI
PeiPcdGet64Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  )
;


VOID *
EFIAPI
PeiPcdGetPtrEx (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  )
;


BOOLEAN
EFIAPI
PeiPcdGetBoolEx (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  )
;


UINTN
EFIAPI
PeiPcdGetSizeEx (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet8 (
  IN UINTN             TokenNumber,
  IN UINT8             Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet16 (
  IN UINTN             TokenNumber,
  IN UINT16            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet32 (
  IN UINTN             TokenNumber,
  IN UINT32            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet64 (
  IN UINTN             TokenNumber,
  IN UINT64            Value
  )
;

EFI_STATUS
EFIAPI
PeiPcdSetPtr (
  IN UINTN             TokenNumber,
  IN UINTN             SizeOfBuffer,
  IN VOID              *Buffer
  )
;


EFI_STATUS
EFIAPI
PeiPcdSetBool (
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet8Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT8             Value
  )
;

EFI_STATUS
EFIAPI
PeiPcdSet16Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT16            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet32Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT32            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet64Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT64            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSetPtrEx (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINTN             SizeOfBuffer,
  IN VOID              *Buffer
  )
;


EFI_STATUS
EFIAPI
PeiPcdSetBoolEx (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  )
;



EFI_STATUS
EFIAPI
PeiRegisterCallBackOnSet (
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK        CallBackFunction
  )
;


EFI_STATUS
EFIAPI
PcdUnRegisterCallBackOnSet (
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK        CallBackFunction
  )
;


EFI_STATUS
EFIAPI
PeiPcdGetNextToken (
  IN CONST EFI_GUID           *Guid, OPTIONAL
  IN OUT  UINTN               *TokenNumber
  )
;

extern EFI_GUID gPcdDataBaseHobGuid;

extern EFI_GUID gPcdPeiCallbackFnTableHobGuid;

extern PEI_PCD_DATABASE_INIT gPEIPcdDbInit;

#endif
