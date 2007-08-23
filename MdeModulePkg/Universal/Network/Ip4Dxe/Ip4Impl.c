/** @file

Copyright (c) 2005 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Ip4Impl.c

Abstract:


**/

#include "Ip4Impl.h"


/**
  Get the IP child's current operational data. This can
  all be used to get the underlying MNP and SNP data.

  @param  This                   The IP4 protocol instance
  @param  Ip4ModeData            The IP4 operation data
  @param  MnpConfigData          The MNP configure data
  @param  SnpModeData            The SNP operation data

  @retval EFI_INVALID_PARAMETER  The parameter is invalid because This == NULL
  @retval EFI_SUCCESS            The operational parameter is returned.
  @retval Others                 Failed to retrieve the IP4 route table.

**/
STATIC
EFI_STATUS
EFIAPI
EfiIp4GetModeData (
  IN  CONST EFI_IP4_PROTOCOL                *This,
  OUT       EFI_IP4_MODE_DATA               *Ip4ModeData,    OPTIONAL
  OUT       EFI_MANAGED_NETWORK_CONFIG_DATA *MnpConfigData,  OPTIONAL
  OUT       EFI_SIMPLE_NETWORK_MODE         *SnpModeData     OPTIONAL
  )
{
  IP4_PROTOCOL              *IpInstance;
  IP4_SERVICE               *IpSb;
  EFI_IP4_CONFIG_DATA       *Config;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;
  IP4_ADDR                  Ip;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl     = NET_RAISE_TPL (NET_TPL_LOCK);
  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);
  IpSb       = IpInstance->Service;

  if (Ip4ModeData != NULL) {
    //
    // IsStarted is "whether the EfiIp4Configure has been called".
    // IsConfigured is "whether the station address has been configured"
    //
    Ip4ModeData->IsStarted     = (BOOLEAN)(IpInstance->State == IP4_STATE_CONFIGED);
    CopyMem (&Ip4ModeData->ConfigData, &IpInstance->ConfigData, sizeof (Ip4ModeData->ConfigData));
    Ip4ModeData->IsConfigured  = FALSE;

    Ip4ModeData->GroupCount    = IpInstance->GroupCount;
    Ip4ModeData->GroupTable    = (EFI_IPv4_ADDRESS *) IpInstance->Groups;

    Ip4ModeData->IcmpTypeCount = 23;
    Ip4ModeData->IcmpTypeList  = mIp4SupportedIcmp;

    Ip4ModeData->RouteTable    = NULL;
    Ip4ModeData->RouteCount    = 0;

    //
    // return the current station address for this IP child. So,
    // the user can get the default address through this. Some
    // application wants to know it station address even it is
    // using the default one, such as a ftp server.
    //
    if (Ip4ModeData->IsStarted) {
      Config  = &Ip4ModeData->ConfigData;

      Ip = HTONL (IpInstance->Interface->Ip);
      NetCopyMem (&Config->StationAddress, &Ip, sizeof (EFI_IPv4_ADDRESS));

      Ip = HTONL (IpInstance->Interface->SubnetMask);
      NetCopyMem (&Config->SubnetMask, &Ip, sizeof (EFI_IPv4_ADDRESS));

      Ip4ModeData->IsConfigured = IpInstance->Interface->Configured;

      //
      // Build a EFI route table for user from the internal route table.
      //
      Status = Ip4BuildEfiRouteTable (IpInstance);

      if (EFI_ERROR (Status)) {
        NET_RESTORE_TPL (OldTpl);
        return Status;
      }

      Ip4ModeData->RouteTable = IpInstance->EfiRouteTable;
      Ip4ModeData->RouteCount = IpInstance->EfiRouteCount;
    }
  }

  if (MnpConfigData != NULL) {
    CopyMem (MnpConfigData, &IpSb->MnpConfigData, sizeof (*MnpConfigData));
  }

  if (SnpModeData != NULL) {
    CopyMem (SnpModeData, &IpSb->SnpMode, sizeof (*SnpModeData));
  }

  NET_RESTORE_TPL (OldTpl);
  return EFI_SUCCESS;
}


/**
  Config the MNP parameter used by IP. The IP driver use one MNP
  child to transmit/receive frames. By default, it configures MNP
  to receive unicast/multicast/broadcast. And it will enable/disable
  the promiscous receive according to whether there is IP child
  enable that or not. If Force isn't false, it will iterate through
  all the IP children to check whether the promiscuous receive
  setting has been changed. If it hasn't been changed, it won't
  reconfigure the MNP. If Force is true, the MNP is configured no
  matter whether that is changed or not.

  @param  IpSb                   The IP4 service instance that is to be changed.
  @param  Force                  Force the configuration or not.

  @retval EFI_SUCCESS            The MNP is successfully configured/reconfigured.
  @retval Others                 Configuration failed.

**/
EFI_STATUS
Ip4ServiceConfigMnp (
  IN IP4_SERVICE            *IpSb,
  IN BOOLEAN                Force
  )
{
  NET_LIST_ENTRY            *Entry;
  NET_LIST_ENTRY            *ProtoEntry;
  IP4_INTERFACE             *IpIf;
  IP4_PROTOCOL              *IpInstance;
  BOOLEAN                   Reconfig;
  BOOLEAN                   PromiscReceive;
  EFI_STATUS                Status;

  Reconfig       = FALSE;
  PromiscReceive = FALSE;

  if (!Force) {
    //
    // Iterate through the IP children to check whether promiscuous
    // receive setting has been changed. Update the interface's receive
    // filter also.
    //
    NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {

      IpIf              = NET_LIST_USER_STRUCT (Entry, IP4_INTERFACE, Link);
      IpIf->PromiscRecv = FALSE;

      NET_LIST_FOR_EACH (ProtoEntry, &IpIf->IpInstances) {
        IpInstance = NET_LIST_USER_STRUCT (ProtoEntry, IP4_PROTOCOL, AddrLink);

        if (IpInstance->ConfigData.AcceptPromiscuous) {
          IpIf->PromiscRecv = TRUE;
          PromiscReceive    = TRUE;
        }
      }
    }

    //
    // If promiscuous receive isn't changed, it isn't necessary to reconfigure.
    //
    if (PromiscReceive == IpSb->MnpConfigData.EnablePromiscuousReceive) {
      return EFI_SUCCESS;
    }

    Reconfig  = TRUE;
    IpSb->MnpConfigData.EnablePromiscuousReceive = PromiscReceive;
  }

  Status = IpSb->Mnp->Configure (IpSb->Mnp, &IpSb->MnpConfigData);

  //
  // recover the original configuration if failed to set the configure.
  //
  if (EFI_ERROR (Status) && Reconfig) {
    IpSb->MnpConfigData.EnablePromiscuousReceive = (BOOLEAN) !PromiscReceive;
  }

  return Status;
}


/**
  The event handle for IP4 auto configuration. If IP is asked
  to reconfigure the default address. The original default
  interface and route table are removed as the default. If there
  is active IP children using the default address, the interface
  will remain valid until all the children have freed their
  references. If IP is signalled when auto configuration is done,
  it will configure the default interface and default route table
  with the configuration information retrieved by IP4_CONFIGURE.

  @param  Event                  The event that is signalled.
  @param  Context                The IP4 service binding instance.

  @return None

**/
VOID
EFIAPI
Ip4AutoConfigCallBack (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  EFI_IP4_CONFIG_PROTOCOL   *Ip4Config;
  EFI_IP4_IPCONFIG_DATA     *Data;
  EFI_IP4_ROUTE_TABLE       *RouteEntry;
  IP4_SERVICE               *IpSb;
  IP4_ROUTE_TABLE           *RouteTable;
  IP4_INTERFACE             *IpIf;
  EFI_STATUS                Status;
  UINTN                     Len;
  UINT32                    Index;

  IpSb      = (IP4_SERVICE *) Context;
  NET_CHECK_SIGNATURE (IpSb, IP4_SERVICE_SIGNATURE);

  Ip4Config = IpSb->Ip4Config;

  //
  // IP is asked to do the reconfiguration. If the default interface
  // has been configured, release the default interface and route
  // table, then create a new one. If there are some IP children
  // using it, the interface won't be physically freed until all the
  // children have released their reference to it. Also remember to
  // restart the receive on the default address. IP4 driver only receive
  // frames on the default address, and when the default interface is
  // freed, Ip4AcceptFrame won't be informed.
  //
  if (Event == IpSb->ReconfigEvent) {

    if (IpSb->DefaultInterface->Configured) {
      IpIf = Ip4CreateInterface (IpSb->Mnp, IpSb->Controller, IpSb->Image);

      if (IpIf == NULL) {
        return;
      }

      RouteTable = Ip4CreateRouteTable ();

      if (RouteTable == NULL) {
        Ip4FreeInterface (IpIf, NULL);
        return;
      }

      Ip4CancelReceive (IpSb->DefaultInterface);
      Ip4FreeInterface (IpSb->DefaultInterface, NULL);
      Ip4FreeRouteTable (IpSb->DefaultRouteTable);

      IpSb->DefaultInterface  = IpIf;
      NetListInsertHead (&IpSb->Interfaces, &IpIf->Link);

      IpSb->DefaultRouteTable = RouteTable;
      Ip4ReceiveFrame (IpIf, NULL, Ip4AccpetFrame, IpSb);
    }

    Ip4Config->Stop (Ip4Config);
    Ip4Config->Start (Ip4Config, IpSb->DoneEvent, IpSb->ReconfigEvent);
    return ;
  }

  //
  // Get the configure data in two steps: get the length then the data.
  //
  Len = 0;

  if (Ip4Config->GetData (Ip4Config, &Len, NULL) != EFI_BUFFER_TOO_SMALL) {
    return ;
  }

  Data = NetAllocatePool (Len);

  if (Data == NULL) {
    return ;
  }

  Status = Ip4Config->GetData (Ip4Config, &Len, Data);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  IpIf = IpSb->DefaultInterface;

  //
  // If the default address has been configured don't change it.
  // This is unlikely to happen if EFI_IP4_CONFIG protocol has
  // informed us to reconfigure each time it wants to change the
  // configuration parameters.
  //
  if (IpIf->Configured) {
    goto ON_EXIT;
  }

  //
  // Set the default interface's address, then add a directed
  // route for it, that is, the route whose nexthop is zero.
  //
  Status = Ip4SetAddress (
             IpIf,
             EFI_NTOHL (Data->StationAddress),
             EFI_NTOHL (Data->SubnetMask)
             );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Ip4AddRoute (
    IpSb->DefaultRouteTable,
    EFI_NTOHL (Data->StationAddress),
    EFI_NTOHL (Data->SubnetMask),
    IP4_ALLZERO_ADDRESS
    );

  //
  // Add routes returned by EFI_IP4_CONFIG protocol.
  //
  for (Index = 0; Index < Data->RouteTableSize; Index++) {
    RouteEntry = &Data->RouteTable[Index];

    Ip4AddRoute (
      IpSb->DefaultRouteTable,
      EFI_NTOHL (RouteEntry->SubnetAddress),
      EFI_NTOHL (RouteEntry->SubnetMask),
      EFI_NTOHL (RouteEntry->GatewayAddress)
      );
  }

  IpSb->State = IP4_SERVICE_CONFIGED;

  Ip4SetVariableData (IpSb);

ON_EXIT:
  NetFreePool (Data);
}


/**
  Start the auto configuration for this IP service instance.
  It will locates the EFI_IP4_CONFIG_PROTOCOL, then start the
  auto configuration.

  @param  IpSb                   The IP4 service instance to configure

  @retval EFI_SUCCESS            The auto configuration is successfull started
  @retval Others                 Failed to start auto configuration.

**/
EFI_STATUS
Ip4StartAutoConfig (
  IN IP4_SERVICE            *IpSb
  )
{
  EFI_IP4_CONFIG_PROTOCOL   *Ip4Config;
  EFI_STATUS                Status;

  if (IpSb->State > IP4_SERVICE_UNSTARTED) {
    return EFI_SUCCESS;
  }

  //
  // Create the DoneEvent and ReconfigEvent to call EFI_IP4_CONFIG
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  NET_TPL_LOCK,
                  Ip4AutoConfigCallBack,
                  IpSb,
                  &IpSb->DoneEvent
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ip4AutoConfigCallBack,
                  IpSb,
                  &IpSb->ReconfigEvent
                  );

  if (EFI_ERROR (Status)) {
    goto CLOSE_DONE_EVENT;
  }

  //
  // Open the EFI_IP4_CONFIG protocol then start auto configure
  //
  Status = gBS->OpenProtocol (
                  IpSb->Controller,
                  &gEfiIp4ConfigProtocolGuid,
                  (VOID **) &Ip4Config,
                  IpSb->Image,
                  IpSb->Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER | EFI_OPEN_PROTOCOL_EXCLUSIVE
                  );

  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto CLOSE_RECONFIG_EVENT;
  }

  Status = Ip4Config->Start (Ip4Config, IpSb->DoneEvent, IpSb->ReconfigEvent);

  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           IpSb->Controller,
           &gEfiIp4ConfigProtocolGuid,
           IpSb->Image,
           IpSb->Controller
           );

    goto CLOSE_RECONFIG_EVENT;
  }

  IpSb->Ip4Config = Ip4Config;
  IpSb->State     = IP4_SERVICE_STARTED;
  return Status;

CLOSE_RECONFIG_EVENT:
  gBS->CloseEvent (IpSb->ReconfigEvent);
  IpSb->ReconfigEvent = NULL;

CLOSE_DONE_EVENT:
  gBS->CloseEvent (IpSb->DoneEvent);
  IpSb->DoneEvent = NULL;

  return Status;
}


/**
  Intiialize the IP4_PROTOCOL structure to the unconfigured states.

  @param  IpSb                   The IP4 service instance.
  @param  IpInstance             The IP4 child instance.

  @return None

**/
VOID
Ip4InitProtocol (
  IN IP4_SERVICE            *IpSb,
  IN IP4_PROTOCOL           *IpInstance
  )
{
  ASSERT ((IpSb != NULL) && (IpInstance != NULL));

  NetZeroMem (IpInstance, sizeof (IP4_PROTOCOL));

  IpInstance->Signature = IP4_PROTOCOL_SIGNATURE;
  CopyMem (&IpInstance->Ip4Proto, &mEfiIp4ProtocolTemplete, sizeof (IpInstance->Ip4Proto));
  IpInstance->State     = IP4_STATE_UNCONFIGED;
  IpInstance->Service   = IpSb;

  NetListInit (&IpInstance->Link);
  NetMapInit  (&IpInstance->RxTokens);
  NetMapInit  (&IpInstance->TxTokens);
  NetListInit (&IpInstance->Received);
  NetListInit (&IpInstance->Delivered);
  NetListInit (&IpInstance->AddrLink);

  NET_RECYCLE_LOCK_INIT (&IpInstance->RecycleLock);
}


/**
  Configure the IP4 child. If the child is already configured,
  change the configuration parameter. Otherwise configure it
  for the first time. The caller should validate the configuration
  before deliver them to it. It also don't do configure NULL.

  @param  IpInstance             The IP4 child to configure.
  @param  Config                 The configure data.

  @retval EFI_SUCCESS            The IP4 child is successfully configured.
  @retval EFI_DEVICE_ERROR       Failed to free the pending transive or to
                                 configure  underlying MNP or other errors.
  @retval EFI_NO_MAPPING         The IP4 child is configured to use default
                                 address, but the default address hasn't been
                                 configured. The IP4 child doesn't need to be
                                 reconfigured when default address is configured.

**/
EFI_STATUS
Ip4ConfigProtocol (
  IN  IP4_PROTOCOL        *IpInstance,
  IN  EFI_IP4_CONFIG_DATA *Config
  )
{
  IP4_SERVICE               *IpSb;
  IP4_INTERFACE             *IpIf;
  EFI_STATUS                Status;
  IP4_ADDR                  Ip;
  IP4_ADDR                  Netmask;

  IpSb = IpInstance->Service;

  //
  // User is changing packet filters. It must be stopped
  // before the station address can be changed.
  //
  if (IpInstance->State == IP4_STATE_CONFIGED) {
    //
    // Cancel all the pending transmit/receive from upper layer
    //
    Status = Ip4Cancel (IpInstance, NULL);

    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    CopyMem (&IpInstance->ConfigData, Config, sizeof (IpInstance->ConfigData));
    return EFI_SUCCESS;
  }

  //
  // Configure a fresh IP4 protocol instance. Create a route table.
  // Each IP child has its own route table, which may point to the
  // default table if it is using default address.
  //
  Status                 = EFI_OUT_OF_RESOURCES;
  IpInstance->RouteTable = Ip4CreateRouteTable ();

  if (IpInstance->RouteTable == NULL) {
    return Status;
  }

  //
  // Set up the interface.
  //
  NetCopyMem (&Ip, &Config->StationAddress, sizeof (IP4_ADDR));
  NetCopyMem (&Netmask, &Config->SubnetMask, sizeof (IP4_ADDR));

  Ip      = NTOHL (Ip);
  Netmask = NTOHL (Netmask);

  if (!Config->UseDefaultAddress) {
    //
    // Find whether there is already an interface with the same
    // station address. All the instances with the same station
    // address shares one interface.
    //
    IpIf = Ip4FindStationAddress (IpSb, Ip, Netmask);

    if (IpIf != NULL) {
      NET_GET_REF (IpIf);

    } else {
      IpIf = Ip4CreateInterface (IpSb->Mnp, IpSb->Controller, IpSb->Image);

      if (IpIf == NULL) {
        goto ON_ERROR;
      }

      Status = Ip4SetAddress (IpIf, Ip, Netmask);

      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        Ip4FreeInterface (IpIf, IpInstance);
        goto ON_ERROR;
      }

      NetListInsertTail (&IpSb->Interfaces, &IpIf->Link);
    }

    //
    // Add a route to this connected network in the route table
    //
    Ip4AddRoute (IpInstance->RouteTable, Ip, Netmask, IP4_ALLZERO_ADDRESS);

  } else {
    //
    // Use the default address. If the default configuration hasn't
    // been started, start it.
    //
    if (IpSb->State == IP4_SERVICE_UNSTARTED) {
      Status = Ip4StartAutoConfig (IpSb);

      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }
    }

    IpIf = IpSb->DefaultInterface;
    NET_GET_REF (IpSb->DefaultInterface);

    //
    // If default address is used, so is the default route table.
    // Any route set by the instance has the precedence over the
    // routes in the default route table. Link the default table
    // after the instance's table. Routing will search the local
    // table first.
    //
    NET_GET_REF (IpSb->DefaultRouteTable);
    IpInstance->RouteTable->Next = IpSb->DefaultRouteTable;
  }

  IpInstance->Interface = IpIf;
  NetListInsertTail (&IpIf->IpInstances, &IpInstance->AddrLink);

  CopyMem (&IpInstance->ConfigData, Config, sizeof (IpInstance->ConfigData));
  IpInstance->State       = IP4_STATE_CONFIGED;

  //
  // Although EFI_NO_MAPPING is an error code, the IP child has been
  // successfully configured and doesn't need reconfiguration when
  // default address is acquired.
  //
  if (Config->UseDefaultAddress && IP4_NO_MAPPING (IpInstance)) {
    return EFI_NO_MAPPING;
  }

  return EFI_SUCCESS;

ON_ERROR:
  Ip4FreeRouteTable (IpInstance->RouteTable);
  IpInstance->RouteTable = NULL;
  return Status;
}


/**
  Clean up the IP4 child, release all the resources used by it.

  @param  IpInstance             The IP4 child to clean up.

  @retval EFI_SUCCESS            The IP4 child is cleaned up
  @retval EFI_DEVICE_ERROR       Some resources failed to be released

**/
EFI_STATUS
Ip4CleanProtocol (
  IN  IP4_PROTOCOL          *IpInstance
  )
{
  if (EFI_ERROR (Ip4Cancel (IpInstance, NULL))) {
    return EFI_DEVICE_ERROR;
  }

  if (EFI_ERROR (Ip4Groups (IpInstance, FALSE, NULL))) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Some packets haven't been recycled. It is because either the
  // user forgets to recycle the packets, or because the callback
  // hasn't been called. Just leave it alone.
  //
  if (!NetListIsEmpty (&IpInstance->Delivered)) {
    ;
  }

  if (IpInstance->Interface != NULL) {
    NetListRemoveEntry (&IpInstance->AddrLink);
    Ip4FreeInterface (IpInstance->Interface, IpInstance);
    IpInstance->Interface = NULL;
  }

  if (IpInstance->RouteTable != NULL) {
    if (IpInstance->RouteTable->Next != NULL) {
      Ip4FreeRouteTable (IpInstance->RouteTable->Next);
    }

    Ip4FreeRouteTable (IpInstance->RouteTable);
    IpInstance->RouteTable = NULL;
  }

  if (IpInstance->EfiRouteTable != NULL) {
    NetFreePool (IpInstance->EfiRouteTable);
    IpInstance->EfiRouteTable = NULL;
    IpInstance->EfiRouteCount = 0;
  }

  if (IpInstance->Groups != NULL) {
    NetFreePool (IpInstance->Groups);
    IpInstance->Groups      = NULL;
    IpInstance->GroupCount  = 0;
  }

  NetMapClean (&IpInstance->TxTokens);

  NetMapClean (&IpInstance->RxTokens);

  return EFI_SUCCESS;
}


/**
  Validate that Ip/Netmask pair is OK to be used as station
  address. Only continuous netmasks are supported. and check
  that StationAddress is a unicast address on the newtwork.

  @param  Ip                     The IP address to validate
  @param  Netmask                The netmaks of the IP

  @retval TRUE                   The Ip/Netmask pair is valid
  @retval FALSE                  The

**/
BOOLEAN
Ip4StationAddressValid (
  IN IP4_ADDR               Ip,
  IN IP4_ADDR               Netmask
  )
{
  IP4_ADDR                  NetBrdcastMask;
  INTN                      Len;
  INTN                      Type;

  //
  // Only support the station address with 0.0.0.0/0 to enable DHCP client.
  //
  if (Netmask == IP4_ALLZERO_ADDRESS) {
    return (BOOLEAN) (Ip == IP4_ALLZERO_ADDRESS);
  }

  //
  // Only support the continuous net masks
  //
  if ((Len = NetGetMaskLength (Netmask)) == IP4_MASK_NUM) {
    return FALSE;
  }

  //
  // Station address can't be class D or class E address
  //
  if ((Type = NetGetIpClass (Ip)) > IP4_ADDR_CLASSC) {
    return FALSE;
  }

  //
  // Station address can't be subnet broadcast/net broadcast address
  //
  if ((Ip == (Ip & Netmask)) || (Ip == (Ip | ~Netmask))) {
    return FALSE;
  }

  NetBrdcastMask = mIp4AllMasks[NET_MIN (Len, Type << 3)];

  if (Ip == (Ip | ~NetBrdcastMask)) {
    return FALSE;
  }

  return TRUE;
}


/**
  Configure the EFI_IP4_PROTOCOL instance. If IpConfigData is NULL,
  the instance is cleaned up. If the instance hasn't been configure
  before, it will be initialized. Otherwise, the filter setting of
  the instance is updated.

  @param  This                   The IP4 child to configure
  @param  IpConfigData           The configuration to apply. If NULL, clean it up.

  @retval EFI_INVALID_PARAMETER  The parameter is invalid
  @retval EFI_NO_MAPPING         The default address hasn't been configured and the
                                 instance wants to use it.
  @retval EFI_SUCCESS            The instance is configured.

**/
STATIC
EFI_STATUS
EFIAPI
EfiIp4Configure (
  IN EFI_IP4_PROTOCOL       *This,
  IN EFI_IP4_CONFIG_DATA    *IpConfigData       OPTIONAL
  )
{
  IP4_PROTOCOL              *IpInstance;
  EFI_IP4_CONFIG_DATA       *Current;
  EFI_TPL                   OldTpl;
  EFI_STATUS                Status;
  BOOLEAN                   AddrOk;
  IP4_ADDR                  IpAddress;
  IP4_ADDR                  SubnetMask;

  //
  // First, validate the parameters
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);
  OldTpl     = NET_RAISE_TPL (NET_TPL_LOCK);

  //
  // Validate the configuration first.
  //
  if (IpConfigData != NULL) {
    //
    // This implementation doesn't support RawData
    //
    if (IpConfigData->RawData) {
      Status = EFI_UNSUPPORTED;
      goto ON_EXIT;
    }


    NetCopyMem (&IpAddress, &IpConfigData->StationAddress, sizeof (IP4_ADDR));
    NetCopyMem (&SubnetMask, &IpConfigData->SubnetMask, sizeof (IP4_ADDR));

    IpAddress  = NTOHL (IpAddress);
    SubnetMask = NTOHL (SubnetMask);

    //
    // Check whether the station address is a valid unicast address
    //
    if (!IpConfigData->UseDefaultAddress) {
      AddrOk = Ip4StationAddressValid (IpAddress, SubnetMask);

      if (!AddrOk) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }
    }

    //
    // User can only update packet filters when already configured.
    // If it wants to change the station address, it must configure(NULL)
    // the instance first.
    //
    if (IpInstance->State == IP4_STATE_CONFIGED) {
      Current = &IpInstance->ConfigData;

      if (Current->UseDefaultAddress != IpConfigData->UseDefaultAddress) {
        Status = EFI_ALREADY_STARTED;
        goto ON_EXIT;
      }

      if (!Current->UseDefaultAddress &&
         (!EFI_IP4_EQUAL (Current->StationAddress, IpConfigData->StationAddress) ||
          !EFI_IP4_EQUAL (Current->SubnetMask, IpConfigData->SubnetMask))) {
        Status = EFI_ALREADY_STARTED;
        goto ON_EXIT;
      }

      if (Current->UseDefaultAddress && IP4_NO_MAPPING (IpInstance)) {
        return EFI_NO_MAPPING;
      }
    }
  }

  //
  // Configure the instance or clean it up.
  //
  if (IpConfigData != NULL) {
    Status = Ip4ConfigProtocol (IpInstance, IpConfigData);
  } else {
    Status = Ip4CleanProtocol (IpInstance);

    //
    // Don't change the state if it is DESTORY, consider the following
    // valid sequence: Mnp is unloaded-->Ip Stopped-->Udp Stopped,
    // Configure (ThisIp, NULL). If the state is changed to UNCONFIGED,
    // the unload fails miserably.
    //
    if (IpInstance->State == IP4_STATE_CONFIGED) {
      IpInstance->State = IP4_STATE_UNCONFIGED;
    }
  }

  //
  // Update the MNP's configure data. Ip4ServiceConfigMnp will check
  // whether it is necessary to reconfigure the MNP.
  //
  Ip4ServiceConfigMnp (IpInstance->Service, FALSE);

  //
  // Update the variable data.
  //
  Ip4SetVariableData (IpInstance->Service);

ON_EXIT:
  NET_RESTORE_TPL (OldTpl);
  return Status;

}


/**
  Change the IP4 child's multicast setting. The caller
  should make sure that the parameters is valid.

  @param  IpInstance             The IP4 child to change the setting.
  @param  JoinFlag               TRUE to join the group, otherwise leave it
  @param  GroupAddress           The target group address

  @retval EFI_ALREADY_STARTED    Want to join the group, but already a member of it
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate some resources.
  @retval EFI_DEVICE_ERROR       Failed to set the group configuraton
  @retval EFI_SUCCESS            Successfully updated the group setting.
  @retval EFI_NOT_FOUND          Try to leave the group which it isn't a member.

**/
EFI_STATUS
Ip4Groups (
  IN IP4_PROTOCOL           *IpInstance,
  IN BOOLEAN                JoinFlag,
  IN EFI_IPv4_ADDRESS       *GroupAddress       OPTIONAL
  )
{
  IP4_ADDR                  *Members;
  IP4_ADDR                  Group;
  UINT32                    Index;

  //
  // Add it to the instance's Groups, and join the group by IGMP.
  // IpInstance->Groups is in network byte order. IGMP operates in
  // host byte order
  //
  if (JoinFlag) {
    NetCopyMem (&Group, GroupAddress, sizeof (IP4_ADDR));

    for (Index = 0; Index < IpInstance->GroupCount; Index++) {
      if (IpInstance->Groups[Index] == Group) {
        return EFI_ALREADY_STARTED;
      }
    }

    Members = Ip4CombineGroups (IpInstance->Groups, IpInstance->GroupCount, Group);

    if (Members == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (EFI_ERROR (Ip4JoinGroup (IpInstance, NTOHL (Group)))) {
      NetFreePool (Members);
      return EFI_DEVICE_ERROR;
    }

    if (IpInstance->Groups != NULL) {
      NetFreePool (IpInstance->Groups);
    }

    IpInstance->Groups = Members;
    IpInstance->GroupCount++;

    return EFI_SUCCESS;
  }

  //
  // Leave the group. Leave all the groups if GroupAddress is NULL.
  // Must iterate from the end to the beginning because the GroupCount
  // is decreamented each time an address is removed..
  //
  for (Index = IpInstance->GroupCount; Index > 0 ; Index--) {
    Group = IpInstance->Groups[Index - 1];

    if ((GroupAddress == NULL) || EFI_IP4_EQUAL (Group, *GroupAddress)) {
      if (EFI_ERROR (Ip4LeaveGroup (IpInstance, NTOHL (Group)))) {
        return EFI_DEVICE_ERROR;
      }

      Ip4RemoveGroupAddr (IpInstance->Groups, IpInstance->GroupCount, Group);
      IpInstance->GroupCount--;

      if (IpInstance->GroupCount == 0) {
        ASSERT (Index == 1);

        NetFreePool (IpInstance->Groups);
        IpInstance->Groups = NULL;
      }

      if (GroupAddress != NULL) {
        return EFI_SUCCESS;
      }
    }
  }

  return ((GroupAddress != NULL) ? EFI_NOT_FOUND : EFI_SUCCESS);
}


/**
  Change the IP4 child's multicast setting. If JoinFlag is true,
  the child wants to join the group. Otherwise it wants to leave
  the group. If JoinFlag is false, and GroupAddress is NULL,
  it will leave all the groups which is a member.

  @param  This                   The IP4 child to change the setting.
  @param  JoinFlag               TRUE to join the group, otherwise leave it.
  @param  GroupAddress           The target group address

  @retval EFI_INVALID_PARAMETER  The parameters are invalid
  @retval EFI_SUCCESS            The group setting has been changed.
  @retval Otherwise              It failed to change the setting.

**/
STATIC
EFI_STATUS
EFIAPI
EfiIp4Groups (
  IN EFI_IP4_PROTOCOL       *This,
  IN BOOLEAN                JoinFlag,
  IN EFI_IPv4_ADDRESS       *GroupAddress     OPTIONAL
  )
{
  IP4_PROTOCOL              *IpInstance;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;
  IP4_ADDR                  McastIp;

  if ((This == NULL) || (JoinFlag && (GroupAddress == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  if (GroupAddress != NULL) {
    NetCopyMem (&McastIp, GroupAddress, sizeof (IP4_ADDR));

    if (!IP4_IS_MULTICAST (NTOHL (McastIp))) {
      return EFI_INVALID_PARAMETER;
    }
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);
  OldTpl     = NET_RAISE_TPL (NET_TPL_LOCK);

  if (IpInstance->State != IP4_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  if (IpInstance->ConfigData.UseDefaultAddress && IP4_NO_MAPPING (IpInstance)) {
    Status = EFI_NO_MAPPING;
    goto ON_EXIT;
  }

  Status = Ip4Groups (IpInstance, JoinFlag, GroupAddress);

ON_EXIT:
  NET_RESTORE_TPL (OldTpl);
  return Status;
}


/**
  Modify the IP child's route table. Each instance has its own
  route table.

  @param  This                   The IP4 child to modify the route
  @param  DeleteRoute            TRUE to delete the route, otherwise add it
  @param  SubnetAddress          The destination network
  @param  SubnetMask             The destination network's mask
  @param  GatewayAddress         The next hop address.

  @retval EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval EFI_SUCCESS            The route table is successfully modified.
  @retval Others                 Failed to modify the route table

**/
STATIC
EFI_STATUS
EFIAPI
EfiIp4Routes (
  IN EFI_IP4_PROTOCOL       *This,
  IN BOOLEAN                DeleteRoute,
  IN EFI_IPv4_ADDRESS       *SubnetAddress,
  IN EFI_IPv4_ADDRESS       *SubnetMask,
  IN EFI_IPv4_ADDRESS       *GatewayAddress
  )
{
  IP4_PROTOCOL              *IpInstance;
  IP4_INTERFACE             *IpIf;
  IP4_ADDR                  Dest;
  IP4_ADDR                  Netmask;
  IP4_ADDR                  Nexthop;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;

  //
  // First, validate the parameters
  //
  if ((This == NULL) || (SubnetAddress == NULL) ||
      (SubnetMask == NULL) || (GatewayAddress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);
  OldTpl     = NET_RAISE_TPL (NET_TPL_LOCK);

  if (IpInstance->State != IP4_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  if (IpInstance->ConfigData.UseDefaultAddress && IP4_NO_MAPPING (IpInstance)) {
    Status = EFI_NO_MAPPING;
    goto ON_EXIT;
  }

  NetCopyMem (&Dest, SubnetAddress, sizeof (IP4_ADDR));
  NetCopyMem (&Netmask, SubnetMask, sizeof (IP4_ADDR));
  NetCopyMem (&Nexthop, GatewayAddress, sizeof (IP4_ADDR));

  Dest    = NTOHL (Dest);
  Netmask = NTOHL (Netmask);
  Nexthop = NTOHL (Nexthop);

  IpIf    = IpInstance->Interface;

  if (!IP4_IS_VALID_NETMASK (Netmask)) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  //
  // the gateway address must be a unicast on the connected network if not zero.
  //
  if ((Nexthop != IP4_ALLZERO_ADDRESS) &&
      (!IP4_NET_EQUAL (Nexthop, IpIf->Ip, IpIf->SubnetMask) ||
        IP4_IS_BROADCAST (Ip4GetNetCast (Nexthop, IpIf)))) {

    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  if (DeleteRoute) {
    Status = Ip4DelRoute (IpInstance->RouteTable, Dest, Netmask, Nexthop);
  } else {
    Status = Ip4AddRoute (IpInstance->RouteTable, Dest, Netmask, Nexthop);
  }

ON_EXIT:
  NET_RESTORE_TPL (OldTpl);
  return Status;
}


/**
  Check whether the user's token or event has already
  been enqueue on IP4's list.

  @param  Map                    The container of either user's transmit or receive
                                 token.
  @param  Item                   Current item to check against
  @param  Context                The Token to check againist.

  @retval EFI_ACCESS_DENIED      The token or event has already been enqueued in IP
  @retval EFI_SUCCESS            The current item isn't the same token/event as the
                                 context.

**/
STATIC
EFI_STATUS
Ip4TokenExist (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Context
  )
{
  EFI_IP4_COMPLETION_TOKEN  *Token;
  EFI_IP4_COMPLETION_TOKEN  *TokenInItem;

  Token       = (EFI_IP4_COMPLETION_TOKEN *) Context;
  TokenInItem = (EFI_IP4_COMPLETION_TOKEN *) Item->Key;

  if ((Token == TokenInItem) || (Token->Event == TokenInItem->Event)) {
    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}


/**
  Validate the user's token against current station address.

  @param  Token                  User's token to validate
  @param  IpIf                   The IP4 child's interface.

  @retval EFI_INVALID_PARAMETER  Some parameters are invalid
  @retval EFI_BAD_BUFFER_SIZE    The user's option/data is too long.
  @retval EFI_SUCCESS            The token is OK

**/
STATIC
EFI_STATUS
Ip4TxTokenValid (
  IN EFI_IP4_COMPLETION_TOKEN   *Token,
  IN IP4_INTERFACE              *IpIf
  )
{
  EFI_IP4_TRANSMIT_DATA     *TxData;
  EFI_IP4_OVERRIDE_DATA     *Override;
  IP4_ADDR                  Src;
  IP4_ADDR                  Gateway;
  UINT32                    Offset;
  UINT32                    Index;
  UINT32                    HeadLen;

  if ((Token == NULL) || (Token->Event == NULL) || (Token->Packet.TxData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TxData = Token->Packet.TxData;

  //
  // Check the IP options: no more than 40 bytes and format is OK
  //
  if (TxData->OptionsLength != 0) {
    if ((TxData->OptionsLength > 40) || (TxData->OptionsBuffer == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    if (!Ip4OptionIsValid (TxData->OptionsBuffer, TxData->OptionsLength, FALSE)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Check the fragment table: no empty fragment, and length isn't bogus
  //
  if ((TxData->TotalDataLength == 0) || (TxData->FragmentCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Offset = TxData->TotalDataLength;

  for (Index = 0; Index < TxData->FragmentCount; Index++) {
    if ((TxData->FragmentTable[Index].FragmentBuffer == NULL) ||
        (TxData->FragmentTable[Index].FragmentLength == 0)) {

      return EFI_INVALID_PARAMETER;
    }

    Offset -= TxData->FragmentTable[Index].FragmentLength;
  }

  if (Offset != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check the source and gateway: they must be a valid unicast.
  // Gateway must also be on the connected network.
  //
  if (TxData->OverrideData) {
    Override = TxData->OverrideData;

    NetCopyMem (&Src, &Override->SourceAddress, sizeof (IP4_ADDR));
    NetCopyMem (&Gateway, &Override->GatewayAddress, sizeof (IP4_ADDR));

    Src     = NTOHL (Src);
    Gateway = NTOHL (Gateway);

    if ((NetGetIpClass (Src) > IP4_ADDR_CLASSC) ||
        (Src == IP4_ALLONE_ADDRESS) ||
        IP4_IS_BROADCAST (Ip4GetNetCast (Src, IpIf))) {

      return EFI_INVALID_PARAMETER;
    }

    //
    // If gateway isn't zero, it must be a unicast address, and
    // on the connected network.
    //
    if ((Gateway != IP4_ALLZERO_ADDRESS) &&
        ((NetGetIpClass (Gateway) > IP4_ADDR_CLASSC) ||
         !IP4_NET_EQUAL (Gateway, IpIf->Ip, IpIf->SubnetMask) ||
         IP4_IS_BROADCAST (Ip4GetNetCast (Gateway, IpIf)))) {

      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Check the packet length: Head length and packet length all has a limit
  //
  HeadLen = sizeof (IP4_HEAD) + ((TxData->OptionsLength + 3) &~0x03);

  if ((HeadLen > IP4_MAX_HEADLEN) ||
      (TxData->TotalDataLength + HeadLen > IP4_MAX_PACKET_SIZE)) {

    return EFI_BAD_BUFFER_SIZE;
  }

  return EFI_SUCCESS;
}


/**
  The callback function for the net buffer which wraps the user's
  transmit token. Although it seems this function is pretty simple,
  there are some subtle things.
  When user requests the IP to transmit a packet by passing it a
  token, the token is wrapped in an IP4_TXTOKEN_WRAP and the data
  is wrapped in an net buffer. the net buffer's Free function is
  set to Ip4FreeTxToken. The Token and token wrap are added to the
  IP child's TxToken map. Then the buffer is passed to Ip4Output for
  transmission. If something error happened before that, the buffer
  is freed, which in turn will free the token wrap. The wrap may
  have been added to the TxToken map or not, and the user's event
  shouldn't be fired because we are still in the EfiIp4Transmit. If
  the buffer has been sent by Ip4Output, it should be removed from
  the TxToken map and user's event signaled. The token wrap and buffer
  are bound together. Check the comments in Ip4Output for information
  about IP fragmentation.

  @param  Context                The token's wrap

  @return None

**/
STATIC
VOID
Ip4FreeTxToken (
  IN VOID                   *Context
  )
{
  IP4_TXTOKEN_WRAP          *Wrap;
  NET_MAP_ITEM              *Item;

  Wrap = (IP4_TXTOKEN_WRAP *) Context;

  //
  // Find the token in the instance's map. EfiIp4Transmit put the
  // token to the map. If that failed, NetMapFindKey will return NULL.
  //
  Item = NetMapFindKey (&Wrap->IpInstance->TxTokens, Wrap->Token);

  if (Item != NULL) {
    NetMapRemoveItem (&Wrap->IpInstance->TxTokens, Item, NULL);
  }

  if (Wrap->Sent) {
    gBS->SignalEvent (Wrap->Token->Event);
  }

  NetFreePool (Wrap);
}


/**
  The callback function to Ip4Output to update the transmit status.

  @param  Ip4Instance            The Ip4Instance that request the transmit.
  @param  Packet                 The user's transmit request
  @param  IoStatus               The result of the transmission
  @param  Flag                   Not used during transmission
  @param  Context                The token's wrap.

  @return None

**/
STATIC
VOID
Ip4OnPacketSent (
  IP4_PROTOCOL              *Ip4Instance,
  NET_BUF                   *Packet,
  EFI_STATUS                IoStatus,
  UINT32                    Flag,
  VOID                      *Context
  )
{
  IP4_TXTOKEN_WRAP          *Wrap;

  //
  // This is the transmission request from upper layer,
  // not the IP4 driver itself.
  //
  ASSERT (Ip4Instance != NULL);

  //
  // The first fragment of the packet has been sent. Update
  // the token's status. That is, if fragmented, the transmit's
  // status is the first fragment's status. The Wrap will be
  // release when all the fragments are release. Check the comments
  // in Ip4FreeTxToken and Ip4Output for information.
  //
  Wrap                = (IP4_TXTOKEN_WRAP *) Context;
  Wrap->Token->Status = IoStatus;

  NetbufFree (Wrap->Packet);
}


/**
  Transmit the user's data asynchronously. When transmission
  completed,the Token's status is updated and its event signalled.

  @param  This                   The IP4 child instance
  @param  Token                  The user's transmit token, which contains user's
                                 data, the result and an event to signal when
                                 completed.

  @retval EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval EFI_NOT_STARTED        The IP4 child hasn't been started.
  @retval EFI_SUCCESS            The user's data has been successfully enqueued
                                 for transmission.

**/
STATIC
EFI_STATUS
EFIAPI
EfiIp4Transmit (
  IN EFI_IP4_PROTOCOL         *This,
  IN EFI_IP4_COMPLETION_TOKEN *Token
  )
{
  IP4_SERVICE               *IpSb;
  IP4_PROTOCOL              *IpInstance;
  IP4_INTERFACE             *IpIf;
  IP4_TXTOKEN_WRAP          *Wrap;
  EFI_IP4_TRANSMIT_DATA     *TxData;
  EFI_IP4_CONFIG_DATA       *Config;
  EFI_IP4_OVERRIDE_DATA     *Override;
  IP4_HEAD                  Head;
  IP4_ADDR                  GateWay;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;
  BOOLEAN                   DontFragment;
  UINT32                    HeadLen;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);

  if (IpInstance->State != IP4_STATE_CONFIGED) {
    return EFI_NOT_STARTED;
  }

  OldTpl  = NET_RAISE_TPL (NET_TPL_LOCK);

  IpSb    = IpInstance->Service;
  IpIf    = IpInstance->Interface;
  Config  = &IpInstance->ConfigData;

  if (Config->UseDefaultAddress && IP4_NO_MAPPING (IpInstance)) {
    Status = EFI_NO_MAPPING;
    goto ON_EXIT;
  }

  //
  // make sure that token is properly formated
  //
  Status = Ip4TxTokenValid (Token, IpIf);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Check whether the token or signal already existed.
  //
  if (EFI_ERROR (NetMapIterate (&IpInstance->TxTokens, Ip4TokenExist, Token))) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // Build the IP header, need to fill in the Tos, TotalLen, Id,
  // fragment, Ttl, protocol, Src, and Dst.
  //
  TxData = Token->Packet.TxData;

  NetCopyMem (&Head.Dst, &TxData->DestinationAddress, sizeof (IP4_ADDR));
  Head.Dst = NTOHL (Head.Dst);

  if (TxData->OverrideData) {
    Override      = TxData->OverrideData;
    Head.Protocol = Override->Protocol;
    Head.Tos      = Override->TypeOfService;
    Head.Ttl      = Override->TimeToLive;
    DontFragment  = Override->DoNotFragment;

    NetCopyMem (&Head.Src, &Override->SourceAddress, sizeof (IP4_ADDR));
    NetCopyMem (&GateWay, &Override->GatewayAddress, sizeof (IP4_ADDR));

    Head.Src = NTOHL (Head.Src);
    GateWay  = NTOHL (GateWay);
  } else {
    Head.Src      = IpIf->Ip;
    GateWay       = IP4_ALLZERO_ADDRESS;
    Head.Protocol = Config->DefaultProtocol;
    Head.Tos      = Config->TypeOfService;
    Head.Ttl      = Config->TimeToLive;
    DontFragment  = Config->DoNotFragment;
  }

  Head.Fragment = IP4_HEAD_FRAGMENT_FIELD (DontFragment, FALSE, 0);
  HeadLen       = sizeof (IP4_HEAD) + ((TxData->OptionsLength + 3) &~0x03);

  //
  // If don't fragment and fragment needed, return error
  //
  if (DontFragment && (TxData->TotalDataLength + HeadLen > IpSb->SnpMode.MaxPacketSize)) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto ON_EXIT;
  }

  //
  // OK, it survives all the validation check. Wrap the token in
  // a IP4_TXTOKEN_WRAP and the data in a netbuf
  //
  Status = EFI_OUT_OF_RESOURCES;
  Wrap   = NetAllocatePool (sizeof (IP4_TXTOKEN_WRAP));
  if (Wrap == NULL) {
    goto ON_EXIT;
  }

  Wrap->IpInstance  = IpInstance;
  Wrap->Token       = Token;
  Wrap->Sent        = FALSE;
  Wrap->Life        = IP4_US_TO_SEC (Config->TransmitTimeout);
  Wrap->Packet      = NetbufFromExt (
                        (NET_FRAGMENT *) TxData->FragmentTable,
                        TxData->FragmentCount,
                        IP4_MAX_HEADLEN,
                        0,
                        Ip4FreeTxToken,
                        Wrap
                        );

  if (Wrap->Packet == NULL) {
    NetFreePool (Wrap);
    goto ON_EXIT;
  }

  Token->Status = EFI_NOT_READY;

  if (EFI_ERROR (NetMapInsertTail (&IpInstance->TxTokens, Token, Wrap))) {
    //
    // NetbufFree will call Ip4FreeTxToken, which in turn will
    // free the IP4_TXTOKEN_WRAP. Now, the token wrap hasn't been
    // enqueued.
    //
    NetbufFree (Wrap->Packet);
    goto ON_EXIT;
  }

  Status = Ip4Output (
             IpSb,
             IpInstance,
             Wrap->Packet,
             &Head,
             TxData->OptionsBuffer,
             TxData->OptionsLength,
             GateWay,
             Ip4OnPacketSent,
             Wrap
             );

  if (EFI_ERROR (Status)) {
    NetbufFree (Wrap->Packet);
    goto ON_EXIT;
  }

  //
  // Mark the packet sent, so when Ip4FreeTxToken is called, it
  // will signal the upper layer.
  //
  Wrap->Sent = TRUE;

ON_EXIT:
  NET_RESTORE_TPL (OldTpl);
  return Status;
}


/**
  Receive a packet for the upper layer. If there are packets
  pending on the child's receive queue, the receive request
  will be fulfilled immediately. Otherwise, the request is
  enqueued. When receive request is completed, the status in
  the Token is updated and its event is signalled.

  @param  This                   The IP4 child to receive packet.
  @param  Token                  The user's receive token

  @retval EFI_INVALID_PARAMETER  The token is invalid.
  @retval EFI_NOT_STARTED        The IP4 child hasn't been started
  @retval EFI_ACCESS_DENIED      The token or event is already queued.
  @retval EFI_SUCCESS            The receive request has been issued.

**/
STATIC
EFI_STATUS
EFIAPI
EfiIp4Receive (
  IN EFI_IP4_PROTOCOL         *This,
  IN EFI_IP4_COMPLETION_TOKEN *Token
  )
{
  IP4_PROTOCOL              *IpInstance;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;

  //
  // First validate the parameters
  //
  if ((This == NULL) || (Token == NULL) || (Token->Event == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);

  OldTpl = NET_RAISE_TPL (NET_TPL_LOCK);

  if (IpInstance->State != IP4_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  //
  // Current Udp implementation creates an IP child for each Udp child.
  // It initates a asynchronous receive immediately no matter whether
  // there is no mapping or not. Disable this for now.
  //
#if 0
  if (Config->UseDefaultAddress && IP4_NO_MAPPING (IpInstance)) {
    Status = EFI_NO_MAPPING;
    goto ON_EXIT;
  }
#endif

  //
  // Check whether the toke is already on the receive queue.
  //
  Status = NetMapIterate (&IpInstance->RxTokens, Ip4TokenExist, Token);

  if (EFI_ERROR (Status)) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // Queue the token then check whether there is pending received packet.
  //
  Status = NetMapInsertTail (&IpInstance->RxTokens, Token, NULL);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = Ip4InstanceDeliverPacket (IpInstance);

ON_EXIT:
  NET_RESTORE_TPL (OldTpl);
  return Status;
}


/**
  Cancel the transmitted but not recycled packet. If a matching
  token is found, it will call Ip4CancelPacket to cancel the
  packet. Ip4CancelPacket will cancel all the fragments of the
  packet. When all the fragments are freed, the IP4_TXTOKEN_WRAP
  will be deleted from the Map, and user's event signalled.
  Because Ip4CancelPacket and other functions are all called in
  line, so, after Ip4CancelPacket returns, the Item has been freed.

  @param  Map                    The IP4 child's transmit queue
  @param  Item                   The current transmitted packet to test.
  @param  Context                The user's token to cancel.

  @retval EFI_SUCCESS            Continue to check the next Item.
  @retval EFI_ABORTED            The user's Token (Token != NULL) is cancelled.

**/
STATIC
EFI_STATUS
Ip4CancelTxTokens (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Context
  )
{
  EFI_IP4_COMPLETION_TOKEN  *Token;
  IP4_TXTOKEN_WRAP          *Wrap;

  Token = (EFI_IP4_COMPLETION_TOKEN *) Context;

  //
  // Return EFI_SUCCESS to check the next item in the map if
  // this one doesn't match.
  //
  if ((Token != NULL) && (Token != Item->Key)) {
    return EFI_SUCCESS;
  }

  Wrap = (IP4_TXTOKEN_WRAP *) Item->Value;
  ASSERT (Wrap != NULL);

  //
  // Don't access the Item, Wrap and Token's members after this point.
  // Item and wrap has been freed. And we no longer own the Token.
  //
  Ip4CancelPacket (Wrap->IpInstance->Interface, Wrap->Packet, EFI_ABORTED);

  //
  // If only one item is to be cancel, return EFI_ABORTED to stop
  // iterating the map any more.
  //
  if (Token != NULL) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}


/**
  Cancel the receive request. This is quiet simple, because
  it is only enqueued in our local receive map.

  @param  Map                    The IP4 child's receive queue
  @param  Item                   Current receive request to cancel.
  @param  Context                The user's token to cancel

  @retval EFI_SUCCESS            Continue to check the next receive request on the
                                 queue.
  @retval EFI_ABORTED            The user's token (token != NULL) has been
                                 cancelled.

**/
STATIC
EFI_STATUS
Ip4CancelRxTokens (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Context
  )
{
  EFI_IP4_COMPLETION_TOKEN  *Token;
  EFI_IP4_COMPLETION_TOKEN  *This;

  Token = (EFI_IP4_COMPLETION_TOKEN *) Context;
  This  = Item->Key;

  if ((Token != NULL) && (Token != This)) {
    return EFI_SUCCESS;
  }

  NetMapRemoveItem (Map, Item, NULL);

  This->Status        = EFI_ABORTED;
  This->Packet.RxData = NULL;
  gBS->SignalEvent (This->Event);

  if (Token != NULL) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}


/**
  Cancel the user's receive/transmit request.

  @param  IpInstance             The IP4 child
  @param  Token                  The token to cancel. If NULL, all token will be
                                 cancelled.

  @retval EFI_SUCCESS            The token is cancelled
  @retval EFI_NOT_FOUND          The token isn't found on either the
                                 transmit/receive queue
  @retval EFI_DEVICE_ERROR       Not all token is cancelled when Token is NULL.

**/
EFI_STATUS
Ip4Cancel (
  IN IP4_PROTOCOL             *IpInstance,
  IN EFI_IP4_COMPLETION_TOKEN *Token          OPTIONAL
  )
{
  EFI_STATUS                Status;

  //
  // First check the transmitted packet. Ip4CancelTxTokens returns
  // EFI_ABORTED to mean that the token has been cancelled when
  // token != NULL. So, return EFI_SUCCESS for this condition.
  //
  Status = NetMapIterate (&IpInstance->TxTokens, Ip4CancelTxTokens, Token);

  if (EFI_ERROR (Status)) {
    if ((Token != NULL) && (Status == EFI_ABORTED)) {
      return EFI_SUCCESS;
    }

    return Status;
  }

  //
  // Check the receive queue. Ip4CancelRxTokens also returns EFI_ABORT
  // for Token!=NULL and it is cancelled.
  //
  Status = NetMapIterate (&IpInstance->RxTokens, Ip4CancelRxTokens, Token);

  if (EFI_ERROR (Status)) {
    if ((Token != NULL) && (Status == EFI_ABORTED)) {
      return EFI_SUCCESS;
    }

    return Status;
  }

  //
  // OK, if the Token is found when Token != NULL, the NetMapIterate
  // will return EFI_ABORTED, which has been interrupted as EFI_SUCCESS.
  //
  if (Token != NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // If Token == NULL, cancel all the tokens. return error if no
  // all of them are cancelled.
  //
  if (!NetMapIsEmpty (&IpInstance->TxTokens) ||
      !NetMapIsEmpty (&IpInstance->RxTokens)) {

    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/**
  Cancel the queued receive/transmit requests. If Token is NULL,
  all the queued requests will be cancelled. It just validate
  the parameter then pass them to Ip4Cancel.

  @param  This                   The IP4 child to cancel the request
  @param  Token                  The token to cancel, if NULL, cancel all.

  @retval EFI_INVALID_PARAMETER  This is NULL
  @retval EFI_NOT_STARTED        The IP4 child hasn't been configured.
  @retval EFI_NO_MAPPING         The IP4 child is configured to use the default,
                                 but the default address hasn't been acquired.
  @retval EFI_SUCCESS            The Token is cancelled.

**/
STATIC
EFI_STATUS
EFIAPI
EfiIp4Cancel (
  IN EFI_IP4_PROTOCOL         *This,
  IN EFI_IP4_COMPLETION_TOKEN *Token    OPTIONAL
  )
{
  IP4_PROTOCOL              *IpInstance;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);

  OldTpl = NET_RAISE_TPL (NET_TPL_LOCK);

  if (IpInstance->State != IP4_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  if (IpInstance->ConfigData.UseDefaultAddress && IP4_NO_MAPPING (IpInstance)) {
    Status = EFI_NO_MAPPING;
    goto ON_EXIT;
  }

  Status = Ip4Cancel (IpInstance, Token);

ON_EXIT:
  NET_RESTORE_TPL (OldTpl);
  return Status;
}


/**
  Poll the network stack. The EFI network stack is poll based. There
  is no interrupt support for the network interface card.

  @param  This                   The IP4 child to poll through

  @retval EFI_INVALID_PARAMETER  The parameter is invalid
  @retval EFI_NOT_STARTED        The IP4 child hasn't been configured.

**/
STATIC
EFI_STATUS
EFIAPI
EfiIp4Poll (
  IN EFI_IP4_PROTOCOL       *This
  )
{
  IP4_PROTOCOL                  *IpInstance;
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance = IP4_INSTANCE_FROM_PROTOCOL (This);

  if (IpInstance->State == IP4_STATE_UNCONFIGED) {
    return EFI_NOT_STARTED;
  }

  Mnp = IpInstance->Service->Mnp;

  //
  // Don't lock the Poll function to enable the deliver of
  // the packet polled up.
  //
  return Mnp->Poll (Mnp);
}

EFI_IP4_PROTOCOL
mEfiIp4ProtocolTemplete = {
  EfiIp4GetModeData,
  EfiIp4Configure,
  EfiIp4Groups,
  EfiIp4Routes,
  EfiIp4Transmit,
  EfiIp4Receive,
  EfiIp4Cancel,
  EfiIp4Poll
};


/**
  Decrease the life of the transmitted packets. If it is
  decreased to zero, cancel the packet. This function is
  called by Ip4packetTimerTicking which time out both the
  received-but-not-delivered and transmitted-but-not-recycle
  packets.

  @param  Map                    The IP4 child's transmit map.
  @param  Item                   Current transmitted packet
  @param  Context                Not used.

  @retval EFI_SUCCESS            Always returns EFI_SUCCESS

**/
EFI_STATUS
Ip4SentPacketTicking (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Context
  )
{
  IP4_TXTOKEN_WRAP          *Wrap;

  Wrap = (IP4_TXTOKEN_WRAP *) Item->Value;
  ASSERT (Wrap != NULL);

  if ((Wrap->Life > 0) && (--Wrap->Life == 0)) {
    Ip4CancelPacket (Wrap->IpInstance->Interface, Wrap->Packet, EFI_ABORTED);
  }

  return EFI_SUCCESS;
}


/**
  The heart beat timer of IP4 service instance. It times out
  all of its IP4 children's received-but-not-delivered and
  transmitted-but-not-recycle packets, and provides time input
  for its IGMP protocol.

  @param  Event                  The IP4 service instance's heart beat timer.
  @param  Context                The IP4 service instance.

  @return None

**/
VOID
EFIAPI
Ip4TimerTicking (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  IP4_SERVICE               *IpSb;

  IpSb = (IP4_SERVICE *) Context;
  NET_CHECK_SIGNATURE (IpSb, IP4_SERVICE_SIGNATURE);

  Ip4PacketTimerTicking (IpSb);
  Ip4IgmpTicking (IpSb);
}
