/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*  
*  This program and the accompanying materials                          
*  are licensed and made available under the terms and conditions of the BSD License         
*  which accompanies this distribution.  The full text of the license may be found at        
*  http://opensource.org/licenses/bsd-license.php                                            
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
*
**/

#include <Library/IoLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <Drivers/PL341Dmc.h>
#include <Drivers/SP804Timer.h>

#include <ArmPlatform.h>

/**
  Initialize the Secure peripherals and memory regions

  If Trustzone is supported by your platform then this function makes the required initialization
  of the secure peripherals and memory regions.

**/
VOID
ArmPlatformTrustzoneInit (
  VOID
  )
{
  //ASSERT(FALSE);
  DEBUG((EFI_D_ERROR,"Initialize Trustzone Hardware\n"));
}

/**
  Initialize controllers that must setup at the early stage

  Some peripherals must be initialized in Secure World.
  For example, some L2x0 requires to be initialized in Secure World

**/
VOID
ArmPlatformSecInitialize (
  VOID
  ) {
  // Do nothing yet
}

/**
  Call before jumping to Normal World

  This function allows the firmware platform to do extra actions before
  jumping to the Normal World

**/
VOID
ArmPlatformSecExtraAction (
  IN  UINTN         MpId,
  OUT UINTN*        JumpAddress
  )
{
  *JumpAddress = PcdGet32(PcdNormalFvBaseAddress);
}
