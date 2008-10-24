/** @file
  AsmMwait function

  Copyright (c) 2006 - 2008, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

/**
  Executes an MWAIT instruction.

  Executes an MWAIT instruction with the register state specified by Eax and
  Ecx. Returns Eax. This function is only available on IA-32 and X64.

  @param  RegisterEax The value to load into EAX or RAX before executing the MONITOR
                      instruction.
  @param  RegisterEcx The value to load into ECX or RCX before executing the MONITOR
                      instruction.

  @return RegisterEax

**/
UINTN
EFIAPI
AsmMwait (
  IN      UINTN                     RegisterEax,
  IN      UINTN                     RegisterEcx
  )
{
  _asm {
    mov     eax, RegisterEax
    mov     ecx, RegisterEcx
    _emit   0x0f              // mwait
    _emit   0x01
    _emit   0xC9
  }
}

