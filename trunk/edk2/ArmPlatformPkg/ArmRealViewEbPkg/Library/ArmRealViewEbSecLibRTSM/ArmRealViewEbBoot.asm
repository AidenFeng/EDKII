//
//  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

#include <AsmMacroIoLib.h>
#include <Base.h>
#include <Library/ArmPlatformSecLib.h>
#include <ArmPlatform.h>
#include <AutoGen.h>

  INCLUDE AsmMacroIoLib.inc

  EXPORT  ArmPlatformSecBootAction
  EXPORT  ArmPlatformSecBootMemoryInit

  PRESERVE8
  AREA    ArmRealviewEbBootMode, CODE, READONLY

/**
  Call at the beginning of the platform boot up

  This function allows the firmware platform to do extra actions at the early
  stage of the platform power up.

  Note: This function must be implemented in assembler as there is no stack set up yet

**/
ArmPlatformSecBootAction
  LoadConstantToReg (ARM_EB_SYS_FLAGS_NV_REG, r0)
  ldr   r0, [r0]
  cmp   r0, #0
  bxeq  lr
  bxne  r0

/**
  Initialize the memory where the initial stacks will reside

  This memory can contain the initial stacks (Secure and Secure Monitor stacks).
  In some platform, this region is already initialized and the implementation of this function can
  do nothing. This memory can also represent the Secure RAM.
  This function is called before the satck has been set up. Its implementation must ensure the stack
  pointer is not used (probably required to use assembly language)

**/
ArmPlatformSecBootMemoryInit
  // The SMC does not need to be initialized for RTSM
  bx    lr

  END
