/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    OemFvHob.c
    
Abstract:

  The GUID of the GUIDed HOB that represents the OEM FV block.

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (PeiPerformanceHob)

EFI_GUID  gEfiPeiPerformanceHobGuid  = EFI_PEI_PERFORMANCE_HOB_GUID;

EFI_GUID_STRING (&gEfiPeiPerformanceHobGuid, "PEI Performance HOB",
                 "Guid for PEI Performance Measurement HOB");

