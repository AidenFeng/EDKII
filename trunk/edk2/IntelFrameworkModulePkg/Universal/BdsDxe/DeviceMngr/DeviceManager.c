/** @file
  The platform device manager reference implementation

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DeviceManager.h"

DEVICE_MANAGER_CALLBACK_DATA  gDeviceManagerPrivate = {
  DEVICE_MANAGER_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    FakeExtractConfig,
    FakeRouteConfig,
    DeviceManagerCallback
  }
};

EFI_GUID mDeviceManagerGuid = DEVICE_MANAGER_FORMSET_GUID;

DEVICE_MANAGER_MENU_ITEM  mDeviceManagerMenuItemTable[] = {
  { STRING_TOKEN (STR_DISK_DEVICE),     EFI_DISK_DEVICE_CLASS },
  { STRING_TOKEN (STR_VIDEO_DEVICE),    EFI_VIDEO_DEVICE_CLASS },
  { STRING_TOKEN (STR_NETWORK_DEVICE),  EFI_NETWORK_DEVICE_CLASS },
  { STRING_TOKEN (STR_INPUT_DEVICE),    EFI_INPUT_DEVICE_CLASS },
  { STRING_TOKEN (STR_ON_BOARD_DEVICE), EFI_ON_BOARD_DEVICE_CLASS },
  { STRING_TOKEN (STR_OTHER_DEVICE),    EFI_OTHER_DEVICE_CLASS }
};

HII_VENDOR_DEVICE_PATH  mDeviceManagerHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {102579A0-3686-466e-ACD8-80C087044F4A}
    //
    { 0x102579a0, 0x3686, 0x466e, { 0xac, 0xd8, 0x80, 0xc0, 0x87, 0x4, 0x4f, 0x4a } }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { 
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

#define MENU_ITEM_NUM  \
  (sizeof (mDeviceManagerMenuItemTable) / sizeof (DEVICE_MANAGER_MENU_ITEM))

/**
  This function is invoked if user selected a iteractive opcode from Device Manager's
  Formset. The decision by user is saved to gCallbackKey for later processing. If
  user set VBIOS, the new value is saved to EFI variable.

  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

**/
EFI_STATUS
EFIAPI
DeviceManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }


  gCallbackKey = QuestionId;

  //
  // Request to exit SendForm(), so as to switch to selected form
  //
  *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;


  return EFI_SUCCESS;
}

/**

  This function registers HII packages to HII database.

  @retval  EFI_SUCCESS           HII packages for the Device Manager were registered successfully.
  @retval  EFI_OUT_OF_RESOURCES  HII packages for the Device Manager failed to be registered.

**/
EFI_STATUS
InitializeDeviceManager (
  VOID
  )
{
  EFI_STATUS                  Status;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gDeviceManagerPrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mDeviceManagerHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gDeviceManagerPrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  gDeviceManagerPrivate.HiiHandle = HiiAddPackages (
                                      &mDeviceManagerGuid,
                                      gDeviceManagerPrivate.DriverHandle,
                                      DeviceManagerVfrBin,
                                      BdsDxeStrings,
                                      NULL
                                      );
  if (gDeviceManagerPrivate.HiiHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    Status = EFI_SUCCESS;
  }
  return Status;
}

/**
  Call the browser and display the device manager to allow user
  to configure the platform.

  This function create the dynamic content for device manager. It includes
  section header for all class of devices, one-of opcode to set VBIOS.
  
  @retval  EFI_SUCCESS             Operation is successful.
  @return  Other values if failed to clean up the dynamic content from HII
           database.

**/
EFI_STATUS
CallDeviceManager (
  VOID
  )
{
  EFI_STATUS                  Status;
  UINTN                       Count;
  UINTN                       Index;
  EFI_STRING                  String;
  EFI_HII_UPDATE_DATA         UpdateData[MENU_ITEM_NUM];
  EFI_STRING_ID               Token;
  EFI_STRING_ID               TokenHelp;
  EFI_HII_HANDLE              *HiiHandles;
  EFI_HII_HANDLE              HiiHandle;
  UINT16                      FormSetClass;
  EFI_STRING_ID               FormSetTitle;
  EFI_STRING_ID               FormSetHelp;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;

  HiiHandles          = NULL;

  Status        = EFI_SUCCESS;
  gCallbackKey  = 0;

  //
  // Connect all prior to entering the platform setup menu.
  //
  if (!gConnectAllHappened) {
    BdsLibConnectAllDriversToAllControllers ();
    gConnectAllHappened = TRUE;
  }

  //
  // Create Subtitle OpCodes
  //
  for (Index = 0; Index < MENU_ITEM_NUM; Index++) {
    //
    // Allocate space for creation of UpdateData Buffer
    //
    UpdateData[Index].BufferSize = 0x1000;
    UpdateData[Index].Offset = 0;
    UpdateData[Index].Data = AllocatePool (0x1000);
    ASSERT (UpdateData[Index].Data != NULL);

    CreateSubTitleOpCode (mDeviceManagerMenuItemTable[Index].StringId, 0, 0, 1,  &UpdateData[Index]);
  }

  //
  // Get all the Hii handles
  //
  HiiHandles = HiiGetHiiHandles (NULL);
  ASSERT (HiiHandles != NULL);

  HiiHandle = gDeviceManagerPrivate.HiiHandle;

  //
  // Search for formset of each class type
  //
  for (Index = 0; HiiHandles[Index] != NULL; Index++) {
    IfrLibExtractClassFromHiiHandle (HiiHandles[Index], &FormSetClass, &FormSetTitle, &FormSetHelp);

    if (FormSetClass == EFI_NON_DEVICE_CLASS) {
      continue;
    }

    String = HiiGetString (HiiHandles[Index], FormSetTitle, NULL);
    ASSERT (String != NULL);
    Token = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    String = HiiGetString (HiiHandles[Index], FormSetHelp, NULL);
    ASSERT (String != NULL);
    TokenHelp = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    for (Count = 0; Count < MENU_ITEM_NUM; Count++) {
      if (FormSetClass & mDeviceManagerMenuItemTable[Count].Class) {
        CreateActionOpCode (
          (EFI_QUESTION_ID) (Index + DEVICE_KEY_OFFSET),
          Token,
          TokenHelp,
          EFI_IFR_FLAG_CALLBACK,
          0,
          &UpdateData[Count]
          );
      }
    }
  }

  for (Index = 0; Index < MENU_ITEM_NUM; Index++) {
    //
    // Add End Opcode for Subtitle
    //
    CreateEndOpCode (&UpdateData[Index]);

    IfrLibUpdateForm (
      HiiHandle,
      &mDeviceManagerGuid,
      DEVICE_MANAGER_FORM_ID,
      mDeviceManagerMenuItemTable[Index].Class,
      FALSE,
      &UpdateData[Index]
      );
  }

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = gFormBrowser2->SendForm (
                           gFormBrowser2,
                           &HiiHandle,
                           1,
                           NULL,
                           0,
                           NULL,
                           &ActionRequest
                           );
  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    EnableResetRequired ();
  }

  //
  // We will have returned from processing a callback - user either hit ESC to exit, or selected
  // a target to display
  //
  if (gCallbackKey != 0) {
    ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
    Status = gFormBrowser2->SendForm (
                             gFormBrowser2,
                             &HiiHandles[gCallbackKey - DEVICE_KEY_OFFSET],
                             1,
                             NULL,
                             0,
                             NULL,
                             &ActionRequest
                             );

    if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
      EnableResetRequired ();
    }

    //
    // Force return to Device Manager
    //
    gCallbackKey = FRONT_PAGE_KEY_DEVICE_MANAGER;
  }

  //
  // Cleanup dynamic created strings in HII database by reinstall the packagelist
  //
  HiiRemovePackages (HiiHandle);

  gDeviceManagerPrivate.HiiHandle = HiiAddPackages (
                                      &mDeviceManagerGuid,
                                      gDeviceManagerPrivate.DriverHandle,
                                      DeviceManagerVfrBin,
                                      BdsDxeStrings,
                                      NULL
                                      );
  if (gDeviceManagerPrivate.HiiHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    Status = EFI_SUCCESS;
  }

  for (Index = 0; Index < MENU_ITEM_NUM; Index++) {
    FreePool (UpdateData[Index].Data);
  }
  FreePool (HiiHandles);

  return Status;
}
