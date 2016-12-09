/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#ifndef _EFI_EDB_DISASM_H_
#define _EFI_EDB_DISASM_H_

#include <Uefi.h>

//
// Definition for instruction OPCODE, MODIFIER, and OPERAND
//
#define GET_OPCODE(Addr)       (UINT8)((*(UINT8 *)(UINTN)(Addr)) & 0x3F)
#define GET_MODIFIERS(Addr)    (UINT8)((*(UINT8 *)(UINTN)(Addr)) & 0xC0)
#define GET_OPCODE_BYTE(Addr)  (UINT8)(*(UINT8 *)(UINTN)(Addr))
#define GET_OPERANDS(Addr)     (UINT8)(*(UINT8 *)(UINTN)((Addr) + 1))

typedef
UINTN
(* EDB_DISASM_INSTRUCTION) (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisAsmString
  );

#endif
