/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    DriverDiagnostics.c
    
Abstract:

    EFI Driver Diagnostics Protocol

Revision History

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (DriverDiagnostics)

EFI_GUID  gEfiDriverDiagnosticsProtocolGuid = EFI_DRIVER_DIAGNOSTICS_PROTOCOL_GUID;

EFI_GUID_STRING
  (&gEfiDriverDiagnosticsProtocolGuid, "Driver Diagnostics Protocol", "EFI 1.1 Driver Diagnostics Protocol");
