/** @file
  Implement the connection to the socket driver

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/EfiSocket.h>
#include <Protocol/ServiceBinding.h>


/**
  Connect to the socket driver

  @param [in] ppSocketProtocol  Address to receive the socket protocol address

  @retval 0             Successfully returned the socket protocol
  @retval other         Value for errno
 **/
int
EslServiceGetProtocol (
  IN EFI_SOCKET_PROTOCOL ** ppSocketProtocol
  )
{
  EFI_SERVICE_BINDING_PROTOCOL * pServiceBinding;
  int RetVal;
  EFI_HANDLE SocketHandle;
  EFI_STATUS Status;

  //
  //  Locate the socket protocol
  //
  Status = gBS->LocateProtocol ( &gEfiSocketServiceBindingProtocolGuid,
                                 NULL,
                                 (VOID **)&pServiceBinding );
  if ( !EFI_ERROR ( Status )) {
    //
    //  Create a new socket
    //
    SocketHandle = NULL;
    Status = pServiceBinding->CreateChild ( pServiceBinding,
                                            &SocketHandle );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Get the socket protocol
      //
      Status = gBS->OpenProtocol ( SocketHandle,
                                   &gEfiSocketProtocolGuid,
                                   (VOID **)ppSocketProtocol,
                                   NULL,
                                   NULL,
                                   EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL );
      if ( !EFI_ERROR ( Status )) {
        //
        //  Success!
        //
        RetVal = 0;
      }
      else {
        DEBUG (( DEBUG_ERROR,
                  "ERROR - No socket protocol on 0x%08x, Status: %r\r\n",
                  SocketHandle,
                  Status ));
        RetVal = ENODEV;
      }
    }
    else {
      //
      //  Translate the error
      //
      DEBUG (( DEBUG_ERROR,
                "ERROR - CreateChild failed, Status: %r\r\n",
                Status ));
      switch ( Status ) {
      case EFI_SUCCESS:
        RetVal = 0;
        break;

      case EFI_ACCESS_DENIED:
      case EFI_WRITE_PROTECTED:
        RetVal = EACCES;
        break;

      case EFI_NO_RESPONSE:
        RetVal = EHOSTUNREACH;
        break;

      case EFI_BAD_BUFFER_SIZE:
      case EFI_BUFFER_TOO_SMALL:
      case EFI_INVALID_PARAMETER:
        RetVal = EINVAL;
        break;

      case EFI_DEVICE_ERROR:
      case EFI_MEDIA_CHANGED:
      case EFI_NO_MEDIA:
      case EFI_VOLUME_CORRUPTED:
        RetVal = EIO;
        break;

      case EFI_NOT_FOUND:
        RetVal = ENOENT;
        break;

      default:
      case EFI_OUT_OF_RESOURCES:
        RetVal = ENOMEM;
        break;

      case EFI_VOLUME_FULL:
        RetVal = ENOSPC;
        break;

      case EFI_UNSUPPORTED:
        RetVal = ENOSYS;
        break;

      case EFI_NO_MAPPING:
        RetVal = ENXIO;
        break;

      case EFI_LOAD_ERROR:
        RetVal = ESRCH;
        break;

      case EFI_TIMEOUT:
        RetVal = ETIMEDOUT;
        break;

      case EFI_NOT_READY:
        RetVal = EWOULDBLOCK;
        break;
      }
    }
  }
  else {
    DEBUG (( DEBUG_ERROR,
              "ERROR - No socket service binding protocol, Status: %r\r\n",
              Status ));
    RetVal = ENODEV;
  }

  //
  //  Return the operation status
  //
  return RetVal;
}
