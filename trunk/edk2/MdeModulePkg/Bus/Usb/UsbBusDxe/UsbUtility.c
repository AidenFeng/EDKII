/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:

    UsbUtility.c

  Abstract:

    Wrapper function for usb host controller interface

  Revision History


**/


#include "UsbBus.h"


/**
  Get the capability of the host controller

  @param  UsbBus           The usb driver
  @param  MaxSpeed         The maximum speed this host controller supports
  @param  NumOfPort        The number of the root hub port
  @param  Is64BitCapable   Whether this controller support 64 bit addressing

  @retval EFI_SUCCESS      The host controller capability is returned
  @retval Others           Failed to retrieve the host controller capability.

**/
EFI_STATUS
UsbHcGetCapability (
  IN  USB_BUS             *UsbBus,
  OUT UINT8               *MaxSpeed,
  OUT UINT8               *NumOfPort,
  OUT UINT8               *Is64BitCapable
  )
{
  EFI_STATUS              Status;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->GetCapability (
                              UsbBus->Usb2Hc,
                              MaxSpeed,
                              NumOfPort,
                              Is64BitCapable
                              );

  } else {
    Status = UsbBus->UsbHc->GetRootHubPortNumber (UsbBus->UsbHc, NumOfPort);

    *MaxSpeed       = EFI_USB_SPEED_FULL;
    *Is64BitCapable = (UINT8) FALSE;
  }

  return Status;
}


/**
  Reset the host controller

  @param  UsbBus           The usb bus driver
  @param  Attributes       The reset type, only global reset is used by this driver

  @return GC_TODO: add return values

**/
EFI_STATUS
UsbHcReset (
  IN USB_BUS              *UsbBus,
  IN UINT16               Attributes
  )
{
  EFI_STATUS              Status;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->Reset (UsbBus->Usb2Hc, Attributes);
  } else {
    Status = UsbBus->UsbHc->Reset (UsbBus->UsbHc, Attributes);
  }

  return Status;
}


/**
  Get the current operation state of the host controller

  @param  UsbBus           The USB bus driver
  @param  State            The host controller operation state

  @retval EFI_SUCCESS      The operation state is returned in State
  @retval Others           Failed to get the host controller state

**/
EFI_STATUS
UsbHcGetState (
  IN  USB_BUS             *UsbBus,
  OUT EFI_USB_HC_STATE    *State
  )
{
  EFI_STATUS              Status;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->GetState (UsbBus->Usb2Hc, State);
  } else {
    Status = UsbBus->UsbHc->GetState (UsbBus->UsbHc, State);
  }

  return Status;
}


/**
  Set the host controller operation state

  @param  UsbBus           The USB bus driver
  @param  State            The state to set

  @retval EFI_SUCCESS      The host controller is now working at State
  @retval Others           Failed to set operation state

**/
EFI_STATUS
UsbHcSetState (
  IN USB_BUS              *UsbBus,
  IN EFI_USB_HC_STATE     State
  )
{
  EFI_STATUS              Status;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->SetState (UsbBus->Usb2Hc, State);
  } else {
    Status = UsbBus->UsbHc->SetState (UsbBus->UsbHc, State);
  }

  return Status;
}


/**
  Get the root hub port state

  @param  UsbBus           The USB bus driver
  @param  PortIndex        The index of port
  @param  PortStatus       The variable to save port state

  @retval EFI_SUCCESS      The root port state is returned in
  @retval Others           Failed to get the root hub port state

**/
EFI_STATUS
UsbHcGetRootHubPortStatus (
  IN  USB_BUS             *UsbBus,
  IN  UINT8               PortIndex,
  OUT EFI_USB_PORT_STATUS *PortStatus
  )
{
  EFI_STATUS              Status;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->GetRootHubPortStatus (UsbBus->Usb2Hc, PortIndex, PortStatus);
  } else {
    Status = UsbBus->UsbHc->GetRootHubPortStatus (UsbBus->UsbHc, PortIndex, PortStatus);
  }

  return Status;
}


/**
  Set the root hub port feature

  @param  UsbBus           The USB bus driver
  @param  PortIndex        The port index
  @param  Feature          The port feature to set

  @retval EFI_SUCCESS      The port feature is set
  @retval Others           Failed to set port feature

**/
EFI_STATUS
UsbHcSetRootHubPortFeature (
  IN USB_BUS              *UsbBus,
  IN UINT8                PortIndex,
  IN EFI_USB_PORT_FEATURE Feature
  )
{
  EFI_STATUS              Status;


  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->SetRootHubPortFeature (UsbBus->Usb2Hc, PortIndex, Feature);
  } else {
    Status = UsbBus->UsbHc->SetRootHubPortFeature (UsbBus->UsbHc, PortIndex, Feature);
  }

  return Status;
}


/**
  Clear the root hub port feature

  @param  UsbBus           The USB bus driver
  @param  PortIndex        The port index
  @param  Feature          The port feature to clear

  @retval EFI_SUCCESS      The port feature is clear
  @retval Others           Failed to clear port feature

**/
EFI_STATUS
UsbHcClearRootHubPortFeature (
  IN USB_BUS              *UsbBus,
  IN UINT8                PortIndex,
  IN EFI_USB_PORT_FEATURE Feature
  )
{
  EFI_STATUS              Status;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->ClearRootHubPortFeature (UsbBus->Usb2Hc, PortIndex, Feature);
  } else {
    Status = UsbBus->UsbHc->ClearRootHubPortFeature (UsbBus->UsbHc, PortIndex, Feature);
  }

  return Status;
}


/**
  Execute a control transfer to the device

  @param  UsbBus           The USB bus driver
  @param  DevAddr          The device address
  @param  DevSpeed         The device speed
  @param  MaxPacket        Maximum packet size of endpoint 0
  @param  Request          The control transfer request
  @param  Direction        The direction of data stage
  @param  Data             The buffer holding data
  @param  DataLength       The length of the data
  @param  TimeOut          Timeout (in ms) to wait until timeout
  @param  Translator       The transaction translator for low/full speed device
  @param  UsbResult        The result of transfer

  @retval EFI_SUCCESS      The control transfer finished without error
  @retval Others           The control transfer failed, reason returned in UsbReslt

**/
EFI_STATUS
UsbHcControlTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN  EFI_USB_DEVICE_REQUEST              *Request,
  IN  EFI_USB_DATA_DIRECTION              Direction,
  IN  OUT VOID                            *Data,
  IN  OUT UINTN                           *DataLength,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *UsbResult
  )
{
  EFI_STATUS              Status;
  BOOLEAN                 IsSlowDevice;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->ControlTransfer (
                               UsbBus->Usb2Hc,
                               DevAddr,
                               DevSpeed,
                               MaxPacket,
                               Request,
                               Direction,
                               Data,
                               DataLength,
                               TimeOut,
                               Translator,
                               UsbResult
                               );

  } else {
    IsSlowDevice = (BOOLEAN)(EFI_USB_SPEED_LOW == DevSpeed);
    Status = UsbBus->UsbHc->ControlTransfer (
                              UsbBus->UsbHc,
                              DevAddr,
                              IsSlowDevice,
                              (UINT8) MaxPacket,
                              Request,
                              Direction,
                              Data,
                              DataLength,
                              TimeOut,
                              UsbResult
                              );
  }

  return Status;
}


/**
  Execute a bulk transfer to the device's endpoint

  @param  UsbBus           The USB bus driver
  @param  DevAddr          The target device address
  @param  EpAddr           The target endpoint address, with direction encoded in
                           bit 7
  @param  DevSpeed         The device's speed
  @param  MaxPacket        The endpoint's max packet size
  @param  BufferNum        The number of data buffer
  @param  Data             Array of pointers to data buffer
  @param  DataLength       The length of data buffer
  @param  DataToggle       On input, the initial data toggle to use, also  return
                           the next toggle on output.
  @param  TimeOut          The time to wait until timeout
  @param  Translator       The transaction translator for low/full speed device
  @param  UsbResult        The result of USB execution

  @retval EFI_SUCCESS      The bulk transfer is finished without error
  @retval Others           Failed to execute bulk transfer, result in UsbResult

**/
EFI_STATUS
UsbHcBulkTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               EpAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN  UINT8                               BufferNum,
  IN  OUT VOID                            *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN  OUT UINTN                           *DataLength,
  IN  OUT UINT8                           *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *UsbResult
  )
{
  EFI_STATUS              Status;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->BulkTransfer (
                               UsbBus->Usb2Hc,
                               DevAddr,
                               EpAddr,
                               DevSpeed,
                               MaxPacket,
                               BufferNum,
                               Data,
                               DataLength,
                               DataToggle,
                               TimeOut,
                               Translator,
                               UsbResult
                               );
  } else {
    Status = UsbBus->UsbHc->BulkTransfer (
                              UsbBus->UsbHc,
                              DevAddr,
                              EpAddr,
                              (UINT8) MaxPacket,
                              *Data,
                              DataLength,
                              DataToggle,
                              TimeOut,
                              UsbResult
                              );
  }

  return Status;
}


/**
  Queue or cancel an asynchronous interrupt transfer

  @param  UsbBus           The USB bus driver
  @param  DevAddr          The target device address
  @param  EpAddr           The target endpoint address, with direction encoded in
                           bit 7
  @param  DevSpeed         The device's speed
  @param  MaxPacket        The endpoint's max packet size
  @param  IsNewTransfer    Whether this is a new request. If not, cancel the old
                           request
  @param  DataToggle       Data toggle to use on input, next toggle on output
  @param  PollingInterval  The interval to poll the interrupt transfer (in ms)
  @param  DataLength       The length of periodical data receive
  @param  Translator       The transaction translator for low/full speed device
  @param  Callback         Function to call when data is received
  @param  Context          The context to the callback

  @retval EFI_SUCCESS      The asynchronous transfer is queued
  @retval Others           Failed to queue the transfer

**/
EFI_STATUS
UsbHcAsyncInterruptTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               EpAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN  BOOLEAN                             IsNewTransfer,
  IN OUT UINT8                            *DataToggle,
  IN  UINTN                               PollingInterval,
  IN  UINTN                               DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     Callback,
  IN  VOID                                *Context OPTIONAL
  )
{
  EFI_STATUS              Status;
  BOOLEAN                 IsSlowDevice;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->AsyncInterruptTransfer (
                               UsbBus->Usb2Hc,
                               DevAddr,
                               EpAddr,
                               DevSpeed,
                               MaxPacket,
                               IsNewTransfer,
                               DataToggle,
                               PollingInterval,
                               DataLength,
                               Translator,
                               Callback,
                               Context
                               );
  } else {
    IsSlowDevice = (BOOLEAN)(EFI_USB_SPEED_LOW == DevSpeed);

    Status = UsbBus->UsbHc->AsyncInterruptTransfer (
                              UsbBus->UsbHc,
                              DevAddr,
                              EpAddr,
                              IsSlowDevice,
                              (UINT8) MaxPacket,
                              IsNewTransfer,
                              DataToggle,
                              PollingInterval,
                              DataLength,
                              Callback,
                              Context
                              );
  }

  return Status;
}


/**
  Execute a synchronous interrupt transfer to the target endpoint

  @param  UsbBus           The USB bus driver
  @param  DevAddr          The target device address
  @param  EpAddr           The target endpoint address, with direction encoded in
                           bit 7
  @param  DevSpeed         The device's speed
  @param  MaxPacket        The endpoint's max packet size
  @param  Data             Pointer to data buffer
  @param  DataLength       The length of data buffer
  @param  DataToggle       On input, the initial data toggle to use, also  return
                           the next toggle on output.
  @param  TimeOut          The time to wait until timeout
  @param  Translator       The transaction translator for low/full speed device
  @param  UsbResult        The result of USB execution

  @retval EFI_SUCCESS      The synchronous interrupt transfer is OK
  @retval Others           Failed to execute the synchronous interrupt transfer

**/
EFI_STATUS
UsbHcSyncInterruptTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               EpAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN OUT VOID                             *Data,
  IN OUT UINTN                            *DataLength,
  IN OUT UINT8                            *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *UsbResult
  )
{
  EFI_STATUS              Status;
  BOOLEAN                 IsSlowDevice;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->SyncInterruptTransfer (
                               UsbBus->Usb2Hc,
                               DevAddr,
                               EpAddr,
                               DevSpeed,
                               MaxPacket,
                               Data,
                               DataLength,
                               DataToggle,
                               TimeOut,
                               Translator,
                               UsbResult
                               );
  } else {
    IsSlowDevice = (BOOLEAN) ((EFI_USB_SPEED_LOW == DevSpeed) ? TRUE : FALSE);
    Status = UsbBus->UsbHc->SyncInterruptTransfer (
                              UsbBus->UsbHc,
                              DevAddr,
                              EpAddr,
                              IsSlowDevice,
                              (UINT8) MaxPacket,
                              Data,
                              DataLength,
                              DataToggle,
                              TimeOut,
                              UsbResult
                              );
  }

  return Status;
}


/**
  Execute a synchronous Isochronous USB transfer

  @param  UsbBus           The USB bus driver
  @param  DevAddr          The target device address
  @param  EpAddr           The target endpoint address, with direction encoded in
                           bit 7
  @param  DevSpeed         The device's speed
  @param  MaxPacket        The endpoint's max packet size
  @param  BufferNum        The number of data buffer
  @param  Data             Array of pointers to data buffer
  @param  DataLength       The length of data buffer
  @param  Translator       The transaction translator for low/full speed device
  @param  UsbResult        The result of USB execution

  @retval EFI_UNSUPPORTED  The isochronous transfer isn't supported now

**/
EFI_STATUS
UsbHcIsochronousTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               EpAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN  UINT8                               BufferNum,
  IN  OUT VOID                            *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN  UINTN                               DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *UsbResult
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Queue an asynchronous isochronous transfer

  @param  UsbBus           The USB bus driver
  @param  DevAddr          The target device address
  @param  EpAddr           The target endpoint address, with direction encoded in
                           bit 7
  @param  DevSpeed         The device's speed
  @param  MaxPacket        The endpoint's max packet size
  @param  BufferNum        The number of data buffer
  @param  Data             Array of pointers to data buffer
  @param  DataLength       The length of data buffer
  @param  Translator       The transaction translator for low/full speed device
  @param  Callback         The function to call when data is transferred
  @param  Context          The context to the callback function

  @retval EFI_UNSUPPORTED  The asynchronous isochronous transfer isn't supported

**/
EFI_STATUS
UsbHcAsyncIsochronousTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               EpAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN  UINT8                               BufferNum,
  IN OUT VOID                             *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN  UINTN                               DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     Callback,
  IN  VOID                                *Context
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Open the USB host controller protocol BY_CHILD

  @param  Bus              The USB bus driver
  @param  Child            The child handle

  @return The open protocol return

**/
EFI_STATUS
UsbOpenHostProtoByChild (
  IN USB_BUS              *Bus,
  IN EFI_HANDLE           Child
  )
{
  EFI_USB_HC_PROTOCOL     *UsbHc;
  EFI_USB2_HC_PROTOCOL    *Usb2Hc;
  EFI_STATUS              Status;

  if (Bus->Usb2Hc != NULL) {
    Status = gBS->OpenProtocol (
                    Bus->HostHandle,
                    &gEfiUsb2HcProtocolGuid,
                    (VOID **) &Usb2Hc,
                    mUsbBusDriverBinding.DriverBindingHandle,
                    Child,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );

  } else {
    Status = gBS->OpenProtocol (
                    Bus->HostHandle,
                    &gEfiUsbHcProtocolGuid,
                    (VOID **) &UsbHc,
                    mUsbBusDriverBinding.DriverBindingHandle,
                    Child,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
  }

  return Status;
}


/**
  Close the USB host controller protocol BY_CHILD

  @param  Bus              The USB bus driver
  @param  Child            The child handle

  @return None

**/
VOID
UsbCloseHostProtoByChild (
  IN USB_BUS              *Bus,
  IN EFI_HANDLE           Child
  )
{
  if (Bus->Usb2Hc != NULL) {
    gBS->CloseProtocol (
           Bus->HostHandle,
           &gEfiUsb2HcProtocolGuid,
           mUsbBusDriverBinding.DriverBindingHandle,
           Child
           );

  } else {
    gBS->CloseProtocol (
           Bus->HostHandle,
           &gEfiUsbHcProtocolGuid,
           mUsbBusDriverBinding.DriverBindingHandle,
           Child
           );
  }
}



/**
  return the current TPL, copied from the EDKII glue lib.

  VOID

  @return Current TPL

**/
EFI_TPL
UsbGetCurrentTpl (
  VOID
  )
{
  EFI_TPL                 Tpl;

  Tpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (Tpl);

  return Tpl;
}


#ifdef EFI_DEBUG
VOID
UsbDebug (
  IN  CHAR8               *Format,
  ...
  )
/*++

Routine Description:

  USB's debug output function.

Arguments:

  Format  - The format parameters to the print
  ...     - The variable length parameters after format

Returns:

  None

--*/
{
  VA_LIST                 Marker;

  VA_START (Marker, Format);
  DebugVPrint (DEBUG_INFO, Format, Marker);
  VA_END (Marker);
}



/**
  USB's error output function.

  @param  Format           The format parameters to the print
  @param  ...              The variable length parameters after format

  @return None

**/
VOID
UsbError (
  IN  CHAR8               *Format,
  ...
  )
{
  VA_LIST                 Marker;

  VA_START (Marker, Format);
  DebugVPrint (DEBUG_ERROR, Format, Marker);
  VA_END (Marker);
}

#endif
