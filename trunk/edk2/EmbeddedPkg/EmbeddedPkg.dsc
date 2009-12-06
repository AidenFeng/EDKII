#%HEADER%
#/** @file
# Embedded Package
#
#
# Copyright (c) 2007, Intel Corporation
#
#  All rights reserved. This program and the accompanying materials
#    are licensed and made available under the terms and conditions of the BSD License
#    which accompanies this distribution. The full text of the license may be found at
#    http://opensource.org/licenses/bsd-license.php
#
#    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#**/

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = Embedded
  PLATFORM_GUID                  = 8DBB580B-CF89-4D57-95C6-DFE96C44686E
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/Embedded
  SUPPORTED_ARCHITECTURES        = IA32|X64|IPF|ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = EmbeddedPkg/EmbeddedPkg.fdf


################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this
#                              Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT              # The entry: 0|DEFAULT is reserved and always required.

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################
[LibraryClasses.common]
#  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf

  
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  EfiFileLib|EmbeddedPkg/Library/EfiFileLib/EfiFileLib.inf
  
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PrePiLib|EmbeddedPkg/Library/PrePiLib/PrePiLib.inf
  
  SerialPortLib|EmbeddedPkg/Library/TemplateSerialPortLib/TemplateSerialPortLib.inf
  RealTimeClockLib|EmbeddedPkg/Library/TemplateRealTimeClockLib/TemplateRealTimeClockLib.inf
  EfiResetSystemLib|EmbeddedPkg/Library/TemplateResetSystemLib/TemplateResetSystemLib.inf
  GdbSerialLib|EmbeddedPkg/Library/GdbSerialLib/GdbSerialLib.inf


 #
 # Need to change this for IPF
 #
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf

  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf


  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  EblCmdLib|EmbeddedPkg/Library/EblCmdLibNull/EblCmdLibNull.inf
  
  EblNetworkLib|EmbeddedPkg/Library/EblNetworkLib/EblNetworkLib.inf
  

[LibraryClasses.common.DXE_DRIVER]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf


[LibraryClasses.common.UEFI_APPLICATION]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf

[LibraryClasses.common.UEFI_DRIVER]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  
[LibraryClasses.common.SEC]
  ExtractGuidedSectionLib|EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf

[LibraryClasses.ARM]
  SemihostLib|ArmPkg/Library/SemihostLib/SemihostLib.inf

################################################################################
#
# Pcd Section - list of all PCD Entries defined by this Platform
#
################################################################################

[PcdsFeatureFlag.common]
  gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnosticsDisable|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdComponentName2Disable|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnostics2Disable|FALSE
  
  #
  # Control what commands are supported from the UI
  # Turn these on and off to add features or save size
  #  
  gEmbeddedTokenSpaceGuid.PcdEmbeddedMacBoot|TRUE
  gEmbeddedTokenSpaceGuid.PcdEmbeddedDirCmd|TRUE
  gEmbeddedTokenSpaceGuid.PcdEmbeddedHobCmd|TRUE
  gEmbeddedTokenSpaceGuid.PcdEmbeddedHwDebugCmd|TRUE
  gEmbeddedTokenSpaceGuid.PcdEmbeddedIoEnable|FALSE
  gEmbeddedTokenSpaceGuid.PcdEmbeddedScriptCmd|FALSE
  gEmbeddedTokenSpaceGuid.PcdEmbeddedPciDebugCmd|TRUE
  
  gEmbeddedTokenSpaceGuid.PcdPrePiProduceMemoryTypeInformationHob|FALSE


[PcdsFixedAtBuild.common]
  gEfiMdePkgTokenSpaceGuid.PcdMaximumUnicodeStringLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumAsciiStringLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumLinkedListLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdSpinLockTimeout|10000000
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x06
  gEfiMdePkgTokenSpaceGuid.PcdDebugClearMemoryValue|0xAF
  gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|0
  gEfiMdePkgTokenSpaceGuid.PcdPostCodePropertyMask|0
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xE0000000
  gEfiMdePkgTokenSpaceGuid.PcdFSBClock|200000000
  gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|320
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xE0000000
  gEfiMdePkgTokenSpaceGuid.PcdFSBClock|200000000
  gEmbeddedTokenSpaceGuid.PcdEmbeddedAutomaticBootCommand|L""|VOID*|2
  gEmbeddedTokenSpaceGuid.PcdEmbeddedDefaultTextColor|0x07
  gEmbeddedTokenSpaceGuid.PcdEmbeddedMemVariableStoreSize|0x10000
  
  gEmbeddedTokenSpaceGuid.PcdPrePiHobBase|0
  gEmbeddedTokenSpaceGuid.PcdPrePiStackBase|0
  gEmbeddedTokenSpaceGuid.PcdPrePiStackSize|0
  gEmbeddedTokenSpaceGuid.PcdPrePiTempMemorySize|0
  gEmbeddedTokenSpaceGuid.PcdPrePiBfvBaseAddress|0
  gEmbeddedTokenSpaceGuid.PcdPrePiBfvSize|0

#
# Optinal feature to help prevent EFI memory map fragments
# Turned on and off via: PcdPrePiProduceMemoryTypeInformationHob
# Values are in EFI Pages (4K). DXE Core will make sure that 
# at least this much of each type of memory can be allocated 
# from a single memory range. This way you only end up with
# maximum of two fragements for each type in the memory map
# (the memory used, and the free memory that was prereserved
# but not used).
#
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiACPIReclaimMemory|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiACPIMemoryNVS|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiReservedMemoryType|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiRuntimeServicesData|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiRuntimeServicesCode|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiBootServicesCode|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiBootServicesData|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiLoaderCode|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiLoaderData|0

#
# Timer config for this platform
#
  gEmbeddedTokenSpaceGuid.PcdTimerBaseAddress|0x3c700000
  gEmbeddedTokenSpaceGuid.PcdTimerVector|7
  gEmbeddedTokenSpaceGuid.PcdTimerPeriod|100000


[PcdsFixedAtBuild.ARM]
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuMemorySize|32
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuIoSize|0

[PcdsFixedAtBuild.IA32]
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuMemorySize|36
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuIoSize|16

[PcdsFixedAtBuild.X64]
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuMemorySize|52
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuIoSize|16



[PcdsFixedAtBuild.IPF]
  gEfiMdePkgTokenSpaceGuid.PcdIoBlockBaseAddressForIpf|0x0ffffc000000

#
# This makes it so you can source level debug with NT32. VC++ debugger limitiation!
#
#[BuildOptions]
#  DEBUG_*_IA32_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /ALIGN:4096 /SUBSYSTEM:CONSOLE
#  RELEASE_*_IA32_DLINK_FLAGS = /ALIGN:4096
#  *_*_IA32_CC_FLAGS = /D EFI_SPECIFICATION_VERSION=0x0002000A /D TIANO_RELEASE_VERSION=0x00080006


################################################################################
#
# Components Section - list of all Modules needed by this Platform
#
################################################################################
[Components.common]
  EmbeddedPkg/Library/EblAddExternalCommandLib/EblAddExternalCommandLib.inf 
  EmbeddedPkg/Library/EblCmdLibNull/EblCmdLibNull.inf
  EmbeddedPkg/Library/EfiFileLib/EfiFileLib.inf
  EmbeddedPkg/Library/GdbSerialDebugPortLib/GdbSerialDebugPortLib.inf  # ApplePkg
  EmbeddedPkg/Library/GdbSerialLib/GdbSerialLib.inf                    # ApplePkg
  EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf
  EmbeddedPkg/Library/PrePiLib/PrePiLib.inf
  EmbeddedPkg/Library/TemplateSerialPortLib/TemplateSerialPortLib.inf
  EmbeddedPkg/Library/TemplateResetSystemLib/TemplateResetSystemLib.inf
  EmbeddedPkg/Library/TemplateRealTimeClockLib/TemplateRealTimeClockLib.inf

####ArmPkg/Library/UncachedMemoryAllocationLib/UncachedMemoryAllocationLib.inf ???


  EmbeddedPkg/Ebl/Ebl.inf
####  EmbeddedPkg/EblExternCmd/EblExternCmd.inf
  EmbeddedPkg/EmbeddedMonotonicCounter/EmbeddedMonotonicCounter.inf
  EmbeddedPkg/GdbStub/GdbStub.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  EmbeddedPkg/ResetRuntimeDxe/ResetRuntimeDxe.inf
  EmbeddedPkg/SerialDxe/SerialDxe.inf
  EmbeddedPkg/SimpleTextInOutSerial/SimpleTextInOutSerial.inf
  EmbeddedPkg/TemplateBds/TemplateBds.inf
  EmbeddedPkg/TemplateCpuDxe/TemplateCpuDxe.inf
  EmbeddedPkg/TemplateMetronomeDxe/TemplateMetronomeDxe.inf
  EmbeddedPkg/TemplateSec/TemplateSec.inf
  EmbeddedPkg/TemplateTimerDxe/TemplateTimerDxe.inf

  
  

