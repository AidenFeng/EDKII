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

#include <Library/ArmPlatformLib.h>
#include <Library/ArmTrustZoneLib.h>
#include <Library/ArmPlatformSysConfigLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>

#include <Drivers/PL310L2Cache.h>

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
  //
  // Setup TZ Protection Controller
  //

  // Set Non Secure access for all devices
  TZPCSetDecProtBits(ARM_VE_TZPC_BASE, TZPC_DECPROT_0, 0xFFFFFFFF);
  TZPCSetDecProtBits(ARM_VE_TZPC_BASE, TZPC_DECPROT_1, 0xFFFFFFFF);
  TZPCSetDecProtBits(ARM_VE_TZPC_BASE, TZPC_DECPROT_2, 0xFFFFFFFF);

  // Remove Non secure access to secure devices
  TZPCClearDecProtBits(ARM_VE_TZPC_BASE, TZPC_DECPROT_0,
       ARM_VE_DECPROT_BIT_TZPC | ARM_VE_DECPROT_BIT_DMC_TZASC | ARM_VE_DECPROT_BIT_NMC_TZASC | ARM_VE_DECPROT_BIT_SMC_TZASC);

  TZPCClearDecProtBits(ARM_VE_TZPC_BASE, TZPC_DECPROT_2,
       ARM_VE_DECPROT_BIT_EXT_MAST_TZ | ARM_VE_DECPROT_BIT_DMC_TZASC_LOCK | ARM_VE_DECPROT_BIT_NMC_TZASC_LOCK | ARM_VE_DECPROT_BIT_SMC_TZASC_LOCK);

  //
  // Setup TZ Address Space Controller for the SMC. Create 5 Non Secure regions (NOR0, NOR1, SRAM, SMC Peripheral regions)
  //

  // NOR Flash 0 non secure (BootMon)
  TZASCSetRegion(ARM_VE_TZASC_BASE,1,TZASC_REGION_ENABLED,
      ARM_VE_SMB_NOR0_BASE,0,
      TZASC_REGION_SIZE_64MB, TZASC_REGION_SECURITY_NSRW);

  // NOR Flash 1. The first half of the NOR Flash1 must be secure for the secure firmware (sec_uefi.bin)
#if EDK2_ARMVE_SECURE_SYSTEM
  //Note: Your OS Kernel must be aware of the secure regions before to enable this region
  TZASCSetRegion(ARM_VE_TZASC_BASE,2,TZASC_REGION_ENABLED,
      ARM_VE_SMB_NOR1_BASE + SIZE_32MB,0,
      TZASC_REGION_SIZE_32MB, TZASC_REGION_SECURITY_NSRW);
#else
  TZASCSetRegion(ARM_VE_TZASC_BASE,2,TZASC_REGION_ENABLED,
      ARM_VE_SMB_NOR1_BASE,0,
      TZASC_REGION_SIZE_64MB, TZASC_REGION_SECURITY_NSRW);
#endif

  // Base of SRAM. Only half of SRAM in Non Secure world
  // First half non secure (16MB) + Second Half secure (16MB) = 32MB of SRAM
#if EDK2_ARMVE_SECURE_SYSTEM
  //Note: Your OS Kernel must be aware of the secure regions before to enable this region
  TZASCSetRegion(ARM_VE_TZASC_BASE,3,TZASC_REGION_ENABLED,
      ARM_VE_SMB_SRAM_BASE,0,
      TZASC_REGION_SIZE_16MB, TZASC_REGION_SECURITY_NSRW);
#else
  TZASCSetRegion(ARM_VE_TZASC_BASE,3,TZASC_REGION_ENABLED,
      ARM_VE_SMB_SRAM_BASE,0,
      TZASC_REGION_SIZE_32MB, TZASC_REGION_SECURITY_NSRW);
#endif

  // Memory Mapped Peripherals. All in non secure world
  TZASCSetRegion(ARM_VE_TZASC_BASE,4,TZASC_REGION_ENABLED,
      ARM_VE_SMB_PERIPH_BASE,0,
      TZASC_REGION_SIZE_64MB, TZASC_REGION_SECURITY_NSRW);

  // MotherBoard Peripherals and On-chip peripherals.
  TZASCSetRegion(ARM_VE_TZASC_BASE,5,TZASC_REGION_ENABLED,
      ARM_VE_SMB_MB_ON_CHIP_PERIPH_BASE,0,
      TZASC_REGION_SIZE_256MB, TZASC_REGION_SECURITY_NSRW);
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
  // The L2x0 controller must be intialize in Secure World
  L2x0CacheInit(PcdGet32(PcdL2x0ControllerBase),
      PL310_TAG_LATENCIES(L2x0_LATENCY_8_CYCLES,L2x0_LATENCY_8_CYCLES,L2x0_LATENCY_8_CYCLES),
      PL310_DATA_LATENCIES(L2x0_LATENCY_8_CYCLES,L2x0_LATENCY_8_CYCLES,L2x0_LATENCY_8_CYCLES),
      0,~0, // Use default setting for the Auxiliary Control Register
      FALSE);

  // Initialize the System Configuration
  ArmPlatformSysConfigInitialize ();
}
