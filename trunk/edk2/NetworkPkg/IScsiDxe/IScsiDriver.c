/** @file
  The entry point of IScsi driver.

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IScsiImpl.h"

EFI_DRIVER_BINDING_PROTOCOL gIScsiDriverBinding = {
  IScsiDriverBindingSupported,
  IScsiDriverBindingStart,
  IScsiDriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_GUID                    mIScsiV4PrivateGuid = ISCSI_V4_PRIVATE_GUID;
EFI_GUID                    mIScsiV6PrivateGuid = ISCSI_V6_PRIVATE_GUID;
ISCSI_PRIVATE_DATA          *mPrivate           = NULL;

/**
  Tests to see if this driver supports the RemainingDevicePath.

  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This 
                                   parameter is ignored by device drivers, and is optional for bus 
                                   drivers. For bus drivers, if this parameter is not NULL, then 
                                   the bus driver must determine if the bus controller specified 
                                   by ControllerHandle and the child controller specified 
                                   by RemainingDevicePath are both supported by this 
                                   bus driver.

  @retval EFI_SUCCESS              The RemainingDevicePath is supported or NULL.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
IScsiIsDevicePathSupported (
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *CurrentDevicePath;

  CurrentDevicePath = RemainingDevicePath;
  if (CurrentDevicePath != NULL) {
    while (!IsDevicePathEnd (CurrentDevicePath)) {
      if ((CurrentDevicePath->Type == MESSAGING_DEVICE_PATH) && (CurrentDevicePath->SubType == MSG_ISCSI_DP)) {
        return EFI_SUCCESS;
      }

      CurrentDevicePath = NextDevicePathNode (CurrentDevicePath);
    }

    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}


/**
  Tests to see if this driver supports a given controller. If a child device is provided, 
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by 
  ControllerHandle. Drivers typically use the device path attached to 
  ControllerHandle and/or the services from the bus I/O abstraction attached to 
  ControllerHandle to determine if the driver supports ControllerHandle. This function 
  may be called many times during platform initialization. In order to reduce boot times, the tests 
  performed by this function must be very small and take as little time as possible to execute. This 
  function must not change the state of any hardware devices, and this function must be aware that the 
  device specified by ControllerHandle may already be managed by the same driver or a 
  different driver. This function must match its calls to AllocatePages() with FreePages(), 
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().  
  Since ControllerHandle may have been previously started by the same driver, if a protocol is 
  already in the opened state, then it must not be closed with CloseProtocol(). This is required 
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This 
                                   parameter is ignored by device drivers, and is optional for bus 
                                   drivers. For bus drivers, if this parameter is not NULL, then 
                                   the bus driver must determine if the bus controller specified 
                                   by ControllerHandle and the child controller specified 
                                   by RemainingDevicePath are both supported by this 
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
IScsiDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                Status;
  BOOLEAN                   IsIscsi4Started;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &mIScsiV4PrivateGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    IsIscsi4Started = TRUE;
  } else {
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gEfiTcp4ServiceBindingProtocolGuid,
                    NULL,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      Status = IScsiIsDevicePathSupported (RemainingDevicePath);
      if (!EFI_ERROR (Status)) {
        return EFI_SUCCESS;
      }
    }

    IsIscsi4Started = FALSE;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &mIScsiV6PrivateGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    if (IsIscsi4Started) {
      return EFI_ALREADY_STARTED;
    }
  } else {
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gEfiTcp6ServiceBindingProtocolGuid,
                    NULL,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      Status = IScsiIsDevicePathSupported (RemainingDevicePath);
      if (!EFI_ERROR (Status)) {
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_UNSUPPORTED;
}


/**
  Start to manage the controller. This is the worker function for
  IScsiDriverBindingStart.

  @param[in]  Image                Handle of the image.
  @param[in]  ControllerHandle     Handle of the controller.
  @param[in]  IpVersion            Ip4 or Ip6

  @retval EFI_SUCCES            This driver supports this device.
  @retval EFI_ALREADY_STARTED   This driver is already running on this device.
  @retval EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval EFI_NOT_FOUND         There is no sufficient information to establish
                                the iScsi session.
  @retval EFI_DEVICE_ERROR      Failed to get TCP connection device path.                              

**/
EFI_STATUS
IScsiStart (
  IN EFI_HANDLE                   Image,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINT8                        IpVersion
  )
{
  EFI_STATUS                      Status;
  ISCSI_DRIVER_DATA               *Private;
  LIST_ENTRY                      *Entry;
  LIST_ENTRY                      *NextEntry;
  ISCSI_ATTEMPT_CONFIG_NVDATA     *AttemptConfigData;
  ISCSI_SESSION                   *Session;
  UINT8                           Index;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *ExistIScsiExtScsiPassThru;
  ISCSI_DRIVER_DATA               *ExistPrivate;
  UINT8                           *AttemptConfigOrder;
  UINTN                           AttemptConfigOrderSize;
  UINT8                           BootSelected;
  EFI_HANDLE                      *HandleBuffer;
  UINTN                           NumberOfHandles;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  EFI_GUID                        *IScsiPrivateGuid;
  EFI_GUID                        *TcpServiceBindingGuid;
  CHAR16                          MacString[ISCSI_MAX_MAC_STRING_LEN];
  BOOLEAN                         NeedUpdate;
  VOID                            *Interface;
  EFI_GUID                        *ProtocolGuid;

  //
  // Test to see if iSCSI driver supports the given controller.
  //

  if (IpVersion == IP_VERSION_4) {
    IScsiPrivateGuid      = &mIScsiV4PrivateGuid;
    TcpServiceBindingGuid = &gEfiTcp4ServiceBindingProtocolGuid;
    ProtocolGuid          = &gEfiTcp4ProtocolGuid;
  } else if (IpVersion == IP_VERSION_6) {
    IScsiPrivateGuid      = &mIScsiV6PrivateGuid;
    TcpServiceBindingGuid = &gEfiTcp6ServiceBindingProtocolGuid;
    ProtocolGuid          = &gEfiTcp6ProtocolGuid;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  IScsiPrivateGuid,
                  NULL,
                  Image,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  TcpServiceBindingGuid,
                  NULL,
                  Image,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Record the incoming NIC info.
  //
  Status = IScsiAddNic (ControllerHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create the instance private data.
  //
  Private = IScsiCreateDriverData (Image, ControllerHandle);
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create a underlayer child instance, but not need to configure it. Just open ChildHandle
  // via BY_DRIVER. That is, establishing the relationship between ControllerHandle and ChildHandle.
  // Therefore, when DisconnectController(), especially VLAN virtual controller handle,
  // IScsiDriverBindingStop() will be called.
  //
  Status = NetLibCreateServiceChild (
             ControllerHandle,
             Image,
             TcpServiceBindingGuid,
             &Private->ChildHandle
             );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Private->ChildHandle,
                  ProtocolGuid,
                  &Interface,
                  Image,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
                  
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Always install private protocol no matter what happens later. We need to 
  // keep the relationship between ControllerHandle and ChildHandle.
  //
  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  IScsiPrivateGuid,
                  EFI_NATIVE_INTERFACE,
                  &Private->IScsiIdentifier
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }
  
  if (IpVersion == IP_VERSION_4) {
    mPrivate->Ipv6Flag = FALSE;
  } else {
    mPrivate->Ipv6Flag = TRUE;
  }

  //
  // Get the current iSCSI configuration data.
  //
  Status = IScsiGetConfigData (Private);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // If there is already a successul attempt, check whether this attempt is the
  // first "enabled for MPIO" attempt. If not, still try the first attempt.
  // In single path mode, try all attempts.
  //
  ExistPrivate = NULL;
  Status       = EFI_NOT_FOUND;

  if (mPrivate->OneSessionEstablished && mPrivate->EnableMpio) {
    AttemptConfigData = NULL;
    NET_LIST_FOR_EACH (Entry, &mPrivate->AttemptConfigs) {
     AttemptConfigData = NET_LIST_USER_STRUCT (Entry, ISCSI_ATTEMPT_CONFIG_NVDATA, Link);
      if (AttemptConfigData->SessionConfigData.Enabled == ISCSI_ENABLED_FOR_MPIO) {
        break;
      }
    }

    if (AttemptConfigData == NULL) {
      goto ON_ERROR;
    }

    if (AttemptConfigData->AttemptConfigIndex == mPrivate->BootSelectedIndex) {
      goto ON_EXIT;
    }

    //
    // Uninstall the original ExtScsiPassThru first.
    //

    //
    // Locate all ExtScsiPassThru protocol instances.
    //
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiExtScsiPassThruProtocolGuid,
                    NULL,
                    &NumberOfHandles,
                    &HandleBuffer
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Find ExtScsiPassThru protocol instance produced by this driver.
    //
    ExistIScsiExtScsiPassThru = NULL;
    for (Index = 0; Index < NumberOfHandles && ExistIScsiExtScsiPassThru == NULL; Index++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiDevicePathProtocolGuid,
                      (VOID **) &DevicePath
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      while (!IsDevicePathEnd (DevicePath)) {
        if ((DevicePath->Type == MESSAGING_DEVICE_PATH) && (DevicePath->SubType == MSG_MAC_ADDR_DP)) {
          //
          // Get the ExtScsiPassThru protocol instance.
          //
          Status = gBS->HandleProtocol (
                          HandleBuffer[Index],
                          &gEfiExtScsiPassThruProtocolGuid,
                          (VOID **) &ExistIScsiExtScsiPassThru
                          );
          ASSERT_EFI_ERROR (Status);
          break;
        }

        DevicePath = NextDevicePathNode (DevicePath);
      }
    }

    FreePool (HandleBuffer);

    if (ExistIScsiExtScsiPassThru == NULL) {
      Status = EFI_NOT_FOUND;
      goto ON_ERROR;
    }

    ExistPrivate = ISCSI_DRIVER_DATA_FROM_EXT_SCSI_PASS_THRU (ExistIScsiExtScsiPassThru);

    Status = gBS->UninstallProtocolInterface (
                    ExistPrivate->ExtScsiPassThruHandle,
                    &gEfiExtScsiPassThruProtocolGuid,
                    &ExistPrivate->IScsiExtScsiPassThru
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  //
  // Install the Ext SCSI PASS THRU protocol.
  //
  Status = gBS->InstallProtocolInterface (
                  &Private->ExtScsiPassThruHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Private->IScsiExtScsiPassThru
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  BootSelected = 0;

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &mPrivate->AttemptConfigs) {
    AttemptConfigData = NET_LIST_USER_STRUCT (Entry, ISCSI_ATTEMPT_CONFIG_NVDATA, Link);
    //
    // Don't process the attempt that does not associate with the current NIC or
    // this attempt is disabled or established.
    //
    if (AttemptConfigData->NicIndex != mPrivate->CurrentNic ||
        AttemptConfigData->SessionConfigData.Enabled == ISCSI_DISABLED ||
        AttemptConfigData->ValidPath) {
      continue;
    }

    //
    // In multipath mode, don't process attempts configured for single path.
    // In default single path mode, don't process attempts configured for multipath.
    //
    if ((mPrivate->EnableMpio &&
         AttemptConfigData->SessionConfigData.Enabled != ISCSI_ENABLED_FOR_MPIO) ||
        (!mPrivate->EnableMpio &&
         AttemptConfigData->SessionConfigData.Enabled != ISCSI_ENABLED)) {
      continue;
    }

    //
    // Don't process the attempt that fails to get the init/target information from DHCP.
    //
    if (AttemptConfigData->SessionConfigData.InitiatorInfoFromDhcp &&
        !AttemptConfigData->DhcpSuccess) {
      if (!mPrivate->EnableMpio && mPrivate->ValidSinglePathCount > 0) {
        mPrivate->ValidSinglePathCount--;
      }
      continue;
    }

    //
    // Don't process the autoconfigure path if it is already established.
    //
    if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_AUTOCONFIG &&
        AttemptConfigData->AutoConfigureMode == IP_MODE_AUTOCONFIG_SUCCESS) {
      continue;
    }

    //
    // Don't process the attempt if its IP mode is not in the current IP version.
    //
    if (!mPrivate->Ipv6Flag) {
      if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_IP6) {
        continue;
      }
      if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_AUTOCONFIG &&
          AttemptConfigData->AutoConfigureMode == IP_MODE_AUTOCONFIG_IP6) {
        continue;
      }
    } else {
      if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_IP4) {
        continue;
      }
      if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_AUTOCONFIG &&
          AttemptConfigData->AutoConfigureMode == IP_MODE_AUTOCONFIG_IP4) {
        continue;
      }
    }

    //
    // Fill in the Session and init it.
    //
    Session = (ISCSI_SESSION *) AllocateZeroPool (sizeof (ISCSI_SESSION));
    if (Session == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }

    Session->Private    = Private;
    Session->ConfigData = AttemptConfigData;
    Session->AuthType   = AttemptConfigData->AuthenticationType;

    AsciiStrToUnicodeStr (AttemptConfigData->MacString, MacString);
    UnicodeSPrint (
      mPrivate->PortString,
      (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
      L"%s%d",
      MacString,
      (UINTN) AttemptConfigData->AttemptConfigIndex
      );

    if (Session->AuthType == ISCSI_AUTH_TYPE_CHAP) {
      Session->AuthData.CHAP.AuthConfig = &AttemptConfigData->AuthConfigData.CHAP;
    }

    IScsiSessionInit (Session, FALSE);

    //
    // Try to login and create an iSCSI session according to the configuration.
    //
    Status = IScsiSessionLogin (Session);
    if (Status == EFI_MEDIA_CHANGED) {
      //
      // The specified target is not available, and the redirection information is
      // received. Login the session again with the updated target address.
      //
      Status = IScsiSessionLogin (Session);
    } else if (Status == EFI_NOT_READY) {
      Status = IScsiSessionReLogin (Session);
    }

    if (EFI_ERROR (Status)) {
      //
      // In Single path mode, only the successful attempt will be recorded in iBFT;
      // in multi-path mode, all the attempt entries in MPIO will be recorded in iBFT.
      //
      if (!mPrivate->EnableMpio && mPrivate->ValidSinglePathCount > 0) {
        mPrivate->ValidSinglePathCount--;
      }

      FreePool (Session);

    } else {
      AttemptConfigData->ValidPath = TRUE;

      //
      // Do not record the attempt in iBFT if it login with KRB5.
      // TODO: record KRB5 attempt information in the iSCSI device path.
      //
      if (Session->AuthType == ISCSI_AUTH_TYPE_KRB) {
        if (!mPrivate->EnableMpio && mPrivate->ValidSinglePathCount > 0) {
          mPrivate->ValidSinglePathCount--;
        }

        AttemptConfigData->ValidiBFTPath = FALSE;
      } else {
        AttemptConfigData->ValidiBFTPath = TRUE;
      }

      //
      // IScsi session success. Update the attempt state to NVR.
      //
      if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_AUTOCONFIG) {
        AttemptConfigData->AutoConfigureMode = IP_MODE_AUTOCONFIG_SUCCESS;
      }

      gRT->SetVariable (
             mPrivate->PortString,
             &gEfiIScsiInitiatorNameProtocolGuid,
             ISCSI_CONFIG_VAR_ATTR,
             sizeof (ISCSI_ATTEMPT_CONFIG_NVDATA),
             AttemptConfigData
             );

      //
      // Select the first login session. Abort others.
      //
      if (Private->Session == NULL) {
        Private->Session = Session;
        BootSelected     = AttemptConfigData->AttemptConfigIndex;
        //
        // Don't validate other attempt in multipath mode if one is success.
        //
        if (mPrivate->EnableMpio) {
          break;
        }
      } else {
        IScsiSessionAbort (Session);
        FreePool (Session);
      }
    }
  }

  //
  // All attempts configured for this driver instance are not valid.
  //
  if (Private->Session == NULL) {
    Status = gBS->UninstallProtocolInterface (
                    Private->ExtScsiPassThruHandle,
                    &gEfiExtScsiPassThruProtocolGuid,
                    &Private->IScsiExtScsiPassThru
                    );
    ASSERT_EFI_ERROR (Status);
    Private->ExtScsiPassThruHandle = NULL;

    //
    // Reinstall the original ExtScsiPassThru back.
    //
    if (mPrivate->OneSessionEstablished && ExistPrivate != NULL) {
      Status = gBS->InstallProtocolInterface (
                      &ExistPrivate->ExtScsiPassThruHandle,
                      &gEfiExtScsiPassThruProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      &ExistPrivate->IScsiExtScsiPassThru
                      );
      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }

      goto ON_EXIT;
    }

    Status = EFI_NOT_FOUND;

    goto ON_ERROR;
  }

  NeedUpdate = TRUE;
  //
  // More than one attempt successes.
  //
  if (Private->Session != NULL && mPrivate->OneSessionEstablished) {

    AttemptConfigOrder = IScsiGetVariableAndSize (
                           L"AttemptOrder",
                           &gIScsiConfigGuid,
                           &AttemptConfigOrderSize
                           );
    ASSERT (AttemptConfigOrder != NULL);
    for (Index = 0; Index < AttemptConfigOrderSize / sizeof (UINT8); Index++) {
      if (AttemptConfigOrder[Index] == mPrivate->BootSelectedIndex ||
          AttemptConfigOrder[Index] == BootSelected) {
        break;
      }
    }

    if (mPrivate->EnableMpio) {
      //
      // Use the attempt in earlier order. Abort the later one in MPIO.
      //
      if (AttemptConfigOrder[Index] == mPrivate->BootSelectedIndex) {
        IScsiSessionAbort (Private->Session);
        FreePool (Private->Session);
        Private->Session = NULL;
        gBS->UninstallProtocolInterface (
               Private->ExtScsiPassThruHandle,
               &gEfiExtScsiPassThruProtocolGuid,
               &Private->IScsiExtScsiPassThru
               );
        Private->ExtScsiPassThruHandle = NULL;

        //
        // Reinstall the original ExtScsiPassThru back.
        //
        Status = gBS->InstallProtocolInterface (
                        &ExistPrivate->ExtScsiPassThruHandle,
                        &gEfiExtScsiPassThruProtocolGuid,
                        EFI_NATIVE_INTERFACE,
                        &ExistPrivate->IScsiExtScsiPassThru
                        );
        if (EFI_ERROR (Status)) {
          goto ON_ERROR;
        }

        goto ON_EXIT;
      } else {
        ASSERT (AttemptConfigOrder[Index] == BootSelected);
        mPrivate->BootSelectedIndex = BootSelected;
        //
        // Clear the resource in ExistPrivate.
        //
        gBS->UninstallProtocolInterface (
               ExistPrivate->Controller,
               IScsiPrivateGuid,
               &ExistPrivate->IScsiIdentifier
               ); 
        
        IScsiRemoveNic (ExistPrivate->Controller);
        if (ExistPrivate->Session != NULL) {
          IScsiSessionAbort (ExistPrivate->Session);
        }

        IScsiCleanDriverData (ExistPrivate);
      }
    } else {
      //
      // Use the attempt in earlier order as boot selected in single path mode.
      //
      if (AttemptConfigOrder[Index] == mPrivate->BootSelectedIndex) {
        NeedUpdate = FALSE;
      }
    }

  }

  if (NeedUpdate) {
    mPrivate->OneSessionEstablished = TRUE;
    mPrivate->BootSelectedIndex     = BootSelected;
  }

  //
  // Duplicate the Session's tcp connection device path. The source port field
  // will be set to zero as one iSCSI session is comprised of several iSCSI
  // connections.
  //
  Private->DevicePath = IScsiGetTcpConnDevicePath (Private->Session);
  if (Private->DevicePath == NULL) {
    Status = EFI_DEVICE_ERROR;
    goto ON_ERROR;
  }
  //
  // Install the updated device path onto the ExtScsiPassThruHandle.
  //
  Status = gBS->InstallProtocolInterface (
                  &Private->ExtScsiPassThruHandle,
                  &gEfiDevicePathProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  Private->DevicePath
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

ON_EXIT:

  //
  // Update/Publish the iSCSI Boot Firmware Table.
  //
  if (mPrivate->BootSelectedIndex != 0) {
    IScsiPublishIbft ();
  }

  return EFI_SUCCESS;

ON_ERROR:

  if (Private->Session != NULL) {
    IScsiSessionAbort (Private->Session);
  }

  return Status;
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this 
  common boot service. It is legal to call Start() from other locations, 
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.  

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This 
                                   parameter is ignored by device drivers, and is optional for bus 
                                   drivers. For a bus driver, if this parameter is NULL, then handles 
                                   for all the children of Controller are created by this driver.  
                                   If this parameter is not NULL and the first Device Path Node is 
                                   not the End of Device Path Node, then only the handle for the 
                                   child device specified by the first Device Path Node of 
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is 
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
IScsiDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS        V4Status;
  EFI_STATUS        V6Status;

  V4Status = IScsiStart (This->DriverBindingHandle, ControllerHandle, IP_VERSION_4);
  if (V4Status == EFI_ALREADY_STARTED) {
    V4Status = EFI_SUCCESS;
  }

  V6Status = IScsiStart (This->DriverBindingHandle, ControllerHandle, IP_VERSION_6);
  if (V6Status == EFI_ALREADY_STARTED) {
    V6Status = EFI_SUCCESS;
  }

  if (!EFI_ERROR (V4Status) || !EFI_ERROR (V6Status)) {
    return EFI_SUCCESS;
  } else if (EFI_ERROR (V4Status)) {
    return V4Status;
  } else {
    return V6Status;
  }
}

/**
  Stops a device controller or a bus controller.
  
  The Stop() function is designed to be invoked from the EFI boot service DisconnectController(). 
  As a result, much of the error checking on the parameters to Stop() has been moved 
  into this common boot service. It is legal to call Stop() from other locations, 
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.
  
  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must 
                                support a bus specific I/O protocol for the driver 
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL 
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
IScsiDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_HANDLE                      IScsiController;
  EFI_STATUS                      Status;
  ISCSI_PRIVATE_PROTOCOL          *IScsiIdentifier;
  ISCSI_DRIVER_DATA               *Private;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *PassThru;
  ISCSI_CONNECTION                *Conn;
  EFI_GUID                        *ProtocolGuid;
  EFI_GUID                        *TcpServiceBindingGuid;
  EFI_GUID                        *TcpProtocolGuid;


  if (NumberOfChildren != 0) {
    //
    // We should have only one child.
    //
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[0],
                    &gEfiExtScsiPassThruProtocolGuid,
                    (VOID **) &PassThru,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Private = ISCSI_DRIVER_DATA_FROM_EXT_SCSI_PASS_THRU (PassThru);
    Conn    = NET_LIST_HEAD (&Private->Session->Conns, ISCSI_CONNECTION, Link);

    //
    // Previously the TCP protocol is opened BY_CHILD_CONTROLLER. Just close
    // the protocol here, but do not uninstall the device path protocol and
    // EXT SCSI PASS THRU protocol installed on ExtScsiPassThruHandle.
    //
    if (!Conn->Ipv6Flag) {
      ProtocolGuid = &gEfiTcp4ProtocolGuid;
    } else {
      ProtocolGuid = &gEfiTcp6ProtocolGuid;
    }

    gBS->CloseProtocol (
           Conn->TcpIo.Handle,
           ProtocolGuid,
           Private->Image,
           Private->ExtScsiPassThruHandle
           );

    return EFI_SUCCESS;
  }
  //
  // Get the handle of the controller we are controling.
  //
  IScsiController = NetLibGetNicHandle (ControllerHandle, &gEfiTcp4ProtocolGuid);
  if (IScsiController != NULL) {
    ProtocolGuid            = &mIScsiV4PrivateGuid;
    TcpProtocolGuid         = &gEfiTcp4ProtocolGuid;
    TcpServiceBindingGuid   = &gEfiTcp4ServiceBindingProtocolGuid;
  } else {
    IScsiController = NetLibGetNicHandle (ControllerHandle, &gEfiTcp6ProtocolGuid);
    ASSERT (IScsiController != NULL);
    ProtocolGuid            = &mIScsiV6PrivateGuid;
    TcpProtocolGuid         = &gEfiTcp6ProtocolGuid;
    TcpServiceBindingGuid   = &gEfiTcp6ServiceBindingProtocolGuid;
  }

  Status = gBS->OpenProtocol (
                  IScsiController,
                  ProtocolGuid,
                  (VOID **) &IScsiIdentifier,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Private = ISCSI_DRIVER_DATA_FROM_IDENTIFIER (IScsiIdentifier);
  ASSERT (Private != NULL);

  if (Private->ChildHandle != NULL) {
    Status = gBS->CloseProtocol (
                    Private->ChildHandle,
                    TcpProtocolGuid,
                    This->DriverBindingHandle,
                    IScsiController
                    );
                    
    ASSERT (!EFI_ERROR (Status));

    Status = NetLibDestroyServiceChild (
               IScsiController,
               This->DriverBindingHandle,
               TcpServiceBindingGuid,
               Private->ChildHandle
               );

    ASSERT (!EFI_ERROR (Status));
  }

  gBS->UninstallProtocolInterface (
         IScsiController,
         ProtocolGuid,
         &Private->IScsiIdentifier
         ); 

  //
  // Remove this NIC.
  //
  IScsiRemoveNic (IScsiController);

  //
  // Update the iSCSI Boot Firware Table.
  //
  IScsiPublishIbft ();

  if (Private->Session != NULL) {
    IScsiSessionAbort (Private->Session);
  }

  IScsiCleanDriverData (Private);

  return EFI_SUCCESS;
}


/**
  Unload the iSCSI driver.

  @param[in]  ImageHandle          The handle of the driver image.

  @retval     EFI_SUCCESS          The driver is unloaded.
  @retval     EFI_DEVICE_ERROR     An unexpected error occurred.

**/
EFI_STATUS
EFIAPI
IScsiUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;
  UINTN       DeviceHandleCount;
  EFI_HANDLE  *DeviceHandleBuffer;
  UINTN       Index;

  //
  // Try to disonnect the driver from the devices it's controlling.
  //
  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &DeviceHandleCount,
                  &DeviceHandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < DeviceHandleCount; Index++) {
      Status = gBS->DisconnectController (
                      DeviceHandleBuffer[Index],
                      ImageHandle,
                      NULL
                      );
    }

    if (DeviceHandleBuffer != NULL) {
      FreePool (DeviceHandleBuffer);
    }
  }
  //
  // Unload the iSCSI configuration form.
  //
  IScsiConfigFormUnload (gIScsiDriverBinding.DriverBindingHandle);

  //
  // Uninstall the protocols installed by iSCSI driver.
  //
  gBS->UninstallMultipleProtocolInterfaces (
         ImageHandle,
         &gEfiAuthenticationInfoProtocolGuid,
         &gIScsiAuthenticationInfo,
         NULL
         );
  
  return gBS->UninstallMultipleProtocolInterfaces (
                ImageHandle,
                &gEfiDriverBindingProtocolGuid,
                &gIScsiDriverBinding,
                &gEfiComponentName2ProtocolGuid,
                &gIScsiComponentName2,
                &gEfiComponentNameProtocolGuid,
                &gIScsiComponentName,
                &gEfiIScsiInitiatorNameProtocolGuid,
                &gIScsiInitiatorName,
                NULL
                );
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.
  
  The entry point for iSCSI driver which initializes the global variables and
  installs the driver binding, component name protocol, iSCSI initiator name
  protocol and Authentication Info protocol on its image.
  
  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
IScsiDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                         Status;
  EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *IScsiInitiatorName;
  EFI_AUTHENTICATION_INFO_PROTOCOL   *AuthenticationInfo;

  //
  // There should be only one EFI_ISCSI_INITIATOR_NAME_PROTOCOL.
  //
  Status = gBS->LocateProtocol (
                  &gEfiIScsiInitiatorNameProtocolGuid,
                  NULL,
                  (VOID **) &IScsiInitiatorName
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Initialize the EFI Driver Library.
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gIScsiDriverBinding,
             ImageHandle,
             &gIScsiComponentName,
             &gIScsiComponentName2
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install the iSCSI Initiator Name Protocol.
  //
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiIScsiInitiatorNameProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gIScsiInitiatorName
                  );
  if (EFI_ERROR (Status)) {
    goto Error1;
  } 

  //
  // Create the private data structures.
  //
  mPrivate = AllocateZeroPool (sizeof (ISCSI_PRIVATE_DATA));
  if (mPrivate == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error2;
  }

  InitializeListHead (&mPrivate->NicInfoList);
  InitializeListHead (&mPrivate->AttemptConfigs);

  //
  // Initialize the configuration form of iSCSI.
  //
  Status = IScsiConfigFormInit (gIScsiDriverBinding.DriverBindingHandle);
  if (EFI_ERROR (Status)) {
    goto Error3;
  }

  //
  // There should be only one EFI_AUTHENTICATION_INFO_PROTOCOL. If already exists,
  // do not produce the protocol instance.
  //
  Status = gBS->LocateProtocol (
                  &gEfiAuthenticationInfoProtocolGuid,
                  NULL,
                  (VOID **) &AuthenticationInfo
                  );
  if (Status == EFI_NOT_FOUND) {
    Status = gBS->InstallProtocolInterface (
                    &ImageHandle,
                    &gEfiAuthenticationInfoProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &gIScsiAuthenticationInfo
                    );
    if (EFI_ERROR (Status)) {
      goto Error4;
    }    
  }

  return EFI_SUCCESS;

Error4:
  IScsiConfigFormUnload (gIScsiDriverBinding.DriverBindingHandle);

Error3:
  FreePool (mPrivate);

Error2:
  gBS->UninstallMultipleProtocolInterfaces (
         ImageHandle,
         &gEfiIScsiInitiatorNameProtocolGuid,
         &gIScsiInitiatorName,
         NULL
         );

Error1:
  gBS->UninstallMultipleProtocolInterfaces (
         ImageHandle,
         &gEfiDriverBindingProtocolGuid,
         &gIScsiDriverBinding,
         &gEfiComponentName2ProtocolGuid,
         &gIScsiComponentName2,
         &gEfiComponentNameProtocolGuid,
         &gIScsiComponentName,
         NULL
         );

  return Status;
}

