/** @file
  I/O Library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  IoLib.c

**/

//
// Globle varible to cache pointer to CpuIo protocol.
//
STATIC EFI_CPU_IO_PROTOCOL          *mCpuIo = NULL;

/**
  The constructor function caches the pointer to CpuIo protocol.

  The constructor function locates CpuIo protocol from protocol database.
  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
IoLibConstructor (
  IN      EFI_HANDLE                ImageHandle,
  IN      EFI_SYSTEM_TABLE          *SystemTable
  )
{
  EFI_STATUS                        Status;

  Status = gBS->LocateProtocol (&gEfiCpuIoProtocolGuid, NULL, (VOID**) &mCpuIo);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Reads registers in the EFI CPU I/O space.

  Reads the I/O port specified by Port with registers width specified by Width.
  The read value is returned. If such operations are not supported, then ASSERT().
  This function must guarantee that all I/O read and write operations are serialized.

  @param  Port          The base address of the I/O operation.
                        The caller is responsible for aligning the Address if required.
  @param  Width         The width of the I/O operation.

  @return Data read from registers in the EFI CPU I/O space.

**/
UINT64
EFIAPI
IoReadWorker (
  IN      UINTN                     Port,
  IN      EFI_CPU_IO_PROTOCOL_WIDTH Width
  )
{
  EFI_STATUS                        Status;
  UINT64                            Data;

  Status = mCpuIo->Io.Read (mCpuIo, Width, Port, 1, &Data);
  ASSERT_EFI_ERROR (Status);

  return Data;
}

/**
  Writes registers in the EFI CPU I/O space.

  Writes the I/O port specified by Port with registers width and value specified by Width
  and Data respectively.  Data is returned. If such operations are not supported, then ASSERT().
  This function must guarantee that all I/O read and write operations are serialized.

  @param  Port          The base address of the I/O operation.
                        The caller is responsible for aligning the Address if required.
  @param  Width         The width of the I/O operation.
  @param  Data          The value to write to the I/O port.

  @return The paramter of Data.

**/
UINT64
EFIAPI
IoWriteWorker (
  IN      UINTN                     Port,
  IN      EFI_CPU_IO_PROTOCOL_WIDTH Width,
  IN      UINT64                    Data
  )
{
  EFI_STATUS                        Status;

  Status = mCpuIo->Io.Write (mCpuIo, Width, Port, 1, &Data);
  ASSERT_EFI_ERROR (Status);

  return Data;
}

/**
  Reads memory-mapped registers in the EFI system memory space.

  Reads the MMIO registers specified by Address with registers width specified by Width.
  The read value is returned. If such operations are not supported, then ASSERT().
  This function must guarantee that all MMIO read and write operations are serialized.

  @param  Address       The MMIO register to read.
                        The caller is responsible for aligning the Address if required.
  @param  Width         The width of the I/O operation.

  @return Data read from registers in the EFI system memory space.

**/
UINT64
EFIAPI
MmioReadWorker (
  IN      UINTN                     Address,
  IN      EFI_CPU_IO_PROTOCOL_WIDTH Width
  )
{
  EFI_STATUS                        Status;
  UINT64                            Data;

  Status = mCpuIo->Mem.Read (mCpuIo, Width, Address, 1, &Data);
  ASSERT_EFI_ERROR (Status);

  return Data;
}

/**
  Writes memory-mapped registers in the EFI system memory space.

  Writes the MMIO registers specified by Address with registers width and value specified by Width
  and Data respectively. Data is returned. If such operations are not supported, then ASSERT().
  This function must guarantee that all MMIO read and write operations are serialized.

  @param  Address       The MMIO register to read.
                        The caller is responsible for aligning the Address if required.
  @param  Width         The width of the I/O operation.

  @return Data read from registers in the EFI system memory space.

**/
UINT64
EFIAPI
MmioWriteWorker (
  IN      UINTN                     Address,
  IN      EFI_CPU_IO_PROTOCOL_WIDTH Width,
  IN      UINT64                    Data
  )
{
  EFI_STATUS                        Status;

  Status = mCpuIo->Mem.Write (mCpuIo, Width, Address, 1, &Data);
  ASSERT_EFI_ERROR (Status);

  return Data;
}

/**
  Reads an 8-bit I/O port.

  Reads the 8-bit I/O port specified by Port. The 8-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 8-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to read.

  @return The value read.

**/
UINT8
EFIAPI
IoRead8 (
  IN      UINTN                     Port
  )
{
  return (UINT8)IoReadWorker (Port, EfiCpuIoWidthUint8);
}

/**
  Writes an 8-bit I/O port.

  Writes the 8-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 8-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
UINT8
EFIAPI
IoWrite8 (
  IN      UINTN                     Port,
  IN      UINT8                     Value
  )
{
  return (UINT8)IoWriteWorker (Port, EfiCpuIoWidthUint8, Value);
}

/**
  Reads a 16-bit I/O port.

  Reads the 16-bit I/O port specified by Port. The 16-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 16-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to read.

  @return The value read.

**/
UINT16
EFIAPI
IoRead16 (
  IN      UINTN                     Port
  )
{
  //
  // Make sure Port is aligned on a 16-bit boundary.
  //
  ASSERT ((Port & 1) == 0);
  return (UINT16)IoReadWorker (Port, EfiCpuIoWidthUint16);
}

/**
  Writes a 16-bit I/O port.

  Writes the 16-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 16-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
UINT16
EFIAPI
IoWrite16 (
  IN      UINTN                     Port,
  IN      UINT16                    Value
  )
{
  //
  // Make sure Port is aligned on a 16-bit boundary.
  //
  ASSERT ((Port & 1) == 0);
  return (UINT16)IoWriteWorker (Port, EfiCpuIoWidthUint16, Value);
}

/**
  Reads a 32-bit I/O port.

  Reads the 32-bit I/O port specified by Port. The 32-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 32-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to read.

  @return The value read.

**/
UINT32
EFIAPI
IoRead32 (
  IN      UINTN                     Port
  )
{
  //
  // Make sure Port is aligned on a 32-bit boundary.
  //
  ASSERT ((Port & 3) == 0);
  return (UINT32)IoReadWorker (Port, EfiCpuIoWidthUint32);
}

/**
  Writes a 32-bit I/O port.

  Writes the 32-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 32-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
UINT32
EFIAPI
IoWrite32 (
  IN      UINTN                     Port,
  IN      UINT32                    Value
  )
{
  //
  // Make sure Port is aligned on a 32-bit boundary.
  //
  ASSERT ((Port & 3) == 0);
  return (UINT32)IoWriteWorker (Port, EfiCpuIoWidthUint32, Value);
}

/**
  Reads a 64-bit I/O port.

  Reads the 64-bit I/O port specified by Port. The 64-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 64-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to read.

  @return The value read.

**/
UINT64
EFIAPI
IoRead64 (
  IN      UINTN                     Port
  )
{
  //
  // Make sure Port is aligned on a 64-bit boundary.
  //
  ASSERT ((Port & 7) == 0);
  return IoReadWorker (Port, EfiCpuIoWidthUint64);
}

/**
  Writes a 64-bit I/O port.

  Writes the 64-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 64-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
UINT64
EFIAPI
IoWrite64 (
  IN      UINTN                     Port,
  IN      UINT64                    Value
  )
{
  //
  // Make sure Port is aligned on a 64-bit boundary.
  //
  ASSERT ((Port & 7) == 0);
  return IoWriteWorker (Port, EfiCpuIoWidthUint64, Value);
}

/**
  Reads an 8-bit MMIO register.

  Reads the 8-bit MMIO register specified by Address. The 8-bit read value is
  returned. This function must guarantee that all MMIO read and write
  operations are serialized.

  If 8-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to read.

  @return The value read.

**/
UINT8
EFIAPI
MmioRead8 (
  IN      UINTN                     Address
  )
{
  return (UINT8)MmioReadWorker (Address, EfiCpuIoWidthUint8);
}

/**
  Writes an 8-bit MMIO register.

  Writes the 8-bit MMIO register specified by Address with the value specified
  by Value and returns Value. This function must guarantee that all MMIO read
  and write operations are serialized.

  If 8-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to write.
  @param  Value   The value to write to the MMIO register.

**/
UINT8
EFIAPI
MmioWrite8 (
  IN      UINTN                     Address,
  IN      UINT8                     Value
  )
{
  return (UINT8)MmioWriteWorker (Address, EfiCpuIoWidthUint8, Value);
}

/**
  Reads a 16-bit MMIO register.

  Reads the 16-bit MMIO register specified by Address. The 16-bit read value is
  returned. This function must guarantee that all MMIO read and write
  operations are serialized.

  If 16-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to read.

  @return The value read.

**/
UINT16
EFIAPI
MmioRead16 (
  IN      UINTN                     Address
  )
{
  //
  // Make sure Address is aligned on a 16-bit boundary.
  //
  ASSERT ((Address & 1) == 0);
  return (UINT16)MmioReadWorker (Address, EfiCpuIoWidthUint16);
}

/**
  Writes a 16-bit MMIO register.

  Writes the 16-bit MMIO register specified by Address with the value specified
  by Value and returns Value. This function must guarantee that all MMIO read
  and write operations are serialized.

  If 16-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to write.
  @param  Value   The value to write to the MMIO register.

**/
UINT16
EFIAPI
MmioWrite16 (
  IN      UINTN                     Address,
  IN      UINT16                    Value
  )
{
  //
  // Make sure Address is aligned on a 16-bit boundary.
  //
  ASSERT ((Address & 1) == 0);
  return (UINT16)MmioWriteWorker (Address, EfiCpuIoWidthUint16, Value);
}

/**
  Reads a 32-bit MMIO register.

  Reads the 32-bit MMIO register specified by Address. The 32-bit read value is
  returned. This function must guarantee that all MMIO read and write
  operations are serialized.

  If 32-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to read.

  @return The value read.

**/
UINT32
EFIAPI
MmioRead32 (
  IN      UINTN                     Address
  )
{
  //
  // Make sure Address is aligned on a 32-bit boundary.
  //
  ASSERT ((Address & 3) == 0);
  return (UINT32)MmioReadWorker (Address, EfiCpuIoWidthUint32);
}

/**
  Writes a 32-bit MMIO register.

  Writes the 32-bit MMIO register specified by Address with the value specified
  by Value and returns Value. This function must guarantee that all MMIO read
  and write operations are serialized.

  If 32-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to write.
  @param  Value   The value to write to the MMIO register.

**/
UINT32
EFIAPI
MmioWrite32 (
  IN      UINTN                     Address,
  IN      UINT32                    Value
  )
{
  //
  // Make sure Address is aligned on a 32-bit boundary.
  //
  ASSERT ((Address & 3) == 0);
  return (UINT32)MmioWriteWorker (Address, EfiCpuIoWidthUint32, Value);
}

/**
  Reads a 64-bit MMIO register.

  Reads the 64-bit MMIO register specified by Address. The 64-bit read value is
  returned. This function must guarantee that all MMIO read and write
  operations are serialized.

  If 64-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to read.

  @return The value read.

**/
UINT64
EFIAPI
MmioRead64 (
  IN      UINTN                     Address
  )
{
  //
  // Make sure Address is aligned on a 64-bit boundary.
  //
  ASSERT ((Address & 7) == 0);
  return (UINT64)MmioReadWorker (Address, EfiCpuIoWidthUint64);
}

/**
  Writes a 64-bit MMIO register.

  Writes the 64-bit MMIO register specified by Address with the value specified
  by Value and returns Value. This function must guarantee that all MMIO read
  and write operations are serialized.

  If 64-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to write.
  @param  Value   The value to write to the MMIO register.

**/
UINT64
EFIAPI
MmioWrite64 (
  IN      UINTN                     Address,
  IN      UINT64                    Value
  )
{
  //
  // Make sure Address is aligned on a 64-bit boundary.
  //
  ASSERT ((Address & 7) == 0);
  return (UINT64)MmioWriteWorker (Address, EfiCpuIoWidthUint64, Value);
}
