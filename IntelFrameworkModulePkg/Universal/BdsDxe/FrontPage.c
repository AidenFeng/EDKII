/** @file
  FrontPage routines to handle the callbacks and browser calls

Copyright (c) 2004 - 2009, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Bds.h"
#include "FrontPage.h"

EFI_GUID  mFrontPageGuid      = FRONT_PAGE_FORMSET_GUID;

BOOLEAN   gConnectAllHappened = FALSE;
UINTN     gCallbackKey;

EFI_HII_DATABASE_PROTOCOL       *gHiiDatabase;
EFI_HII_STRING_PROTOCOL         *gHiiString;
EFI_FORM_BROWSER2_PROTOCOL      *gFormBrowser2;
EFI_HII_CONFIG_ROUTING_PROTOCOL *gHiiConfigRouting;

FRONT_PAGE_CALLBACK_DATA  gFrontPagePrivate = {
  FRONT_PAGE_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  NULL,
  {
    FakeExtractConfig,
    FakeRouteConfig,
    FrontPageCallback
  }
};

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
FakeExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  return EFI_NOT_FOUND;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
FakeRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  return EFI_SUCCESS;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
  @retval  EFI_DEVICE_ERROR      The variable could not be saved.
  @retval  EFI_UNSUPPORTED       The specified Action is not supported by the callback.

**/
EFI_STATUS
EFIAPI
FrontPageCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  CHAR8                         *LanguageString;
  CHAR8                         *LangCode;
  CHAR8                         *Lang;
  CHAR8                         OldLang[ISO_639_2_ENTRY_SIZE];
  UINTN                         Index;
  EFI_STATUS                    Status;
  CHAR8                         *PlatformSupportedLanguages;
  CHAR8                         *BestLanguage;

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  gCallbackKey = QuestionId;

  //
  // The first 4 entries in the Front Page are to be GUARANTEED to remain constant so IHV's can
  // describe to their customers in documentation how to find their setup information (namely
  // under the device manager and specific buckets)
  //
  switch (QuestionId) {
  case FRONT_PAGE_KEY_CONTINUE:
    //
    // This is the continue - clear the screen and return an error to get out of FrontPage loop
    //
    break;

  case FRONT_PAGE_KEY_LANGUAGE:
    //
    // Collect the languages from what our current Language support is based on our VFR
    //
    LanguageString = HiiLibGetSupportedLanguages (gFrontPagePrivate.HiiHandle);
    ASSERT (LanguageString != NULL);
    //
    // Allocate working buffer for RFC 4646 language in supported LanguageString.
    //
    Lang = AllocatePool (AsciiStrSize (LanguageString));
    ASSERT (Lang != NULL);
    
    Index = 0;
    LangCode = LanguageString;
    while (*LangCode != 0) {
      HiiLibGetNextLanguage (&LangCode, Lang);

      if (Index == Value->u8) {
        break;
      }

      Index++;
    }

    PlatformSupportedLanguages = GetEfiGlobalVariable (L"PlatformLangCodes");
    if (PlatformSupportedLanguages == NULL) {
      PlatformSupportedLanguages = AllocateCopyPool (
                                     AsciiStrSize ((CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes)),
                                     (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes)
                                     );
      ASSERT (PlatformSupportedLanguages != NULL);
    }
    
    //
    // Select the best language in platform supported Language.
    //
    BestLanguage = GetBestLanguage (
                     PlatformSupportedLanguages,
                     FALSE,
                     Lang,
                     NULL
                     );
    if (BestLanguage != NULL) {
      Status = gRT->SetVariable (
                      L"PlatformLang",
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      AsciiStrSize (BestLanguage),
                      Lang
                      );

      if (!FeaturePcdGet (PcdUefiVariableDefaultLangDeprecate)) {
        //
        // Set UEFI deprecated variable "Lang" for backwards compatibility
        //
        Status = ConvertRfc3066LanguageToIso639Language (BestLanguage, OldLang);
        if (!EFI_ERROR (Status)) {
          Status = gRT->SetVariable (
                          L"Lang",
                          &gEfiGlobalVariableGuid,
                          EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                          ISO_639_2_ENTRY_SIZE,
                          OldLang
                          );
        }
      }
      FreePool (BestLanguage);
    }
  
    FreePool (PlatformSupportedLanguages);
    FreePool (Lang);
    FreePool (LanguageString);
    break;

  case FRONT_PAGE_KEY_BOOT_MANAGER:
    //
    // Boot Manager
    //
    break;

  case FRONT_PAGE_KEY_DEVICE_MANAGER:
    //
    // Device Manager
    //
    break;

  case FRONT_PAGE_KEY_BOOT_MAINTAIN:
    //
    // Boot Maintenance Manager
    //
    break;

  default:
    gCallbackKey = 0;
    break;
  }

  *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;

  return EFI_SUCCESS;
}

/**
  Initialize HII information for the FrontPage


  @param InitializeHiiData    TRUE if HII elements need to be initialized.

  @retval  EFI_SUCCESS        The operation is successful.
  @retval  EFI_DEVICE_ERROR   If the dynamic opcode creation failed.

**/
EFI_STATUS
InitializeFrontPage (
  IN BOOLEAN                         InitializeHiiData
  )
{
  EFI_STATUS                  Status;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList;
  EFI_HII_UPDATE_DATA         UpdateData;
  IFR_OPTION                  *OptionList;
  CHAR8                       *LanguageString;
  CHAR8                       *LangCode;
  CHAR8                       *Lang;
  CHAR8                       *CurrentLang;
  CHAR8                       *BestLanguage;
  UINTN                       OptionCount;
  EFI_STRING_ID               Token;
  CHAR16                      *StringBuffer;
  UINTN                       BufferSize;
  UINTN                       Index;
  EFI_HII_HANDLE              HiiHandle;

  if (InitializeHiiData) {
    //
    // Initialize the Device Manager
    //
    InitializeDeviceManager ();

    //
    // Initialize the Device Manager
    //
    InitializeBootManager ();

    gCallbackKey  = 0;

    //
    // Locate Hii relative protocols
    //
    Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **) &gHiiDatabase);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = gBS->LocateProtocol (&gEfiHiiStringProtocolGuid, NULL, (VOID **) &gHiiString);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &gFormBrowser2);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **) &gHiiConfigRouting);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Create driver handle used by HII database
    //
    Status = HiiLibCreateHiiDriverHandle (&gFrontPagePrivate.DriverHandle);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Install Config Access protocol to driver handle
    //
    Status = gBS->InstallProtocolInterface (
                    &gFrontPagePrivate.DriverHandle,
                    &gEfiHiiConfigAccessProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &gFrontPagePrivate.ConfigAccess
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Publish our HII data
    //
    PackageList = HiiLibPreparePackageList (2, &mFrontPageGuid, FrontPageVfrBin, BdsDxeStrings);
    ASSERT (PackageList != NULL);

    Status = gHiiDatabase->NewPackageList (
                             gHiiDatabase,
                             PackageList,
                             gFrontPagePrivate.DriverHandle,
                             &gFrontPagePrivate.HiiHandle
                             );
    FreePool (PackageList);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }


  //
  // Allocate space for creation of UpdateData Buffer
  //
  UpdateData.BufferSize = 0x1000;
  UpdateData.Data = AllocateZeroPool (0x1000);
  ASSERT (UpdateData.Data != NULL);

  OptionList = AllocateZeroPool (0x1000);
  ASSERT (OptionList != NULL);

  //
  // Collect the languages from what our current Language support is based on our VFR
  //
  HiiHandle = gFrontPagePrivate.HiiHandle;
  LanguageString = HiiLibGetSupportedLanguages (HiiHandle);
  ASSERT (LanguageString != NULL);
  //
  // Allocate working buffer for RFC 4646 language in supported LanguageString.
  //
  Lang = AllocatePool (AsciiStrSize (LanguageString));
  ASSERT (Lang != NULL);

  CurrentLang = GetEfiGlobalVariable (L"PlatformLang");
  //
  // Select the best language in LanguageString as the default one.
  //
  BestLanguage = GetBestLanguage (
                   LanguageString,
                   FALSE,
                   (CurrentLang != NULL) ? CurrentLang : "",
                   (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLang),
                   LanguageString,
                   NULL
                   );
  //
  // BestLanguage must be selected as it is the first language in LanguageString by default
  //
  ASSERT (BestLanguage != NULL);

  OptionCount = 0;
  LangCode = LanguageString;
  while (*LangCode != 0) {
    HiiLibGetNextLanguage (&LangCode, Lang);

    if (gFrontPagePrivate.LanguageToken == NULL) {
      //
      // Get Language Name from String Package. The StringId of Printable Language
      // Name is always 1 which is generated by StringGather Tool.
      //
      BufferSize = 0x100;
      StringBuffer = AllocatePool (BufferSize);
      Status = gHiiString->GetString (
                           gHiiString,
                           Lang,
                           HiiHandle,
                           PRINTABLE_LANGUAGE_NAME_STRING_ID,
                           StringBuffer,
                           &BufferSize,
                           NULL
                           );
      if (Status == EFI_BUFFER_TOO_SMALL) {
        FreePool (StringBuffer);
        StringBuffer = AllocatePool (BufferSize);
        Status = gHiiString->GetString (
                             gHiiString,
                             Lang,
                             HiiHandle,
                             PRINTABLE_LANGUAGE_NAME_STRING_ID,
                             StringBuffer,
                             &BufferSize,
                             NULL
                             );
      }
      ASSERT_EFI_ERROR (Status);

      Token = 0;
      Status = HiiLibNewString (HiiHandle, &Token, StringBuffer);
      FreePool (StringBuffer);
    } else {
      Token = gFrontPagePrivate.LanguageToken[OptionCount];
    }

    if (AsciiStrCmp (Lang, BestLanguage) == 0) {
      OptionList[OptionCount].Flags = EFI_IFR_OPTION_DEFAULT;
    } else {
      OptionList[OptionCount].Flags = 0;
    }
    OptionList[OptionCount].StringToken = Token;
    OptionList[OptionCount].Value.u8 = (UINT8) OptionCount;

    OptionCount++;
  }

  if (CurrentLang != NULL) {
    FreePool (CurrentLang);
  }
  FreePool (BestLanguage);
  FreePool (Lang);
  FreePool (LanguageString);

  UpdateData.Offset = 0;
  CreateOneOfOpCode (
    FRONT_PAGE_KEY_LANGUAGE,
    0,
    0,
    STRING_TOKEN (STR_LANGUAGE_SELECT),
    STRING_TOKEN (STR_LANGUAGE_SELECT_HELP),
    EFI_IFR_FLAG_CALLBACK,
    EFI_IFR_NUMERIC_SIZE_1,
    OptionList,
    OptionCount,
    &UpdateData
    );

  Status = IfrLibUpdateForm (
             HiiHandle,
             &mFrontPageGuid,
             FRONT_PAGE_FORM_ID,
             LABEL_SELECT_LANGUAGE,
             FALSE,
             &UpdateData
             );

  //
  // Save the string Id for each language
  //
  gFrontPagePrivate.LanguageToken = AllocatePool (OptionCount * sizeof (EFI_STRING_ID));
  ASSERT (gFrontPagePrivate.LanguageToken != NULL);
  for (Index = 0; Index < OptionCount; Index++) {
    gFrontPagePrivate.LanguageToken[Index] = OptionList[Index].StringToken;
  }

  FreePool (UpdateData.Data);
  FreePool (OptionList);
  return Status;
}

/**
  Call the browser and display the front page

  @return   Status code that will be returned by
            EFI_FORM_BROWSER2_PROTOCOL.SendForm ().

**/
EFI_STATUS
CallFrontPage (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;

  //
  // Begin waiting for USER INPUT
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_INPUT_WAIT)
    );

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = gFormBrowser2->SendForm (
                            gFormBrowser2,
                            &gFrontPagePrivate.HiiHandle,
                            1,
                            NULL,
                            0,
                            NULL,
                            &ActionRequest
                            );
  //
  // Check whether user  change any option setting which needs a reset to be effective
  //
  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    EnableResetRequired ();
  }

  return Status;
}

/**
  Acquire the string associated with the ProducerGuid and return it.


  @param ProducerGuid    The Guid to search the HII database for
  @param Token           The token value of the string to extract
  @param String          The string that is extracted

  @retval  EFI_SUCCESS  The function returns EFI_SUCCESS always.

**/
EFI_STATUS
GetProducerString (
  IN      EFI_GUID                  *ProducerGuid,
  IN      EFI_STRING_ID             Token,
  OUT     CHAR16                    **String
  )
{
  EFI_STATUS      Status;

  Status = HiiLibGetStringFromToken (ProducerGuid, Token, String);
  if (EFI_ERROR (Status)) {
    *String = GetStringById (STRING_TOKEN (STR_MISSING_STRING));
  }

  return EFI_SUCCESS;
}

/**
  Convert Processor Frequency Data to a string

  @param ProcessorFrequency The frequency data to process
  @param String             The string that is created

**/
VOID
ConvertProcessorToString (
  IN  EFI_PROCESSOR_CORE_FREQUENCY_DATA *ProcessorFrequency,
  OUT CHAR16                            **String
  )
{
  CHAR16  *StringBuffer;
  UINTN   Index;
  UINT32  FreqMhz;

  if (ProcessorFrequency->Exponent >= 6) {
    FreqMhz = ProcessorFrequency->Value;
    for (Index = 0; Index < (UINTN) (ProcessorFrequency->Exponent - 6); Index++) {
      FreqMhz *= 10;
    }
  } else {
    FreqMhz = 0;
  }

  StringBuffer = AllocateZeroPool (0x20);
  ASSERT (StringBuffer != NULL);
  Index = UnicodeValueToString (StringBuffer, LEFT_JUSTIFY, FreqMhz / 1000, 3);
  StrCat (StringBuffer, L".");
  UnicodeValueToString (StringBuffer + Index + 1, PREFIX_ZERO, (FreqMhz % 1000) / 10, 2);
  StrCat (StringBuffer, L" GHz");

  *String = (CHAR16 *) StringBuffer;

  return ;
}

/**
  Convert Memory Size to a string.

  @param MemorySize      The size of the memory to process
  @param String          The string that is created

**/
VOID
ConvertMemorySizeToString (
  IN  UINT32          MemorySize,
  OUT CHAR16          **String
  )
{
  CHAR16  *StringBuffer;

  StringBuffer = AllocateZeroPool (0x20);
  ASSERT (StringBuffer != NULL);
  UnicodeValueToString (StringBuffer, LEFT_JUSTIFY, MemorySize, 6);
  StrCat (StringBuffer, L" MB RAM");

  *String = (CHAR16 *) StringBuffer;

  return ;
}

/**
  Update the banner information for the Front Page based on DataHub information.

**/
VOID
UpdateFrontPageStrings (
  VOID
  )
{
  EFI_STATUS                        Status;
  EFI_STRING_ID                     TokenToUpdate;
  CHAR16                            *NewString;
  UINT64                            MonotonicCount;
  EFI_DATA_HUB_PROTOCOL             *DataHub;
  EFI_DATA_RECORD_HEADER            *Record;
  EFI_SUBCLASS_TYPE1_HEADER         *DataHeader;
  EFI_MISC_BIOS_VENDOR_DATA         *BiosVendor;
  EFI_MISC_SYSTEM_MANUFACTURER_DATA *SystemManufacturer;
  EFI_PROCESSOR_VERSION_DATA        *ProcessorVersion;
  EFI_PROCESSOR_CORE_FREQUENCY_DATA *ProcessorFrequency;
  EFI_MEMORY_ARRAY_START_ADDRESS_DATA    *MemoryArray;
  BOOLEAN                           Find[5];

  ZeroMem (Find, sizeof (Find));

  //
  // Update Front Page strings
  //
  Status = gBS->LocateProtocol (
                  &gEfiDataHubProtocolGuid,
                  NULL,
                  (VOID **) &DataHub
                  );
  ASSERT_EFI_ERROR (Status);

  MonotonicCount  = 0;
  Record          = NULL;
  do {
    Status = DataHub->GetNextRecord (DataHub, &MonotonicCount, NULL, &Record);
    if (Record->DataRecordClass == EFI_DATA_RECORD_CLASS_DATA) {
      DataHeader = (EFI_SUBCLASS_TYPE1_HEADER *) (Record + 1);
      if (CompareGuid (&Record->DataRecordGuid, &gEfiMiscSubClassGuid) &&
          (DataHeader->RecordType == EFI_MISC_BIOS_VENDOR_RECORD_NUMBER)
          ) {
        BiosVendor = (EFI_MISC_BIOS_VENDOR_DATA *) (DataHeader + 1);
        GetProducerString (&Record->ProducerName, BiosVendor->BiosVersion, &NewString);
        TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_BIOS_VERSION);
        HiiLibSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString);
        FreePool (NewString);
        Find[0] = TRUE;
      }

      if (CompareGuid (&Record->DataRecordGuid, &gEfiMiscSubClassGuid) &&
          (DataHeader->RecordType == EFI_MISC_SYSTEM_MANUFACTURER_RECORD_NUMBER)
          ) {
        SystemManufacturer = (EFI_MISC_SYSTEM_MANUFACTURER_DATA *) (DataHeader + 1);
        GetProducerString (&Record->ProducerName, SystemManufacturer->SystemProductName, &NewString);
        TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_COMPUTER_MODEL);
        HiiLibSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString);
        FreePool (NewString);
        Find[1] = TRUE;
      }

      if (CompareGuid (&Record->DataRecordGuid, &gEfiProcessorSubClassGuid) &&
          (DataHeader->RecordType == ProcessorVersionRecordType)
          ) {
        ProcessorVersion = (EFI_PROCESSOR_VERSION_DATA *) (DataHeader + 1);
        GetProducerString (&Record->ProducerName, *ProcessorVersion, &NewString);
        TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_CPU_MODEL);
        HiiLibSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString);
        FreePool (NewString);
        Find[2] = TRUE;
      }

      if (CompareGuid (&Record->DataRecordGuid, &gEfiProcessorSubClassGuid) &&
          (DataHeader->RecordType == ProcessorCoreFrequencyRecordType)
          ) {
        ProcessorFrequency = (EFI_PROCESSOR_CORE_FREQUENCY_DATA *) (DataHeader + 1);
        ConvertProcessorToString (ProcessorFrequency, &NewString);
        TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_CPU_SPEED);
        HiiLibSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString);
        FreePool (NewString);
        Find[3] = TRUE;
      }

      if (CompareGuid (&Record->DataRecordGuid, &gEfiMemorySubClassGuid) &&
          (DataHeader->RecordType == EFI_MEMORY_ARRAY_START_ADDRESS_RECORD_NUMBER)
          ) {
        MemoryArray = (EFI_MEMORY_ARRAY_START_ADDRESS_DATA *) (DataHeader + 1);
        ConvertMemorySizeToString (
          (UINT32)(RShiftU64((MemoryArray->MemoryArrayEndAddress - MemoryArray->MemoryArrayStartAddress + 1), 20)),
          &NewString
          );
        TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_MEMORY_SIZE);
        HiiLibSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString);
        FreePool (NewString);
        Find[4] = TRUE;
      }
    }
  } while (!EFI_ERROR (Status) && (MonotonicCount != 0) && !(Find[0] && Find[1] && Find[2] && Find[3] && Find[4]));

  return ;
}

/**
  Function waits for a given event to fire, or for an optional timeout to expire.

  @param Event              The event to wait for
  @param Timeout            An optional timeout value in 100 ns units.

  @retval  EFI_SUCCESS      Event fired before Timeout expired.
  @retval  EFI_TIME_OUT     Timout expired before Event fired..

**/
EFI_STATUS
WaitForSingleEvent (
  IN EFI_EVENT                  Event,
  IN UINT64                     Timeout OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  EFI_EVENT   TimerEvent;
  EFI_EVENT   WaitList[2];

  if (Timeout != 0) {
    //
    // Create a timer event
    //
    Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);
    if (!EFI_ERROR (Status)) {
      //
      // Set the timer event
      //
      gBS->SetTimer (
            TimerEvent,
            TimerRelative,
            Timeout
            );

      //
      // Wait for the original event or the timer
      //
      WaitList[0] = Event;
      WaitList[1] = TimerEvent;
      Status      = gBS->WaitForEvent (2, WaitList, &Index);
      gBS->CloseEvent (TimerEvent);

      //
      // If the timer expired, change the return to timed out
      //
      if (!EFI_ERROR (Status) && Index == 1) {
        Status = EFI_TIMEOUT;
      }
    }
  } else {
    //
    // No timeout... just wait on the event
    //
    Status = gBS->WaitForEvent (1, &Event, &Index);
    ASSERT (!EFI_ERROR (Status));
    ASSERT (Index == 0);
  }

  return Status;
}

/**
  Function show progress bar to wait for user input.


  @param TimeoutDefault  The fault time out value before the system continue to boot.

  @retval  EFI_SUCCESS       User pressed some key except "Enter"
  @retval  EFI_TIME_OUT      Timout expired or user press "Enter"

**/
EFI_STATUS
ShowProgress (
  IN UINT16                       TimeoutDefault
  )
{
  EFI_STATUS                    Status;
  CHAR16                        *TmpStr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color;
  EFI_INPUT_KEY                 Key;
  UINT16                        TimeoutRemain;

  if (TimeoutDefault == 0) {
    return EFI_TIMEOUT;
  }

  DEBUG ((EFI_D_INFO, "\n\nStart showing progress bar... Press any key to stop it! ...Zzz....\n"));
  
  SetMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
  SetMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);
  SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);

  //
  // Clear the progress status bar first
  //
  TmpStr = GetStringById (STRING_TOKEN (STR_START_BOOT_OPTION));
  if (TmpStr != NULL) {
    PlatformBdsShowProgress (Foreground, Background, TmpStr, Color, 0, 0);
  }

  TimeoutRemain = TimeoutDefault;
  while (TimeoutRemain != 0) {
    DEBUG ((EFI_D_INFO, "Showing progress bar...Remaining %d second!\n", TimeoutRemain));
    
    Status = WaitForSingleEvent (gST->ConIn->WaitForKey, ONE_SECOND);
    if (Status != EFI_TIMEOUT) {
      break;
    }
    TimeoutRemain--;

    //
    // Show progress
    //
    if (TmpStr != NULL) {
      PlatformBdsShowProgress (
        Foreground,
        Background,
        TmpStr,
        Color,
        ((TimeoutDefault - TimeoutRemain) * 100 / TimeoutDefault),
        0
        );
    }
  }
  gBS->FreePool (TmpStr);

  //
  // Timeout expired
  //
  if (TimeoutRemain == 0) {
    return EFI_TIMEOUT;
  }

  //
  // User pressed some key
  //
  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
    //
    // User pressed enter, equivalent to select "continue"
    //
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  This function is the main entry of the platform setup entry.
  The function will present the main menu of the system setup,
  this is the platform reference part and can be customize.


  @param TimeoutDefault     The fault time out value before the system
                            continue to boot.
  @param ConnectAllHappened The indicater to check if the connect all have
                            already happended.

**/
VOID
PlatformBdsEnterFrontPage (
  IN UINT16                       TimeoutDefault,
  IN BOOLEAN                      ConnectAllHappened
  )
{
  EFI_STATUS                    Status;

  PERF_START (0, "BdsTimeOut", "BDS", 0);
  //
  // Indicate if we need connect all in the platform setup
  //
  if (ConnectAllHappened) {
    gConnectAllHappened = TRUE;
  }

  if (TimeoutDefault != 0xffff) {
    Status = ShowProgress (TimeoutDefault);

    //
    // Ensure screen is clear when switch Console from Graphics mode to Text mode
    //
    gST->ConOut->EnableCursor (gST->ConOut, TRUE);
    gST->ConOut->ClearScreen (gST->ConOut);

    if (EFI_ERROR (Status)) {
      //
      // Timeout or user press enter to continue
      //
      goto Exit;
    }
  }

  do {

    InitializeFrontPage (FALSE);

    //
    // Update Front Page strings
    //
    UpdateFrontPageStrings ();

    gCallbackKey = 0;
    Status = CallFrontPage ();

    //
    // If gCallbackKey is greater than 1 and less or equal to 5,
    // it will lauch configuration utilities.
    // 2 = set language
    // 3 = boot manager
    // 4 = device manager
    // 5 = boot maintainenance manager
    //
    if (gCallbackKey != 0) {
      REPORT_STATUS_CODE (
        EFI_PROGRESS_CODE,
        (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_USER_SETUP)
        );
    }
    //
    // Based on the key that was set, we can determine what to do
    //
    switch (gCallbackKey) {
    //
    // The first 4 entries in the Front Page are to be GUARANTEED to remain constant so IHV's can
    // describe to their customers in documentation how to find their setup information (namely
    // under the device manager and specific buckets)
    //
    // These entries consist of the Continue, Select language, Boot Manager, and Device Manager
    //
    case FRONT_PAGE_KEY_CONTINUE:
      //
      // User hit continue
      //
      break;

    case FRONT_PAGE_KEY_LANGUAGE:
      //
      // User made a language setting change - display front page again
      //
      break;

    case FRONT_PAGE_KEY_BOOT_MANAGER:
      //
      // User chose to run the Boot Manager
      //
      CallBootManager ();
      break;

    case FRONT_PAGE_KEY_DEVICE_MANAGER:
      //
      // Display the Device Manager
      //
      do {
        CallDeviceManager();
      } while (gCallbackKey == FRONT_PAGE_KEY_DEVICE_MANAGER);
      break;

    case FRONT_PAGE_KEY_BOOT_MAINTAIN:
      //
      // Display the Boot Maintenance Manager
      //
      BdsStartBootMaint ();
      break;
    }

  } while ((Status == EFI_SUCCESS) && (gCallbackKey != FRONT_PAGE_KEY_CONTINUE));

  //
  //Will leave browser, check any reset required change is applied? if yes, reset system
  //
  SetupResetReminder ();

Exit:
  //
  // Automatically load current entry
  // Note: The following lines of code only execute when Auto boot
  // takes affect
  //
  PERF_END (0, "BdsTimeOut", "BDS", 0);
}
