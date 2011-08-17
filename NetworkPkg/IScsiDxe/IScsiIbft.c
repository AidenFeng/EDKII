/** @file
  Implementation for iSCSI Boot Firmware Table publication.

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IScsiImpl.h"

BOOLEAN mIbftInstalled = FALSE;
UINTN   mTableKey;

/**
  Initialize the header of the iSCSI Boot Firmware Table.
  
  @param[out]  Header     The header of the iSCSI Boot Firmware Table.
  @param[in]   OemId      The OEM ID.
  @param[in]   OemTableId The OEM table ID for the iBFT.

**/
VOID
IScsiInitIbfTableHeader (
  OUT EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_HEADER   *Header,
  IN  UINT8                                       *OemId,
  IN  UINT64                                      *OemTableId
  )
{
  Header->Signature = EFI_ACPI_3_0_ISCSI_BOOT_FIRMWARE_TABLE_SIGNATURE;
  Header->Length    = IBFT_HEAP_OFFSET;
  Header->Revision  = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_REVISION;
  Header->Checksum  = 0;

  CopyMem (Header->OemId, OemId, sizeof (Header->OemId));
  CopyMem (&Header->OemTableId, OemTableId, sizeof (UINT64));
}


/**
  Initialize the control section of the iSCSI Boot Firmware Table.

  @param[in]  Table       The ACPI table.

**/
VOID
IScsiInitControlSection (
  IN EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_HEADER  *Table
  )
{
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE  *Control;
  UINTN                                                 NumOffset;

  Control = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE *) (Table + 1);

  Control->Header.StructureId = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE_ID;
  Control->Header.Version     = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE_VERSION;
  Control->Header.Length      = (UINT16) sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE);

  //
  // If in multipathing mode, enable the Boot Failover Flag.
  // If in single path mode, disable it. Mix-model is not allowed.
  //
  // BUGBUG: if Boot Failover Flag is set to 1, the OS installer cannot
  // find the iSCSI mapped disk. So still keep not set for single path mode.
  //
  if (mPrivate->EnableMpio) {
    Control->Header.Flags = 0;
    NumOffset = 2 * (mPrivate->MpioCount - mPrivate->Krb5MpioCount);
  } else {
    NumOffset = 2 * mPrivate->ValidSinglePathCount;
  }

  //
  // Each attempt occupies two offsets: one for the NIC section;
  // the other for the Target section.
  //
  if (NumOffset > 4) {
    //
    // Need expand the control section if more than 2 NIC/Target attempts
    // exist.
    //
    Control->Header.Length = (UINT16) (Control->Header.Length + (NumOffset - 4) * sizeof (UINT16));
  }
}


/**
  Add one item into the heap.

  @param[in, out]  Heap  On input, the current address of the heap. On output, the address of
                         the heap after the item is added.
  @param[in]       Data  The data to add into the heap.
  @param[in]       Len   Length of the Data in byte.

**/
VOID
IScsiAddHeapItem (
  IN OUT UINT8  **Heap,
  IN     VOID   *Data,
  IN     UINTN  Len
  )
{
  //
  // Add one byte for the NULL delimiter.
  //
  *Heap -= Len + 1;

  CopyMem (*Heap, Data, Len);
  *(*Heap + Len) = 0;
}


/**
  Fill the Initiator section of the iSCSI Boot Firmware Table.

  @param[in]       Table    The ACPI table.
  @param[in, out]  Heap     The heap.

**/
VOID
IScsiFillInitiatorSection (
  IN     EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_HEADER  *Table,
  IN OUT UINT8                                      **Heap
  )
{
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE    *Control;
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE  *Initiator;

  Control = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE *) (Table + 1);

  //
  // Initiator section immediately follows the control section.
  //
  Initiator = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE *)
              ((UINT8 *) Control + IBFT_ROUNDUP (Control->Header.Length));

  Control->InitiatorOffset = (UINT16) ((UINTN) Initiator - (UINTN) Table);

  Initiator->Header.StructureId = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE_ID;
  Initiator->Header.Version     = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE_VERSION;
  Initiator->Header.Length      = (UINT16) sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE);
  Initiator->Header.Flags       = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE_FLAG_BLOCK_VALID |
                                  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE_FLAG_BOOT_SELECTED;

  //
  // Fill the iSCSI Initiator Name into the heap.
  //
  IScsiAddHeapItem (Heap, mPrivate->InitiatorName, mPrivate->InitiatorNameLength - 1);

  Initiator->IScsiNameLength  = (UINT16) (mPrivate->InitiatorNameLength - 1);
  Initiator->IScsiNameOffset  = (UINT16) ((UINTN) *Heap - (UINTN) Table);
}


/**
  Map the v4 IP address into v6 IP address.

  @param[in]   V4 The v4 IP address.
  @param[out]  V6 The v6 IP address.

**/
VOID
IScsiMapV4ToV6Addr (
  IN  EFI_IPv4_ADDRESS *V4,
  OUT EFI_IPv6_ADDRESS *V6
  )
{
  UINTN Index;

  ZeroMem (V6, sizeof (EFI_IPv6_ADDRESS));

  V6->Addr[10]  = 0xff;
  V6->Addr[11]  = 0xff;

  for (Index = 0; Index < 4; Index++) {
    V6->Addr[12 + Index] = V4->Addr[Index];
  }
}


/**
  Fill the NIC and target sections in iSCSI Boot Firmware Table.

  @param[in]       Table    The buffer of the ACPI table.
  @param[in, out]  Heap     The heap buffer used to store the variable length
                            parameters such as iSCSI name.

**/
VOID
IScsiFillNICAndTargetSections (
  IN     EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_HEADER  *Table,
  IN OUT UINT8                                      **Heap
  )
{
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE  *Control;
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE      *Nic;
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE   *Target;
  ISCSI_SESSION_CONFIG_NVDATA                           *NvData;
  ISCSI_CHAP_AUTH_CONFIG_NVDATA                         *AuthConfig;
  UINT16                                                *SectionOffset;
  UINTN                                                 Index;
  UINT16                                                Length;
  LIST_ENTRY                                            *Entry;
  ISCSI_ATTEMPT_CONFIG_NVDATA                           *Attempt;
  ISCSI_NIC_INFO                                        *NicInfo;
  BOOLEAN                                               Flag;

  //
  // Get the offset of the first Nic and Target section.
  //
  Control = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE *) (Table + 1);
  Nic     = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE *) ((UINTN) Table +
          Control->InitiatorOffset + IBFT_ROUNDUP (sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE)));
  Target  = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE *) ((UINTN) Nic +
          IBFT_ROUNDUP (sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE)));

  SectionOffset = &Control->NIC0Offset;

  Index = 0;
  Flag  = TRUE;

  NET_LIST_FOR_EACH (Entry, &mPrivate->AttemptConfigs) {
    if (Index == 0) {
      //
      // First entry should be boot selected entry.
      //
      Attempt = IScsiConfigGetAttemptByConfigIndex (mPrivate->BootSelectedIndex);
      if (Attempt == NULL) {
        //
        // First boot selected entry can not be found.
        //
        break;
      }

      ASSERT (Attempt->SessionConfigData.Enabled != ISCSI_DISABLED);

    } else {
      if (Index == 1 && Flag) {
        Entry = mPrivate->AttemptConfigs.ForwardLink;
        Flag = FALSE;
      }

      Attempt = NET_LIST_USER_STRUCT (Entry, ISCSI_ATTEMPT_CONFIG_NVDATA, Link);
      if (Attempt->AttemptConfigIndex == mPrivate->BootSelectedIndex) {
        continue;
      }
    }

    if (Attempt->SessionConfigData.Enabled == ISCSI_DISABLED) {
      continue;
    }

    //
    // Krb5 attempt will not be recorded in iBFT.
    //
    if (Attempt->AuthenticationType == ISCSI_AUTH_TYPE_KRB) {
      continue;
    }

    //
    // If multipath mode is enabled, only the attempts in MPIO will be recorded in iBFT.
    //
    if (mPrivate->EnableMpio && Attempt->SessionConfigData.Enabled != ISCSI_ENABLED_FOR_MPIO) {
      continue;
    }

    //
    // Only the valid attempts will be recorded.
    //
    if (!Attempt->ValidiBFTPath) {
      continue;
    }

    NvData     = &Attempt->SessionConfigData;
    AuthConfig = &Attempt->AuthConfigData.CHAP;

    //
    // Fill the Nic section.
    //

    Nic->Header.StructureId = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_ID;
    Nic->Header.Version     = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_VERSION;
    Nic->Header.Length      = (UINT16) sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE);
    Nic->Header.Index       = (UINT8) Index;
    Nic->Header.Flags       = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_FLAG_BLOCK_VALID |
                            EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_FLAG_GLOBAL;

    if (Index == 0) {
      Nic->Header.Flags    |= EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_FLAG_BOOT_SELECTED;
    }

    if (NvData->InitiatorInfoFromDhcp) {
      Nic->Origin = IpPrefixOriginDhcp;
    } else {
      Nic->Origin = IpPrefixOriginManual;
    }

    if (NvData->IpMode == IP_MODE_IP4 || NvData->IpMode == IP_MODE_AUTOCONFIG) {
      //
      // Get the subnet mask prefix length.
      //
      Nic->SubnetMaskPrefixLength = IScsiGetSubnetMaskPrefixLength (&NvData->SubnetMask);

      //
      // Map the various v4 addresses into v6 addresses.
      //
      IScsiMapV4ToV6Addr (&NvData->LocalIp, &Nic->Ip);
      IScsiMapV4ToV6Addr (&NvData->Gateway, &Nic->Gateway);
      IScsiMapV4ToV6Addr (&Attempt->PrimaryDns.v4, &Nic->PrimaryDns);
      IScsiMapV4ToV6Addr (&Attempt->SecondaryDns.v4, &Nic->SecondaryDns);
      IScsiMapV4ToV6Addr (&Attempt->DhcpServer.v4, &Nic->DhcpServer);

    } else if (NvData->IpMode == IP_MODE_IP6 || NvData->IpMode == IP_MODE_AUTOCONFIG) {
      //
      // TODO: The subnet mask/local ip/gateway/dhcpserver for iBFT-IPv6 needs to be 
      // confirmed with spec owner.
      //

      CopyMem (&Nic->PrimaryDns, &Attempt->PrimaryDns, sizeof (EFI_IPv6_ADDRESS));
      CopyMem (&Nic->SecondaryDns, &Attempt->SecondaryDns, sizeof (EFI_IPv6_ADDRESS));
      //
      // TODO: DHCP server address cannot be retrieved by DHCPv6 process since 
      // DHCP server option is removed.
      //CopyMem (&Nic->DhcpServer, &Attempt->DhcpServer, sizeof (EFI_IPv6_ADDRESS));
      //
    } else {
      ASSERT (FALSE);
    }

    //
    // Get Nic Info: VLAN tag, Mac address, PCI location.
    //
    NicInfo = IScsiGetNicInfoByIndex (Attempt->NicIndex);
    ASSERT (NicInfo != NULL);

    Nic->VLanTag = NicInfo->VlanId;
    CopyMem (Nic->Mac, &NicInfo->PermanentAddress, sizeof (Nic->Mac));
    Nic->PciLocation = (UINT16) ((NicInfo->BusNumber << 8)    |
                                 (NicInfo->DeviceNumber << 3) | NicInfo->FunctionNumber);
    *SectionOffset    = (UINT16) ((UINTN) Nic - (UINTN) Table);
    SectionOffset++;

    //
    // Fill the Target section.
    //

    Target->Header.StructureId  = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_ID;
    Target->Header.Version      = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_VERSION;
    Target->Header.Length       = (UINT16) sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE);
    Target->Header.Index        = (UINT8) Index;
    Target->Header.Flags        = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_FLAG_BLOCK_VALID;

    if (Index == 0) {
      Target->Header.Flags     |= EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_FLAG_BOOT_SELECTED;
    }

    Target->Port                = NvData->TargetPort;

    if (Attempt->AuthenticationType == ISCSI_AUTH_TYPE_CHAP) {
      Target->CHAPType          = AuthConfig->CHAPType;
    } else if (Attempt->AuthenticationType == ISCSI_AUTH_TYPE_NONE) {
      Target->CHAPType          = ISCSI_AUTH_TYPE_NONE;
    }

    Target->NicIndex            = (UINT8) Index;

    if (NvData->IpMode == IP_MODE_IP4 || NvData->IpMode == IP_MODE_AUTOCONFIG) {
      IScsiMapV4ToV6Addr (&NvData->TargetIp.v4, &Target->Ip);
    } else if (NvData->IpMode == IP_MODE_IP6 || NvData->IpMode == IP_MODE_AUTOCONFIG) {
      CopyMem (&Target->Ip, &NvData->TargetIp, sizeof (EFI_IPv6_ADDRESS));
    } else {
      ASSERT (FALSE);
    }

    CopyMem (Target->BootLun, NvData->BootLun, sizeof (Target->BootLun));

    //
    // Target iSCSI Name, CHAP name/secret, reverse CHAP name/secret.
    //
    Length = (UINT16) AsciiStrLen (NvData->TargetName);
    IScsiAddHeapItem (Heap, NvData->TargetName, Length);

    Target->IScsiNameLength = Length;
    Target->IScsiNameOffset = (UINT16) ((UINTN) *Heap - (UINTN) Table);

    if (Attempt->AuthenticationType == ISCSI_AUTH_TYPE_CHAP) {
      //
      // CHAP Name
      //
      Length = (UINT16) AsciiStrLen (AuthConfig->CHAPName);
      IScsiAddHeapItem (Heap, AuthConfig->CHAPName, Length);
      Target->CHAPNameLength  = Length;
      Target->CHAPNameOffset  = (UINT16) ((UINTN) *Heap - (UINTN) Table);

      //
      // CHAP Secret
      //
      Length = (UINT16) AsciiStrLen (AuthConfig->CHAPSecret);
      IScsiAddHeapItem (Heap, AuthConfig->CHAPSecret, Length);
      Target->CHAPSecretLength  = Length;
      Target->CHAPSecretOffset  = (UINT16) ((UINTN) *Heap - (UINTN) Table);

      if (Target->CHAPType == ISCSI_CHAP_MUTUAL) {
        //
        // Reverse CHAP Name.
        //
        Length = (UINT16) AsciiStrLen (AuthConfig->ReverseCHAPName);
        IScsiAddHeapItem (Heap, AuthConfig->ReverseCHAPName, Length);
        Target->ReverseCHAPNameLength = Length;
        Target->ReverseCHAPNameOffset = (UINT16) ((UINTN) *Heap - (UINTN) Table);

        //
        // Reverse CHAP Secret.
        //
        Length = (UINT16) AsciiStrLen (AuthConfig->ReverseCHAPSecret);
        IScsiAddHeapItem (Heap, AuthConfig->ReverseCHAPSecret, Length);
        Target->ReverseCHAPSecretLength = Length;
        Target->ReverseCHAPSecretOffset = (UINT16) ((UINTN) *Heap - (UINTN) Table);
      }
    }

    *SectionOffset = (UINT16) ((UINTN) Target - (UINTN) Table);
    SectionOffset++;

    //
    // Advance to the next NIC/Target pair.
    //
    Nic    = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE *) ((UINTN) Target +
           IBFT_ROUNDUP (sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE)));
    Target = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE *) ((UINTN) Nic +
           IBFT_ROUNDUP (sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE)));

    Index++;
  }
}


/**
  Publish and remove the iSCSI Boot Firmware Table according to the iSCSI
  session status.

**/
VOID
IScsiPublishIbft (
  IN VOID
  )
{
  EFI_STATUS                                    Status;
  EFI_ACPI_TABLE_PROTOCOL                       *AcpiTableProtocol;
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_HEADER     *Table;
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt;
  UINT8                                         *Heap;
  UINT8                                         Checksum;
  UINTN                                         Index;


  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **) &AcpiTableProtocol);
  if (EFI_ERROR (Status)) {
    return ;
  }

  //
  // Find ACPI table RSD_PTR from the system table.
  //
  for (Index = 0, Rsdp = NULL; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpi20TableGuid) ||
      CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpi10TableGuid) ||
      CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpiTableGuid)
      ) {
      //
      // A match was found.
      //
      Rsdp = (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER *) gST->ConfigurationTable[Index].VendorTable;
      break;
    }
  }

  if (Rsdp == NULL) {
    return ;
  } else {
    Rsdt = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN) Rsdp->RsdtAddress;
  }

  if (mIbftInstalled) {
    Status = AcpiTableProtocol->UninstallAcpiTable (
                                  AcpiTableProtocol,
                                  mTableKey
                                  );
    if (EFI_ERROR (Status)) {
      return ;
    }
    mIbftInstalled = FALSE;
  }

  //
  // If there is no valid attempt configuration, just return.
  //
  if ((!mPrivate->EnableMpio && mPrivate->ValidSinglePathCount == 0) ||
      (mPrivate->EnableMpio && mPrivate->MpioCount <= mPrivate->Krb5MpioCount)) {
    return ;
  }

  //
  // Allocate 4k bytes to hold the ACPI table.
  //
  Table = AllocateZeroPool (IBFT_MAX_SIZE);
  if (Table == NULL) {
    return ;
  }

  Heap = (UINT8 *) Table + IBFT_HEAP_OFFSET;

  //
  // Fill in the various section of the iSCSI Boot Firmware Table.
  //
  IScsiInitIbfTableHeader (Table, Rsdt->OemId, &Rsdt->OemTableId);
  IScsiInitControlSection (Table);
  IScsiFillInitiatorSection (Table, &Heap);
  IScsiFillNICAndTargetSections (Table, &Heap);

  Checksum = CalculateCheckSum8((UINT8 *)Table, Table->Length);
  Table->Checksum = Checksum;

  //
  // Install or update the iBFT table.
  //
  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                Table,
                                Table->Length,
                                &mTableKey
                                );
  if (EFI_ERROR(Status)) {
    return;
  }

  mIbftInstalled = TRUE;
  FreePool (Table);
}
