/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    TianoDecompress.c
    
Abstract:

  The GUID for the Tiano Decompress Protocol

--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION(TianoDecompress)

EFI_GUID gEfiTianoDecompressProtocolGuid = EFI_TIANO_DECOMPRESS_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiTianoDecompressProtocolGuid, "Tiano Decompress", "Tiano Decompression Protocol");

