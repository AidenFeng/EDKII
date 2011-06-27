/** @file
  Defines the USB Host Controller PPI that provides I/O services for a USB Host 
  Controller that may be used to access recovery devices.  These interfaces are 
  modeled on the UEFI 2.3 specification EFI_USB2_HOST_CONTROLLER_PROTOCOL.
  Refer to section 16.1 of the UEFI 2.3 Specification for more information on 
  these interfaces.
 
Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_USB_HOST_CONTROLLER_PPI_H_
#define _PEI_USB_HOST_CONTROLLER_PPI_H_

#include <Protocol/Usb2HostController.h>

///
/// Global ID for the PEI_USB_HOST_CONTROLLER_PPI.
///
#define PEI_USB_HOST_CONTROLLER_PPI_GUID \
  { \
    0x652b38a9, 0x77f4, 0x453f, { 0x89, 0xd5, 0xe7, 0xbd, 0xc3, 0x52, 0xfc, 0x53} \
  }

///
/// Forward declaration for the PEI_USB_HOST_CONTROLLER_PPI.
///
typedef struct _PEI_USB_HOST_CONTROLLER_PPI PEI_USB_HOST_CONTROLLER_PPI;

/**
  Initiate a USB control transfer using a specific USB Host controller on the USB bus. 

  @param[in]     PeiServices           The pointer to the PEI Services Table.
  @param[in]     This                  The pointer to this instance of the 
                                       PEI_USB_HOST_CONTROLLER_PPI.
  @param[in]     DeviceAddress         Represents the address of the target device 
                                       on the USB.
  @param[in]     DeviceSpeed           Indicates device speed.
  @param[in]     MaximumPacketLength   Indicates the maximum packet size that the 
                                       default control transfer
                                       endpoint is capable of sending or receiving.
  @param[in]     Request               A pointer to the USB device request that 
                                       will be sent to the USB device.
  @param[in]     TransferDirection     Specifies the data direction for the transfer. 
                                       There are three values available: 
                                       EfiUsbDataIn, EfiUsbDataOut and EfiUsbNoData.
  @param[in,out] Data                  A pointer to the buffer of data that will 
                                       be transmitted to USB device or
                                       received from USB device.
  @param[in,out] DataLength            On input, indicates the size, in bytes, of 
                                       the data buffer specified by Data.
                                       On output, indicates the amount of data 
                                       actually transferred.
  @param[in]     TimeOut               Indicates the maximum time, in milliseconds, 
                                       that the transfer is allowed to complete.
  @param[out]    TransferResult        A pointer to the detailed result information 
                                       generated by this control transfer.

  @retval EFI_DEVICE_ERROR   The control transfer failed due to host controller 
                             or device error.
  @retval EFI_SUCCESS        The control transfer was completed successfully.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_USB_HOST_CONTROLLER_CONTROL_TRANSFER)(
  IN EFI_PEI_SERVICES             **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN     UINT8                    DeviceAddress,
  IN     UINT8                    DeviceSpeed,
  IN     UINT8                    MaximumPacketLength,
  IN     USB_DEVICE_REQUEST       *Request,
  IN     EFI_USB_DATA_DIRECTION   TransferDirection,
  IN OUT VOID                     *Data OPTIONAL,
  IN OUT UINTN                    *DataLength OPTIONAL,
  IN     UINTN                    TimeOut,
  OUT    UINT32                   *TransferResult
  );

/**
  Initiate a USB bulk transfer using a specific USB Host controller on the USB bus. 

  @param[in]     PeiServices           The pointer to the PEI Services Table.
  @param[in]     This                  The pointer to this instance of the 
                                       PEI_USB_HOST_CONTROLLER_PPI.
  @param[in]     DeviceAddress         Represents the address of the target device 
                                       on the USB.
  @param[in]     EndPointAddress       The combination of an endpoint number and 
                                       an endpoint direction of the target USB device.
  @param[in]     MaximumPacketLength   Indicates the maximum packet size the target 
                                       endpoint is capable of sending or receiving.
  @param[in,out] Data                  Array of pointers to the buffers of data 
                                       that will be transmitted to USB device or 
                                       received from USB device.
  @param[in,out] DataLength            When input, indicates the size, in bytes, of  
                                       the data buffers specified by Data. When output,
                                       indicates the data size actually transferred.
  @param[in,out] DataToggle            A pointer to the data toggle value.
  @param[in]     TimeOut               Indicates the maximum time, in milliseconds,
                                       in which the transfer is allowed to complete.
  @param[out]    TransferResult        A pointer to the detailed result information 
                                       of the bulk transfer.

  @retval EFI_SUCCESS           The bulk transfer was completed successfully.
  @retval EFI_DEVICE_ERROR      The bulk transfer failed due to host controller or device error.
                                Caller should check TransferResult for detailed error information.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_USB_HOST_CONTROLLER_BULK_TRANSFER)(
  IN     EFI_PEI_SERVICES             **PeiServices,
  IN     PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN     UINT8                        DeviceAddress,
  IN     UINT8                        EndPointAddress,
  IN     UINT8                        MaximumPacketLength,
  IN OUT VOID                         *Data,
  IN OUT UINTN                        *DataLength,
  IN OUT UINT8                        *DataToggle,
  IN     UINTN                        TimeOut,
  OUT    UINT32                       *TransferResult
  );

/**
  Retrieves the number of root hub ports.

  @param[in]  PeiServices   The pointer to the PEI Services Table.
  @param[in]  This          The pointer to this instance of the 
                            PEI_USB_HOST_CONTROLLER_PPI.
  @param[out] PortNumber    The pointer to the number of the root hub ports.                                
                                
  @retval EFI_SUCCESS           The port number was retrieved successfully.
  @retval EFI_DEVICE_ERROR      An error was encountered while attempting to retrieve 
                                the port number.
  @retval EFI_INVALID_PARAMETER PortNumber is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_USB_HOST_CONTROLLER_GET_ROOTHUB_PORT_NUMBER)(
  IN  EFI_PEI_SERVICES             **PeiServices,
  IN  PEI_USB_HOST_CONTROLLER_PPI  *This,
  OUT UINT8                        *PortNumber
  );

/**
  Retrieves the current status of a USB root hub port.

  @param[in]  PeiServices   The pointer to the PEI Services Table.
  @param[in]  This          The pointer to this instance of the 
                            PEI_USB_HOST_CONTROLLER_PPI.
  @param[in]  PortNumber    Specifies the root hub port from which the status is 
                            to be retrieved.
                            This value is zero based.
  @param[out] PortStatus    A pointer to the current port status bits and port 
                            status change bits.
  
  @retval EFI_SUCCESS           The status of the USB root hub port specified by 
                                PortNumber was returned in PortStatus.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_USB_HOST_CONTROLLER_GET_ROOTHUB_PORT_STATUS)(
  IN  EFI_PEI_SERVICES             **PeiServices,
  IN  PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN  UINT8                        PortNumber,
  OUT EFI_USB_PORT_STATUS          *PortStatus
  );

/**
  Sets a feature for the specified root hub port.

  @param[in] PeiServices   The pointer to the PEI Services Table.
  @param[in] This          The pointer to this instance of the 
                           PEI_USB_HOST_CONTROLLER_PPI.
  @param[in] PortNumber    Specifies the root hub port whose feature is requested 
                           to be set. This value is zero based.
  @param[in] PortFeature   Indicates the feature selector associated with the feature 
                           set request.
  
  @retval EFI_SUCCESS           The feature specified by PortFeature was set for 
                                the USB root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid 
                                for this function.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_USB_HOST_CONTROLLER_SET_ROOTHUB_PORT_FEATURE)(
  IN EFI_PEI_SERVICES             **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN UINT8                        PortNumber,
  IN EFI_USB_PORT_FEATURE         PortFeature
  );

/**
  Clears a feature for the specified root hub port.

  @param[in] PeiServices   The pointer to the PEI Services Table.
  @param[in] This          The pointer to this instance of the 
                           PEI_USB_HOST_CONTROLLER_PPI.
  @param[in] PortNumber    Specifies the root hub port whose feature is
                           requested to be cleared.
  @param[in] PortFeature   Indicates the feature selector associated with the
                           feature clear request.

  @return EFI_SUCCESS             The feature specified by PortFeature was cleared
                                  for the USB root hub port specified by PortNumber.
  @return EFI_INVALID_PARAMETER   PortNumber is invalid or PortFeature is invalid.
  @return EFI_DEVICE_ERROR        Can't read the register.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_USB_HOST_CONTROLLER_CLEAR_ROOTHUB_PORT_FEATURE)(
  IN EFI_PEI_SERVICES             **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN UINT8                        PortNumber,
  IN EFI_USB_PORT_FEATURE         PortFeature
  );

///
/// This PPI contains a set of services to interact with the USB host controller.
/// These interfaces are modeled on the UEFI 2.3 specification protocol
/// EFI_USB2_HOST_CONTROLLER_PROTOCOL.  Refer to section 16.1 of the UEFI 2.3 
/// Specification for more information on these interfaces.
///
struct _PEI_USB_HOST_CONTROLLER_PPI {
  PEI_USB_HOST_CONTROLLER_CONTROL_TRANSFER            ControlTransfer;
  PEI_USB_HOST_CONTROLLER_BULK_TRANSFER               BulkTransfer;
  PEI_USB_HOST_CONTROLLER_GET_ROOTHUB_PORT_NUMBER     GetRootHubPortNumber;
  PEI_USB_HOST_CONTROLLER_GET_ROOTHUB_PORT_STATUS     GetRootHubPortStatus;
  PEI_USB_HOST_CONTROLLER_SET_ROOTHUB_PORT_FEATURE    SetRootHubPortFeature;
  PEI_USB_HOST_CONTROLLER_CLEAR_ROOTHUB_PORT_FEATURE  ClearRootHubPortFeature;
};

extern EFI_GUID gPeiUsbHostControllerPpiGuid;

#endif

