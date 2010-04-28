/**
  AsmWriteLdtr function

  Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BaseLibInternals.h"

VOID
EFIAPI
AsmWriteLdtr (
  IN UINT16 Ldtr
  )
{
  _asm {
    xor     eax, eax
    mov     ax, Ldtr
    lldt    ax
  }
}

