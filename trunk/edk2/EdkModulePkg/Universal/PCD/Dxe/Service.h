/** @file
Private functions used by PCD DXE driver.

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

#define USE_AUTOGEN

#ifndef USE_AUTOGEN
//
// The following definition will be generated by build tool 
//

//
// Common definitions
//
#define PCD_TYPE_SHIFT        24


#define PCD_TYPE_DATA         (0x00 << PCD_TYPE_SHIFT)
#define PCD_TYPE_HII    	    (0x80 << PCD_TYPE_SHIFT)
#define PCD_TYPE_VPD    	    (0x40 << PCD_TYPE_SHIFT)
#define PCD_TYPE_SKU_ENABLED 	(0x20 << PCD_TYPE_SHIFT)


#define PCD_DATABASE_OFFSET_MASK (~(PCD_TYPE_HII | PCD_TYPE_VPD | PCD_TYPE_SKU_ENABLED))

typedef struct  {
  UINT32                ExTokenNumber;
  UINT32                LocalTokenNumber;   // PCD Number of this particular platform build
  UINT16                ExGuidIndex;        // Index of GuidTable
} DYNAMICEX_MAPPING;


typedef struct {
  UINT32  SkuDataStartOffset; //We have to use offsetof MACRO as we don't know padding done by compiler
  UINT32  SkuIdTableOffset;   //Offset from the PCD_DB
} SKU_HEAD;


typedef struct {
  UINT16  GuidTableIndex;   // Offset in Guid Table in units of GUID.
  UINT16  StringIndex;           // Offset in String Table in units of UINT16.
  UINT16  Offset;           // Offset in Variable
} VARIABLE_HEAD  ;


typedef  struct {
  UINT32  Offset;
} VPD_HEAD;

typedef struct {
  UINT32 LocalTokenNumber;
  UINT16 TokenNumber;
  UINT16 Size;
} SIZEINFO;

#define offsetof(s,m)                 (UINT32)&(((s *)0)->m)



//
// C Structure generate for PEI PCD Database
//
#define PEI_EXMAPPING_TABLE_SIZE  1
#define PEI_GUID_TABLE_SIZE         1
#define PEI_LOCAL_TOKEN_NUMBER            1
#define PEI_EXTOKEN_NUMBER          1
#define PEI_STRING_TABLE_SIZE        2
#define PEI_SKUID_TABLE_SIZE         3
#define PEI_SIZE_TABLE_SIZE          1

#define PEI_DATABASE_EMPTRY          FALSE
#define PEI_DYNAMICEX_MAPPING_EMPTY  FALSE
#define PEI_GUID_TABLE_EMPTY         FALSE
#define PEI_STRINGTABLE_EMPTY        FALSE
#define PEI_SIZETABLE_EMPTY          FALSE
#define PEI_SKUID_TABLE_EMPTY        FALSE


typedef struct {

  DYNAMICEX_MAPPING ExMapTable[PEI_EXMAPPING_TABLE_SIZE];
  EFI_GUID          GuidTable[PEI_GUID_TABLE_SIZE];

  UINT32            LocalTokenNumberTable[PEI_LOCAL_TOKEN_NUMBER];


  UINT16            StringTable[PEI_STRING_TABLE_SIZE];
  UINT16            SizeTable[PEI_LOCAL_TOKEN_NUMBER];
  
  SKU_ID            SkuIdTable[PEI_SKUID_TABLE_SIZE];
  
  SKU_ID            SystemSkuId;

} PEI_PCD_DATABASE_INIT;

typedef struct {
  UINT8 Dummy;
} PEI_PCD_DATABASE_UNINIT;

//
// Following code should be generated for PCD DXE driver
//

#define DXE_EXMAPPING_TABLE_SIZE  1
#define DXE_GUID_TABLE_SIZE         1
#define DXE_TOKEN_NUMBER            1
#define DXE_EXTOKEN_NUMBER          1
#define DXE_STRING_TABLE_SIZE        2
#define DXE_SKUID_TABLE_SIZE         3
#define DXE_SIZE_TABLE_SIZE          1

#define DXE_DATABASE_EMPTRY          FALSE
#define DXE_DYNAMICEX_MAPPING_EMPTY  FALSE
#define DXE_GUID_TABLE_EMPTY         FALSE
#define DXE_STRINGTABLE_EMPTY        FALSE
#define DXE_SIZETABLE_EMPTY          FALSE
#define DXE_SKUID_TABLE_EMPTY        FALSE

typedef struct {
  DYNAMICEX_MAPPING ExMapTable[DXE_EXMAPPING_TABLE_SIZE];
  EFI_GUID          GuidTable[DXE_GUID_TABLE_SIZE];

  UINT32            LocalTokenNumberTable[DXE_TOKEN_NUMBER];


  UINT16            StringTable[DXE_STRING_TABLE_SIZE];
  UINT16            SizeTable[DXE_TOKEN_NUMBER];
  
  SKU_ID            SkuIdTable[DXE_SKUID_TABLE_SIZE];
  
} DXE_PCD_DATABASE_INIT;

typedef struct {
  UINT8 Dummy;
} DXE_PCD_DATABASE_UNINIT;


#define DXE_PCD_DB_INIT_VALUE \
    /* ExMapTable */ \
  { \
    { /* ExTokenNumber */ 0x00000001, /* LocalTokenNumberIndex */ 0, /* ExGuidIndex */ 0} \
  }, \
  \
  /* GuidTable */ \
  { \
    { 0xBB25CF6F, 0xF1D4, 0x11D2, {0x9A, 0x0C, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0xFD }} \
  }, \
  \
  /* LocalTokenNumberTable */ \
  { \
    0 \
  }, \
  \
  /* StringTable */ \
  { \
    L"\0" \
  }, \
  \
  /* SizeTable */ \
  { \
    4 \
  }, \
  \
  /* SkuIdTable */ \
  { \
    /*MaxSku*/ 2, /*SkuId*/ 100, /*SkuId*/200   \
  },\
  \

//
// End of Autogen Code
//
#endif

/*
typedef struct {
  PEI_PCD_DATABASE_INIT Init;
  PEI_PCD_DATABASE_UNINIT Uninit;
} PEI_PCD_DATABASE;



typedef struct {
  DXE_PCD_DATABASE_INIT Init;
  DXE_PCD_DATABASE_UNINIT Uninit;
} DXE_PCD_DATABASE;


typedef struct {
  PEI_PCD_DATABASE PeiDb;
  DXE_PCD_DATABASE DxeDb;
} PCD_DATABASE;
*/


//
// Internal Functions
//

EFI_STATUS
SetWorker (
  UINTN         TokenNumber,
  VOID          *Data,
  UINTN         Size,
  BOOLEAN       PtrType
  )
;

EFI_STATUS
ExSetWorker (
  IN UINT32               ExTokenNumber,
  IN CONST EFI_GUID       *Guid,
  VOID                    *Data,
  UINTN                   Size,
  BOOLEAN                 PtrType
  )
;


VOID *
GetWorker (
  UINTN  TokenNumber
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
  OUT VOID          ** VariableData,
  OUT UINTN         *VariableSize
  )
;

EFI_STATUS
DxeRegisterCallBackWorker (
  IN  UINTN        TokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction,
  IN  BOOLEAN                 Reigster
);

EFI_STATUS
DxeGetNextTokenWorker (
  IN OUT UINTN *Token,
  IN CONST EFI_GUID           *Guid     OPTIONAL
  );

VOID
BuildPcdDxeDataBase (
  VOID
);


typedef struct {
  UINTN   TokenNumber;
  UINTN   Size;
  UINT32  LocalTokenNumberAlias;
  BOOLEAN IsPeiDb;
} EX_PCD_ENTRY_ATTRIBUTE;

VOID
GetExPcdTokenAttributes (
  IN CONST EFI_GUID             *Guid,
  IN UINT32                     ExTokenNumber,
  OUT EX_PCD_ENTRY_ATTRIBUTE    *ExAttr
  )
;

//
// Protocol Interface function declaration.
//
VOID
EFIAPI
DxePcdSetSku (
  IN  SKU_ID                  SkuId
  )
;


UINT8
EFIAPI
DxePcdGet8 (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINT16
EFIAPI
DxePcdGet16 (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINT32
EFIAPI
DxePcdGet32 (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINT64
EFIAPI
DxePcdGet64 (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


VOID *
EFIAPI
DxePcdGetPtr (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


BOOLEAN
EFIAPI
DxePcdGetBool (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINTN
EFIAPI
DxePcdGetSize (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINT8
EFIAPI
DxePcdGet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINT16
EFIAPI
DxePcdGet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINT32
EFIAPI
DxePcdGet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;



UINT64
EFIAPI
DxePcdGet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;



VOID *
EFIAPI
DxePcdGetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


BOOLEAN
EFIAPI
DxePcdGetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINTN
EFIAPI
DxePcdGetSizeEx (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


EFI_STATUS
EFIAPI
DxePcdSet8 (
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT8             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet16 (
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT16             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet32 (
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT32             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet64 (
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT64            Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSetPtr (
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINTN             SizeOfBuffer,
  IN VOID              *Buffer
  )
;


EFI_STATUS
EFIAPI
DxePcdSetBool (
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN BOOLEAN           Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT8             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT16            Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT32             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT64            Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSetPtrEx (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINTN             SizeOfBuffer,
  IN VOID              *Buffer
  )
;


EFI_STATUS
EFIAPI
DxePcdSetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN BOOLEAN           Value
  )
;



EFI_STATUS
EFIAPI
PcdRegisterCallBackOnSet (
  IN  PCD_TOKEN_NUMBER        TokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
;


EFI_STATUS
EFIAPI
PcdUnRegisterCallBackOnSet (
  IN  PCD_TOKEN_NUMBER        TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
;


EFI_STATUS
EFIAPI
DxePcdGetNextToken (
  IN CONST EFI_GUID               *Guid, OPTIONAL
  IN OUT   PCD_TOKEN_NUMBER       *TokenNumber
  )
;

EFI_STATUS
SetWorkerByLocalTokenNumber (
  UINT32        LocalTokenNumber,
  VOID          *Data,
  UINTN         Size,
  BOOLEAN       PtrType,
  BOOLEAN       IsPeiDb
  )
;

extern EFI_GUID gPcdDataBaseHobGuid;

extern PCD_DATABASE * gPcdDatabase;

extern DXE_PCD_DATABASE_INIT gDXEPcdDbInit;

#endif
