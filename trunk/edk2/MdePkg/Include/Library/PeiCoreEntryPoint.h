/** @file
  Entry point to the PEI Core

Copyright (c) 2006, Intel Corporation<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __MODULE_ENTRY_POINT_H__
#define __MODULE_ENTRY_POINT_H__

/**
  Enrty point to PEI core.

  @param  PeiStartupDescriptor Pointer of start up information.
 
  @return Status returned by entry points of core and drivers. 

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN CONST  EFI_SEC_PEI_HAND_OFF    *SecCoreData,
  IN CONST  EFI_PEI_PPI_DESCRIPTOR  *PpiList
  );

/**
  Wrapper of enrty point to PEI core.

  @param  PeiStartupDescriptor Pointer of start up information.
 
  @return Status returned by entry points of core and drivers. 

**/
EFI_STATUS
EFIAPI
EfiMain (
  IN CONST  EFI_SEC_PEI_HAND_OFF    *SecCoreData,
  IN CONST  EFI_PEI_PPI_DESCRIPTOR  *PpiList
  );

/**
  Call constructs for all libraries. Automatics Generated by tool.

  @param  FfsHeader   Pointer to header of FFS.
  @param  PeiServices Pointer to the PEI Services Table.

**/
VOID
EFIAPI
ProcessLibraryConstructorList (
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  IN EFI_PEI_SERVICES     **PeiServices
  );


/**
  Call the list of driver entry points. Automatics Generated by tool.

  @param  PeiStartupDescriptor  Pointer to startup information .
  @param  OldCoreData           Pointer to Original startup information.

  @return Status returned by entry points of drivers.  
 
**/
EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN CONST  EFI_SEC_PEI_HAND_OFF    *SecCoreData,
  IN CONST  EFI_PEI_PPI_DESCRIPTOR  *PpiList,
  IN VOID                           *OldCoreData
  );

#endif
