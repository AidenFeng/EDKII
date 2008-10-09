/** @file

  This file provides a definition of the EFI IPv4 Configuration
  Protocol.

  Copyright (c) 2006 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef __EFI_IP4CONFIG_PROTOCOL_H__
#define __EFI_IP4CONFIG_PROTOCOL_H__

#include <Protocol/Ip4.h>

#define EFI_IP4_CONFIG_PROTOCOL_GUID \
  { \
    0x3b95aa31, 0x3793, 0x434b, {0x86, 0x67, 0xc8, 0x07, 0x08, 0x92, 0xe0, 0x5e } \
  }

typedef struct _EFI_IP4_CONFIG_PROTOCOL EFI_IP4_CONFIG_PROTOCOL;

#define IP4_CONFIG_VARIABLE_ATTRIBUTES \
        (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | \
         EFI_VARIABLE_RUNTIME_ACCESS)

typedef struct {
  EFI_IPv4_ADDRESS             StationAddress;
  EFI_IPv4_ADDRESS             SubnetMask;
  UINT32                       RouteTableSize;
  EFI_IP4_ROUTE_TABLE          *RouteTable;    //OPTIONAL
} EFI_IP4_IPCONFIG_DATA;


/**
  Starts running the configuration policy for the EFI IPv4 Protocol driver.

  @param  This          Pointer to the EFI_IP4_CONFIG_PROTOCOL instance.
  @param  DoneEvent     Event that will be signaled when the EFI IPv4 Protocol driver
                        configuration policy completes execution. This event must be of
                        type EVT_NOTIFY_SIGNAL.
  @param  ReconfigEvent Event that will be signaled when the EFI IPv4 Protocol driver
                        configuration needs to be updated. This event must be of type
                        EVT_NOTIFY_SIGNAL.

  @retval EFI_SUCCESS           The configuration policy for the EFI IPv4 Protocol driver is now
                                running.
  @retval EFI_INVALID_PARAMETER This, DoneEvent, or ReconfigEvent is NULL.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_ALREADY_STARTED   The configuration policy for the EFI IPv4 Protocol driver was
                                already started.
  @retval EFI_DEVICE_ERROR      An unexpected system error or network error occurred.
  @retval EFI_UNSUPPORTED       This interface does not support the EFI IPv4 Protocol driver
                                 configuration.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_IP4_CONFIG_START)(
  IN EFI_IP4_CONFIG_PROTOCOL   *This,
  IN EFI_EVENT                 DoneEvent,
  IN EFI_EVENT                 ReconfigEvent
  );

/**
  Stops running the configuration policy for the EFI IPv4 Protocol driver.

  @param  This Pointer to the EFI_IP4_CONFIG_PROTOCOL instance.

  @retval EFI_SUCCESS           The configuration policy for the EFI IPv4 Protocol driver has been stopped.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_NOT_STARTED       The configuration policy for the EFI IPv4 Protocol driver was not started.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_IP4_CONFIG_STOP)(
  IN EFI_IP4_CONFIG_PROTOCOL   *This
  );

/**
  Returns the default configuration data (if any) for the EFI IPv4 Protocol driver.

  @param  This             Pointer to the EFI_IP4_CONFIG_PROTOCOL instance.
  @param  IpConfigDataSize On input, the size of the IpConfigData buffer.
                           On output, the count of bytes that were written into the IpConfigData buffer.
  @param  IpConfigData     Pointer to the EFI IPv4 Configuration Protocol driver
                           configuration data structure.

  @retval EFI_SUCCESS           The EFI IPv4 Protocol driver configuration has been returned.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_NOT_STARTED       The configuration policy for the EFI IPv4 Protocol driver is not
                                running.
  @retval EFI_NOT_READY         EFI IPv4 Protocol driver configuration is still running.
  @retval EFI_ABORTED           EFI IPv4 Protocol driver configuration could not complete.
  @retval EFI_BUFFER_TOO_SMALL  *IpConfigDataSize is smaller than the configuration data
                                buffer or IpConfigData is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_IP4_CONFIG_GET_DATA)(
  IN EFI_IP4_CONFIG_PROTOCOL   *This,
  IN OUT UINTN                 *IpConfigDataSize,
  OUT EFI_IP4_IPCONFIG_DATA    *IpConfigData    OPTIONAL
  );

/**  
  @par Protocol Description:
  The EFI_IP4_CONFIG_PROTOCOL driver performs platform- and policy-dependent 
  configuration for the EFI IPv4 Protocol driver. 
**/
struct _EFI_IP4_CONFIG_PROTOCOL {
  EFI_IP4_CONFIG_START         Start;
  EFI_IP4_CONFIG_STOP          Stop;
  EFI_IP4_CONFIG_GET_DATA      GetData;
};

extern EFI_GUID gEfiIp4ConfigProtocolGuid;

#endif
