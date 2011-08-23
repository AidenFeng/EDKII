/** @file

  The XHCI register operation routines.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Xhci.h"

/**
  Read 1-byte width XHCI capability register.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the 1-byte width capability register.

  @return The register content read.
  @retval If err, return 0xFF.

**/
UINT8
XhcReadCapReg8 (
  IN  USB_XHCI_DEV        *Xhc,
  IN  UINT32              Offset
  )
{
  UINT8                   Data;
  EFI_STATUS              Status;

  Status = Xhc->PciIo->Mem.Read (
                             Xhc->PciIo,
                             EfiPciIoWidthUint8,
                             XHC_BAR_INDEX,
                             (UINT64) Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcReadCapReg: Pci Io read error - %r at %d\n", Status, Offset));
    Data = 0xFF;
  }

  return Data;
}

/**
  Read 4-bytes width XHCI capability register.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the 4-bytes width capability register.

  @return The register content read.
  @retval If err, return 0xFFFFFFFF.

**/
UINT32
XhcReadCapReg (
  IN  USB_XHCI_DEV        *Xhc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  EFI_STATUS              Status;

  Status = Xhc->PciIo->Mem.Read (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             (UINT64) Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcReadCapReg: Pci Io read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

/**
  Read 4-bytes width XHCI Operational register.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the 4-bytes width operational register.

  @return The register content read.
  @retval If err, return 0xFFFFFFFF.

**/
UINT32
XhcReadOpReg (
  IN  USB_XHCI_DEV        *Xhc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  EFI_STATUS              Status;

  ASSERT (Xhc->CapLength != 0);

  Status = Xhc->PciIo->Mem.Read (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             (UINT64) (Xhc->CapLength + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcReadOpReg: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

/**
  Write the data to the 4-bytes width XHCI operational register.

  @param  Xhc      The XHCI device.
  @param  Offset   The offset of the 4-bytes width operational register.
  @param  Data     The data to write.

**/
VOID
XhcWriteOpReg (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  EFI_STATUS              Status;

  ASSERT (Xhc->CapLength != 0);

  Status = Xhc->PciIo->Mem.Write (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             (UINT64) (Xhc->CapLength + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcWriteOpReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Write the data to the 2-bytes width XHCI operational register.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the 2-bytes width operational register.
  @param  Data         The data to write.

**/
VOID
XhcWriteOpReg16 (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Offset,
  IN UINT16               Data
  )
{
  EFI_STATUS              Status;

  ASSERT (Xhc->CapLength != 0);

  Status = Xhc->PciIo->Mem.Write (
                             Xhc->PciIo,
                             EfiPciIoWidthUint16,
                             XHC_BAR_INDEX,
                             (UINT64) (Xhc->CapLength + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcWriteOpReg16: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Write the data to the 8-bytes width XHCI operational register.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the 8-bytes width operational register.
  @param  Data         The data to write.

**/
VOID
XhcWriteOpReg64 (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Offset,
  IN UINT64               Data
  )
{
  EFI_STATUS              Status;

  ASSERT (Xhc->CapLength != 0);

  Status = Xhc->PciIo->Mem.Write (
                             Xhc->PciIo,
                             EfiPciIoWidthUint64,
                             XHC_BAR_INDEX,
                             (UINT64) (Xhc->CapLength + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcWriteOpReg64: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Read XHCI door bell register.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the door bell register.

  @return The register content read

**/
UINT32
XhcReadDoorBellReg (
  IN  USB_XHCI_DEV        *Xhc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  EFI_STATUS              Status;

  ASSERT (Xhc->DBOff != 0);

  Status = Xhc->PciIo->Mem.Read (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             (UINT64) (Xhc->DBOff + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcReadDoorBellReg: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

/**
  Write the data to the XHCI door bell register.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the door bell register.
  @param  Data         The data to write.

**/
VOID
XhcWriteDoorBellReg (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  EFI_STATUS              Status;

  ASSERT (Xhc->DBOff != 0);

  Status = Xhc->PciIo->Mem.Write (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             (UINT64) (Xhc->DBOff + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcWriteOpReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Read XHCI runtime register.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the runtime register.

  @return The register content read

**/
UINT32
XhcReadRuntimeReg (
  IN  USB_XHCI_DEV        *Xhc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  EFI_STATUS              Status;

  ASSERT (Xhc->RTSOff != 0);

  Status = Xhc->PciIo->Mem.Read (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             (UINT64) (Xhc->RTSOff + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcReadRuntimeReg: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

/**
  Read 8-bytes width XHCI runtime register.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the 8-bytes width runtime register.

  @return The register content read

**/
UINT64
XhcReadRuntimeReg64 (
  IN  USB_XHCI_DEV        *Xhc,
  IN  UINT32              Offset
  )
{
  UINT64                  Data;
  EFI_STATUS              Status;

  ASSERT (Xhc->RTSOff != 0);

  Status = Xhc->PciIo->Mem.Read (
                             Xhc->PciIo,
                             EfiPciIoWidthUint64,
                             XHC_BAR_INDEX,
                             (UINT64) (Xhc->RTSOff + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcReadRuntimeReg64: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFFFFFFFFFF;
  }

  return Data;
}

/**
  Write the data to the XHCI runtime register.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the runtime register.
  @param  Data         The data to write.

**/
VOID
XhcWriteRuntimeReg (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  EFI_STATUS              Status;

  ASSERT (Xhc->RTSOff != 0);

  Status = Xhc->PciIo->Mem.Write (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             (UINT64) (Xhc->RTSOff + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcWriteRuntimeReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Write the data to the 8-bytes width XHCI runtime register.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the 8-bytes width runtime register.
  @param  Data         The data to write.

**/
VOID
XhcWriteRuntimeReg64 (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Offset,
  IN UINT64               Data
  )
{
  EFI_STATUS              Status;

  ASSERT (Xhc->RTSOff != 0);

  Status = Xhc->PciIo->Mem.Write (
                             Xhc->PciIo,
                             EfiPciIoWidthUint64,
                             XHC_BAR_INDEX,
                             (UINT64) (Xhc->RTSOff + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcWriteRuntimeReg64: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Read XHCI extended capability register.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the extended capability register.

  @return The register content read

**/
UINT32
XhcReadExtCapReg (
  IN  USB_XHCI_DEV        *Xhc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  EFI_STATUS              Status;

  ASSERT (Xhc->ExtCapRegBase != 0);

  Status = Xhc->PciIo->Mem.Read (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             (UINT64) (Xhc->ExtCapRegBase + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcReadExtCapReg: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

/**
  Write the data to the XHCI extended capability register.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the extended capability register.
  @param  Data         The data to write.

**/
VOID
XhcWriteExtCapReg (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  EFI_STATUS              Status;

  ASSERT (Xhc->ExtCapRegBase != 0);

  Status = Xhc->PciIo->Mem.Write (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             (UINT64) (Xhc->ExtCapRegBase + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcWriteExtCapReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}


/**
  Set one bit of the runtime register while keeping other bits.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the runtime register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
XhcSetRuntimeRegBit (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = XhcReadRuntimeReg (Xhc, Offset);
  Data |= Bit;
  XhcWriteRuntimeReg (Xhc, Offset, Data);
}

/**
  Clear one bit of the runtime register while keeping other bits.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the runtime register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
XhcClearRuntimeRegBit (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = XhcReadRuntimeReg (Xhc, Offset);
  Data &= ~Bit;
  XhcWriteRuntimeReg (Xhc, Offset, Data);
}

/**
  Set one bit of the operational register while keeping other bits.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the operational register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
XhcSetOpRegBit (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = XhcReadOpReg (Xhc, Offset);
  Data |= Bit;
  XhcWriteOpReg (Xhc, Offset, Data);
}


/**
  Clear one bit of the operational register while keeping other bits.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the operational register.
  @param  Bit          The bit mask of the register to clear.

**/
VOID
XhcClearOpRegBit (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = XhcReadOpReg (Xhc, Offset);
  Data &= ~Bit;
  XhcWriteOpReg (Xhc, Offset, Data);
}

/**
  Wait the operation register's bit as specified by Bit
  to become set (or clear).

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the operation register.
  @param  Bit          The bit of the register to wait for.
  @param  WaitToSet    Wait the bit to set or clear.
  @param  Timeout      The time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The bit successfully changed by host controller.
  @retval EFI_TIMEOUT  The time out occurred.

**/
EFI_STATUS
XhcWaitOpRegBit (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Offset,
  IN UINT32               Bit,
  IN BOOLEAN              WaitToSet,
  IN UINT32               Timeout
  )
{
  UINT32                  Index;

  for (Index = 0; Index < Timeout / XHC_SYNC_POLL_INTERVAL + 1; Index++) {
    if (XHC_REG_BIT_IS_SET (Xhc, Offset, Bit) == WaitToSet) {
      return EFI_SUCCESS;
    }

    gBS->Stall (XHC_SYNC_POLL_INTERVAL);
  }

  return EFI_TIMEOUT;
}

/**
  Set Bios Ownership

  @param  Xhc          The XHCI device.

**/
VOID
XhcSetBiosOwnership (
  IN USB_XHCI_DEV         *Xhc
  )
{
  UINT32                    Buffer;

  DEBUG ((EFI_D_INFO, "XhcSetBiosOwnership: called to set BIOS ownership\n"));

  Buffer = XhcReadExtCapReg (Xhc, Xhc->UsbLegSupOffset);
  Buffer = ((Buffer & (~USBLEGSP_OS_SEMAPHORE)) | USBLEGSP_BIOS_SEMAPHORE);
  XhcWriteExtCapReg (Xhc, Xhc->UsbLegSupOffset, Buffer);
}

/**
  Clear Bios Ownership

  @param  Xhc       The XHCI device.

**/
VOID
XhcClearBiosOwnership (
  IN USB_XHCI_DEV         *Xhc
  )
{
  UINT32                    Buffer;

  DEBUG ((EFI_D_INFO, "XhcClearBiosOwnership: called to clear BIOS ownership\n"));

  Buffer = XhcReadExtCapReg (Xhc, Xhc->UsbLegSupOffset);
  Buffer = ((Buffer & (~USBLEGSP_BIOS_SEMAPHORE)) | USBLEGSP_OS_SEMAPHORE);
  XhcWriteExtCapReg (Xhc, Xhc->UsbLegSupOffset, Buffer);
}

/**
  Calculate the XHCI legacy support capability register offset.

  @param  Xhc     The XHCI device.

  @return The offset of XHCI legacy support capability register.

**/
UINT32
XhcGetLegSupCapAddr (
  IN USB_XHCI_DEV         *Xhc
  )
{
  UINT32 ExtCapOffset;
  UINT8  NextExtCapReg;
  UINT32 Data;

  ExtCapOffset = 0;

  do {
    //
    // Check if the extended capability register's capability id is USB Legacy Support.
    //
    Data = XhcReadExtCapReg (Xhc, ExtCapOffset);
    if ((Data & 0xFF) == 0x1) {
      return ExtCapOffset;
    }
    //
    // If not, then traverse all of the ext capability registers till finding out it.
    //
    NextExtCapReg = (Data >> 8) & 0xFF;
    ExtCapOffset += (NextExtCapReg << 2);
  } while (NextExtCapReg != 0);

  return 0;
}

/**
  Whether the XHCI host controller is halted.

  @param  Xhc     The XHCI device.

  @retval TRUE    The controller is halted.
  @retval FALSE   It isn't halted.

**/
BOOLEAN
XhcIsHalt (
  IN USB_XHCI_DEV         *Xhc
  )
{
  return XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT);
}


/**
  Whether system error occurred.

  @param  Xhc      The XHCI device.

  @retval TRUE     System error happened.
  @retval FALSE    No system error.

**/
BOOLEAN
XhcIsSysError (
  IN USB_XHCI_DEV         *Xhc
  )
{
  return XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HSE);
}

/**
  Reset the XHCI host controller.

  @param  Xhc          The XHCI device.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The XHCI host controller is reset.
  @return Others       Failed to reset the XHCI before Timeout.

**/
EFI_STATUS
XhcResetHC (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  DEBUG ((EFI_D_INFO, "XhcResetHC!\n"));
  //
  // Host can only be reset when it is halt. If not so, halt it
  //
  if (!XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT)) {
    Status = XhcHaltHC (Xhc, Timeout);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  XhcSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RESET);
  Status = XhcWaitOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RESET, FALSE, Timeout);
  return Status;
}


/**
  Halt the XHCI host controller.

  @param  Xhc          The XHCI device.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @return EFI_SUCCESS  The XHCI host controller is halt.
  @return EFI_TIMEOUT  Failed to halt the XHCI before Timeout.

**/
EFI_STATUS
XhcHaltHC (
  IN USB_XHCI_DEV        *Xhc,
  IN UINT32              Timeout
  )
{
  EFI_STATUS              Status;

  XhcClearOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RUN);
  Status = XhcWaitOpRegBit (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT, TRUE, Timeout);
  return Status;
}


/**
  Set the XHCI host controller to run.

  @param  Xhc          The XHCI device.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @return EFI_SUCCESS  The XHCI host controller is running.
  @return EFI_TIMEOUT  Failed to set the XHCI to run before Timeout.

**/
EFI_STATUS
XhcRunHC (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  XhcSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RUN);
  Status = XhcWaitOpRegBit (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT, FALSE, Timeout);
  return Status;
}

