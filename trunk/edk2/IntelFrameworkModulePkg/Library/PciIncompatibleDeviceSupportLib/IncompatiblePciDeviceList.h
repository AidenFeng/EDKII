/** @file
  The incompatible PCI device list template.

Copyright (c) 2006 - 2009, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

**/

#ifndef _EFI_INCOMPATIBLE_PCI_DEVICE_LIST_H
#define _EFI_INCOMPATIBLE_PCI_DEVICE_LIST_H

#include <Library/PciIncompatibleDeviceSupportLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>


#define PCI_DEVICE_ID(VendorId, DeviceId, Revision, SubVendorId, SubDeviceId) \
    VendorId, DeviceId, Revision, SubVendorId, SubDeviceId

#define PCI_BAR_TYPE_IO   ACPI_ADDRESS_SPACE_TYPE_IO
#define PCI_BAR_TYPE_MEM  ACPI_ADDRESS_SPACE_TYPE_MEM

#define DEVICE_INF_TAG    0xFFF2
#define DEVICE_RES_TAG    0xFFF1
#define LIST_END_TAG      0x0000

//
// descriptor for access width of incompatible PCI device
//
typedef struct {
  UINT64                         AccessType;
  UINT64                         AccessWidth;
  EFI_PCI_REGISTER_ACCESS_DATA   PciRegisterAccessData;
} EFI_PCI_REGISTER_ACCESS_DESCRIPTOR;

//
// descriptor for register value of incompatible PCI device
//
typedef struct {
  UINT64                         AccessType;
  UINT64                         Offset;
  EFI_PCI_REGISTER_VALUE_DATA    PciRegisterValueData;
} EFI_PCI_REGISTER_VALUE_DESCRIPTOR;

//
// the incompatible PCI devices list for ACPI resource
//
GLOBAL_REMOVE_IF_UNREFERENCED UINT64 gIncompatiblePciDeviceListForResource[] = {
  //
  // DEVICE_INF_TAG,
  // PCI_DEVICE_ID (VendorID, DeviceID, Revision, SubVendorId, SubDeviceId),
  // DEVICE_RES_TAG,
  // ResType,  GFlag , SFlag,   Granularity,  RangeMin,
  // RangeMax, Offset, AddrLen
  //

  //
  // Sample Device 1
  //
  //DEVICE_INF_TAG,
  //PCI_DEVICE_ID(0xXXXX, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  //DEVICE_RES_TAG,
  //PCI_BAR_TYPE_IO,
  //PCI_ACPI_UNUSED,
  //PCI_ACPI_UNUSED,
  //PCI_ACPI_UNUSED,
  //PCI_ACPI_UNUSED,
  //PCI_BAR_EVEN_ALIGN,
  //PCI_BAR_ALL,
  //PCI_BAR_NOCHANGE,

  //
  // Sample Device 2
  //
  //DEVICE_INF_TAG,
  //PCI_DEVICE_ID(0xXXXX, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  //DEVICE_RES_TAG,
  //PCI_BAR_TYPE_IO,
  //PCI_ACPI_UNUSED,
  //PCI_ACPI_UNUSED,
  //PCI_ACPI_UNUSED,
  //PCI_ACPI_UNUSED,
  //PCI_BAR_EVEN_ALIGN,
  //PCI_BAR_ALL,
  //PCI_BAR_NOCHANGE,

  //
  // The end of the list
  //
  LIST_END_TAG
};

//
// the incompatible PCI devices list for the values of configuration registers
//
GLOBAL_REMOVE_IF_UNREFERENCED UINT64 gIncompatiblePciDeviceListForRegister[] = {
  //
  // DEVICE_INF_TAG,
  // PCI_DEVICE_ID (VendorID, DeviceID, Revision, SubVendorId, SubDeviceId),
  // PCI_RES_TAG,
  // PCI_ACCESS_TYPE, PCI_CONFIG_ADDRESS,
  // AND_VALUE, OR_VALUE

  //
  // Sample Device 1
  //
  //DEVICE_INF_TAG,
  //PCI_DEVICE_ID(0xXXXX, 0xXXXX, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  //DEVICE_RES_TAG,
  //PCI_REGISTER_READ,
  //PCI_CAPBILITY_POINTER_OFFSET,
  //0xffffff00,
  //VALUE_NOCARE,

  //
  // Sample Device 2
  //
  //DEVICE_INF_TAG,
  //PCI_DEVICE_ID(0xXXXX, 0xXXXX, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  //DEVICE_RES_TAG,
  //PCI_REGISTER_READ,
  //PCI_CAPBILITY_POINTER_OFFSET,
  //0xffffff00,
  //VALUE_NOCARE,

  //
  // The end of the list
  //
  LIST_END_TAG
};

//
// the incompatible PCI devices list for the access width of configuration registers
//
GLOBAL_REMOVE_IF_UNREFERENCED UINT64 gDeviceListForAccessWidth[] = {
  //
  // DEVICE_INF_TAG,
  // PCI_DEVICE_ID (VendorID, DeviceID, Revision, SubVendorId, SubDeviceId),
  // DEVICE_RES_TAG,
  // PCI_ACCESS_TYPE, PCI_ACCESS_WIDTH,
  // START_ADDRESS, END_ADDRESS,
  // ACTUAL_PCI_ACCESS_WIDTH,
  //

  //
  // Sample Device
  //
  //DEVICE_INF_TAG,
  //PCI_DEVICE_ID(0xXXXX, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE, DEVICE_ID_NOCARE),
  //DEVICE_RES_TAG,
  //PCI_REGISTER_READ,
  //EfiPciWidthUint8,
  //0,
  //0xFF,
  //EfiPciWidthUint32,
  //

  //
  // The end of the list
  //
  LIST_END_TAG
};

#endif
