/** @file
  Hotkey library functions.

Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalBm.h"

//
// Lock for linked list
//
EFI_LOCK                     mBmHotkeyLock            = EFI_INITIALIZE_LOCK_VARIABLE (TPL_NOTIFY);
LIST_ENTRY                   mBmHotkeyList            = INITIALIZE_LIST_HEAD_VARIABLE (mBmHotkeyList);
EFI_EVENT                    mBmHotkeyTriggered       = NULL;
BOOLEAN                      mBmHotkeyServiceStarted  = FALSE;
UINTN                        mBmHotkeySupportCount    = 0;

//
// Set OptionNumber as unassigned value to indicate the option isn't initialized
//
EFI_BOOT_MANAGER_LOAD_OPTION mBmHotkeyBootOption      = { LoadOptionNumberUnassigned };

EFI_BOOT_MANAGER_KEY_OPTION  *mBmContinueKeyOption    = NULL;
VOID                         *mBmTxtInExRegistration  = NULL;

/**

  Check whether the input key option is valid.

  @param   KeyOption               Input key option info.

  @retval  TRUE               Input key option is valid.
  @retval  FALSE              Input key option is not valid.
**/
BOOLEAN
BmIsKeyOptionValid (
  IN EFI_BOOT_MANAGER_KEY_OPTION     *KeyOption
)
{
  UINT16   OptionName[sizeof (L"Boot####")];
  UINT8    *BootOption;
  UINTN    BootOptionSize;
  UINT32   Crc;

  //
  // Check whether corresponding Boot Option exist
  //
  UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", KeyOption->BootOption);
  GetEfiGlobalVariable2 (OptionName, (VOID **) &BootOption, &BootOptionSize);

  if (BootOption == NULL) {
    return FALSE;
  }

  //
  // Check CRC for Boot Option
  //
  gBS->CalculateCrc32 (BootOption, BootOptionSize, &Crc);
  FreePool (BootOption);

  return (BOOLEAN) (KeyOption->BootOptionCrc == Crc);
}

/**

  Check whether the input variable is an key option variable.

  @param   Name               Input variable name.
  @param   Guid               Input variable guid.
  @param   OptionNumber       The option number of this key option variable.

  @retval  TRUE               Input variable is a key option variable.
  @retval  FALSE              Input variable is not a key option variable.
**/
BOOLEAN
BmIsKeyOptionVariable (
  CHAR16        *Name,
  EFI_GUID      *Guid,
  UINT16        *OptionNumber
  )
{
  UINTN         Index;
  UINTN         Uint;
  
  if (!CompareGuid (Guid, &gEfiGlobalVariableGuid) ||
      (StrSize (Name) != sizeof (L"Key####")) ||
      (StrnCmp (Name, L"Key", 3) != 0)
     ) {
    return FALSE;
  }

  *OptionNumber = 0;
  for (Index = 3; Index < 7; Index++) {
    Uint = BmCharToUint (Name[Index]);
    if (Uint == -1) {
      return FALSE;
    } else {
      *OptionNumber = (UINT16) Uint + *OptionNumber * 0x10;
    }
  }

  return TRUE;
}

/**
  Return the buffer size of the EFI_BOOT_MANAGER_KEY_OPTION data.

  @param   KeyOption            The input key option info.

  @retval  The buffer size of the key option data.
**/
UINTN
BmSizeOfKeyOption (
  EFI_BOOT_MANAGER_KEY_OPTION  *KeyOption
  )
{
  return OFFSET_OF (EFI_BOOT_MANAGER_KEY_OPTION, Keys)
    + KeyOption->KeyData.Options.InputKeyCount * sizeof (EFI_INPUT_KEY);
}

/**
  Return the array of key options.

  @param Count  Return the number of key options.

  @retval NULL  No key option.
  @retval Other Pointer to the key options.
**/
EFI_BOOT_MANAGER_KEY_OPTION *
BmGetKeyOptions (
  OUT UINTN     *Count
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index;
  CHAR16                      *Name;
  EFI_GUID                    Guid;
  UINTN                       NameSize;
  UINTN                       NewNameSize;
  EFI_BOOT_MANAGER_KEY_OPTION *KeyOptions;
  EFI_BOOT_MANAGER_KEY_OPTION *KeyOption;
  UINT16                      OptionNumber;

  if (Count == NULL) {
    return NULL;
  }

  *Count     = 0;
  KeyOptions = NULL;

  NameSize = sizeof (CHAR16);
  Name     = AllocateZeroPool (NameSize);
  ASSERT (Name != NULL);
  while (TRUE) {
    NewNameSize = NameSize;
    Status = gRT->GetNextVariableName (&NewNameSize, Name, &Guid);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Name = ReallocatePool (NameSize, NewNameSize, Name);
      ASSERT (Name != NULL);
      Status = gRT->GetNextVariableName (&NewNameSize, Name, &Guid);
      NameSize = NewNameSize;
    }

    if (Status == EFI_NOT_FOUND) {
      break;
    }
    ASSERT_EFI_ERROR (Status);

    if (BmIsKeyOptionVariable (Name ,&Guid, &OptionNumber)) {
      GetEfiGlobalVariable2 (Name, (VOID**) &KeyOption, NULL);
      ASSERT (KeyOption != NULL);
      if (BmIsKeyOptionValid (KeyOption)) {
        KeyOptions = ReallocatePool (
                       *Count * sizeof (EFI_BOOT_MANAGER_KEY_OPTION),
                       (*Count + 1) * sizeof (EFI_BOOT_MANAGER_KEY_OPTION),
                       KeyOptions
                       );
        ASSERT (KeyOptions != NULL);
        //
        // Insert the key option in order
        //
        for (Index = 0; Index < *Count; Index++) {
          if (OptionNumber < KeyOptions[Index].OptionNumber) {
            break;
          }
        }
        CopyMem (&KeyOptions[Index + 1], &KeyOptions[Index], (*Count - Index) * sizeof (EFI_BOOT_MANAGER_KEY_OPTION));
        CopyMem (&KeyOptions[Index], KeyOption, BmSizeOfKeyOption (KeyOption));
        KeyOptions[Index].OptionNumber = OptionNumber;
        (*Count)++;
      }
      FreePool (KeyOption);
    }
  }

  FreePool (Name);

  return KeyOptions;
}

/**
  Callback function for event.
  
  @param    Event          Event for this callback function.
  @param    Context        Context pass to this function.
**/
VOID
EFIAPI
BmEmptyFunction (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
}

/**
  Check whether the bit is set in the value.

  @param   Value            The value need to be check.
  @param   Bit              The bit filed need to be check.

  @retval  TRUE             The bit is set.
  @retval  FALSE            The bit is not set.
**/
BOOLEAN
BmBitSet (
  IN UINT32   Value,
  IN UINT32   Bit
  )
{
  return (BOOLEAN) ((Value & Bit) != 0);
}

/**
  Initialize the KeyData and Key[] in the EFI_BOOT_MANAGER_KEY_OPTION.

  @param  Modifier   Input key info.
  @param  Args       Va_list info.
  @param  KeyOption  Key info which need to update.

  @retval  EFI_SUCCESS             Succeed to initialize the KeyData and Key[].
  @return  EFI_INVALID_PARAMETER   Input parameter error.
**/
EFI_STATUS
BmInitializeKeyFields (
  IN UINT32                       Modifier,
  IN VA_LIST                      Args,
  OUT EFI_BOOT_MANAGER_KEY_OPTION *KeyOption
  )
{
  EFI_INPUT_KEY                   *Key;

  if (KeyOption == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Key = NULL;
  while (KeyOption->KeyData.Options.InputKeyCount < sizeof (KeyOption->Keys) / sizeof (KeyOption->Keys[0])) {
    Key = VA_ARG (Args, EFI_INPUT_KEY *);
    if (Key == NULL) {
      break;
    }
    CopyMem (
      &KeyOption->Keys[KeyOption->KeyData.Options.InputKeyCount],
      Key,
      sizeof (EFI_INPUT_KEY)
      );
    KeyOption->KeyData.Options.InputKeyCount++;
  }

  if (Key != NULL) {
    //
    // Too many keys
    //
    return EFI_INVALID_PARAMETER;
  }

  if ((Modifier & ~(EFI_BOOT_MANAGER_SHIFT_PRESSED
                 | EFI_BOOT_MANAGER_CONTROL_PRESSED
                 | EFI_BOOT_MANAGER_ALT_PRESSED
                 | EFI_BOOT_MANAGER_LOGO_PRESSED
                 | EFI_BOOT_MANAGER_MENU_KEY_PRESSED
                 | EFI_BOOT_MANAGER_SYS_REQ_PRESSED
                 )) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (BmBitSet (Modifier, EFI_BOOT_MANAGER_SHIFT_PRESSED)) {
    KeyOption->KeyData.Options.ShiftPressed = 1;
  }
  if (BmBitSet (Modifier, EFI_BOOT_MANAGER_CONTROL_PRESSED)) {
    KeyOption->KeyData.Options.ControlPressed = 1;
  }
  if (BmBitSet (Modifier, EFI_BOOT_MANAGER_ALT_PRESSED)) {
    KeyOption->KeyData.Options.AltPressed = 1;
  }
  if (BmBitSet (Modifier, EFI_BOOT_MANAGER_LOGO_PRESSED)) {
    KeyOption->KeyData.Options.LogoPressed = 1;
  }
  if (BmBitSet (Modifier, EFI_BOOT_MANAGER_MENU_KEY_PRESSED)) {
    KeyOption->KeyData.Options.MenuPressed = 1;
  }
  if (BmBitSet (Modifier, EFI_BOOT_MANAGER_SYS_REQ_PRESSED)) {
    KeyOption->KeyData.Options.SysReqPressed = 1;
  }

  return EFI_SUCCESS;
}

/**
  Try to boot the boot option triggered by hot key.
**/
VOID
EFIAPI
EfiBootManagerHotkeyBoot (
  VOID
  )
{
  if (mBmHotkeyBootOption.OptionNumber != LoadOptionNumberUnassigned) {
    EfiBootManagerBoot (&mBmHotkeyBootOption);
    EfiBootManagerFreeLoadOption (&mBmHotkeyBootOption);
    mBmHotkeyBootOption.OptionNumber = LoadOptionNumberUnassigned;
  }
}

/**
  This is the common notification function for HotKeys, it will be registered
  with SimpleTextInEx protocol interface - RegisterKeyNotify() of ConIn handle.

  @param KeyData         A pointer to a buffer that is filled in with the keystroke
                         information for the key that was pressed.

  @retval  EFI_SUCCESS   KeyData is successfully processed.
  @return  EFI_NOT_FOUND Fail to find boot option variable.
**/
EFI_STATUS
EFIAPI
BmHotkeyCallback (
  IN EFI_KEY_DATA     *KeyData
)
{
  LIST_ENTRY                    *Link;
  BM_HOTKEY                     *Hotkey;
  CHAR16                        OptionName[sizeof ("Boot####")];
  EFI_STATUS                    Status;
  EFI_KEY_DATA                  *HotkeyData;

  if (mBmHotkeyBootOption.OptionNumber != LoadOptionNumberUnassigned) {
    //
    // Do not process sequential hotkey stroke until the current boot option returns
    //
    return EFI_SUCCESS;
  }

  DEBUG ((EFI_D_INFO, "[Bds]BmHotkeyCallback: %04x:%04x\n", KeyData->Key.ScanCode, KeyData->Key.UnicodeChar));

  EfiAcquireLock (&mBmHotkeyLock);
  for ( Link = GetFirstNode (&mBmHotkeyList)
      ; !IsNull (&mBmHotkeyList, Link)
      ; Link = GetNextNode (&mBmHotkeyList, Link)
      ) {
    Hotkey = BM_HOTKEY_FROM_LINK (Link);

    //
    // Is this Key Stroke we are waiting for?
    //
    ASSERT (Hotkey->WaitingKey < (sizeof (Hotkey->KeyData) / sizeof (Hotkey->KeyData[0])));
    HotkeyData = &Hotkey->KeyData[Hotkey->WaitingKey];
    if ((KeyData->Key.ScanCode == HotkeyData->Key.ScanCode) &&
        (KeyData->Key.UnicodeChar == HotkeyData->Key.UnicodeChar) &&
        (((KeyData->KeyState.KeyShiftState & EFI_SHIFT_STATE_VALID) != 0) ? 
          (KeyData->KeyState.KeyShiftState == HotkeyData->KeyState.KeyShiftState) : TRUE
        )
       ) {

      //
      // Receive an expecting key stroke, transit to next waiting state
      //
      Hotkey->WaitingKey++;

      if (Hotkey->WaitingKey == Hotkey->CodeCount) {
        //
        // Reset to initial waiting state
        //
        Hotkey->WaitingKey = 0;
        //
        // Received the whole key stroke sequence
        //
        Status = gBS->SignalEvent (mBmHotkeyTriggered);
        ASSERT_EFI_ERROR (Status);

        if (!Hotkey->IsContinue) {
          //
          // Launch its BootOption
          //
          UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", Hotkey->BootOption);
          Status = EfiBootManagerVariableToLoadOption (OptionName, &mBmHotkeyBootOption);
          DEBUG ((EFI_D_INFO, "[Bds]Hotkey for %s pressed - %r\n", OptionName, Status));
          if (EFI_ERROR (Status)) {
            mBmHotkeyBootOption.OptionNumber = LoadOptionNumberUnassigned;
          }
        } else {
          DEBUG ((EFI_D_INFO, "[Bds]Continue key pressed!\n"));
        }
      }
    } else {
      //
      // Receive an unexpected key stroke, reset to initial waiting state
      //
      Hotkey->WaitingKey = 0;
    }

  }
  EfiReleaseLock (&mBmHotkeyLock);

  return EFI_SUCCESS;
}

/**
  Unregister hotkey notify list.

  @param    Hotkey                Hotkey list.

  @retval   EFI_SUCCESS           Unregister hotkey notify success.
  @retval   Others                Unregister hotkey notify failed.
**/
EFI_STATUS
BmUnregisterHotkeyNotify (
  IN BM_HOTKEY                          *Hotkey
  )
{
  EFI_STATUS                            Status;
  UINTN                                 Index;
  UINTN                                 KeyIndex;
  EFI_HANDLE                            *Handles;
  UINTN                                 HandleCount;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL     *TxtInEx;
  VOID                                  *NotifyHandle;

  gBS->LocateHandleBuffer (
          ByProtocol,
          &gEfiSimpleTextInputExProtocolGuid,
          NULL,
          &HandleCount,
          &Handles
          );
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (Handles[Index], &gEfiSimpleTextInputExProtocolGuid, (VOID **) &TxtInEx);
    ASSERT_EFI_ERROR (Status);
    for (KeyIndex = 0; KeyIndex < Hotkey->CodeCount; KeyIndex++) {
      Status = TxtInEx->RegisterKeyNotify (
                          TxtInEx,
                          &Hotkey->KeyData[KeyIndex],
                          BmHotkeyCallback,
                          &NotifyHandle
                          );
      if (!EFI_ERROR (Status)) {
        Status = TxtInEx->UnregisterKeyNotify (TxtInEx, NotifyHandle);
        DEBUG ((EFI_D_INFO, "[Bds]UnregisterKeyNotify: %04x/%04x %r\n", Hotkey->KeyData[KeyIndex].Key.ScanCode, Hotkey->KeyData[KeyIndex].Key.UnicodeChar, Status));
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Register hotkey notify list.

  @param    TxtInEx               Pointer to EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL protocol.
  @param    Hotkey                Hotkey list.

  @retval   EFI_SUCCESS           Register hotkey notify success.
  @retval   Others                Register hotkey notify failed.
**/
EFI_STATUS
BmRegisterHotkeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TxtInEx,
  IN BM_HOTKEY                          *Hotkey
  )
{
  EFI_STATUS                            Status;
  UINTN                                 Index;
  VOID                                  *NotifyHandle;

  for (Index = 0; Index < Hotkey->CodeCount; Index++) {
    Status = TxtInEx->RegisterKeyNotify (
                        TxtInEx,
                        &Hotkey->KeyData[Index],
                        BmHotkeyCallback,
                        &NotifyHandle
                        );
    DEBUG ((
      EFI_D_INFO,
      "[Bds]RegisterKeyNotify: %04x/%04x %08x/%02x %r\n",
      Hotkey->KeyData[Index].Key.ScanCode,
      Hotkey->KeyData[Index].Key.UnicodeChar,
      Hotkey->KeyData[Index].KeyState.KeyShiftState,
      Hotkey->KeyData[Index].KeyState.KeyToggleState,
      Status
      ));
    if (EFI_ERROR (Status)) {
      //
      // some of the hotkey registry failed
      // do not unregister all in case we have both CTRL-ALT-P and CTRL-ALT-P-R
      //
      break;
    }
  }

  return EFI_SUCCESS;
}

/**
  Generate key shift state base on the input key option info.

  @param    Depth                 Which key is checked.
  @param    KeyOption             Input key option info.
  @param    KeyShiftState         Input key shift state.
  @param    KeyShiftStates        Return possible key shift state array.
  @param    KeyShiftStateCount    Possible key shift state count.
**/
VOID
BmGenerateKeyShiftState (
  IN UINTN                             Depth,
  IN EFI_BOOT_MANAGER_KEY_OPTION       *KeyOption,
  IN UINT32                            KeyShiftState,
  IN UINT32                            *KeyShiftStates,
  IN UINTN                             *KeyShiftStateCount
  )
{
  switch (Depth) {
  case 0:
    if (KeyOption->KeyData.Options.ShiftPressed) {
      BmGenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_RIGHT_SHIFT_PRESSED, KeyShiftStates, KeyShiftStateCount);
      BmGenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_LEFT_SHIFT_PRESSED,  KeyShiftStates, KeyShiftStateCount);
    } else {
      BmGenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState, KeyShiftStates, KeyShiftStateCount);
    }
    break;

  case 1:
    if (KeyOption->KeyData.Options.ControlPressed) {
      BmGenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_RIGHT_CONTROL_PRESSED, KeyShiftStates, KeyShiftStateCount);
      BmGenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_LEFT_CONTROL_PRESSED,  KeyShiftStates, KeyShiftStateCount);
    } else {
      BmGenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState, KeyShiftStates, KeyShiftStateCount);
    }
    break;

  case 2:
    if (KeyOption->KeyData.Options.AltPressed) {
      BmGenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_RIGHT_ALT_PRESSED, KeyShiftStates, KeyShiftStateCount);
      BmGenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_LEFT_ALT_PRESSED,  KeyShiftStates, KeyShiftStateCount);
    } else {
      BmGenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState, KeyShiftStates, KeyShiftStateCount);
    }
    break;
  case  3:
    if (KeyOption->KeyData.Options.LogoPressed) {
      BmGenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_RIGHT_LOGO_PRESSED, KeyShiftStates, KeyShiftStateCount);
      BmGenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState | EFI_LEFT_LOGO_PRESSED,  KeyShiftStates, KeyShiftStateCount);
    } else {
      BmGenerateKeyShiftState (Depth + 1, KeyOption, KeyShiftState, KeyShiftStates, KeyShiftStateCount);
    }
    break;
  case 4:
    if (KeyOption->KeyData.Options.MenuPressed) {
      KeyShiftState |= EFI_MENU_KEY_PRESSED;
    }
    if (KeyOption->KeyData.Options.SysReqPressed) {
      KeyShiftState |= EFI_SYS_REQ_PRESSED;
    }
    KeyShiftStates[*KeyShiftStateCount] = KeyShiftState;
    (*KeyShiftStateCount)++;
    break;
  }
}

/**
  Add it to hot key database, register it to existing TxtInEx.
  New TxtInEx will be automatically registered with all the hot key in dababase

  @param    KeyOption  Input key option info.
**/
EFI_STATUS
BmProcessKeyOption (
  IN EFI_BOOT_MANAGER_KEY_OPTION       *KeyOption
  )
{
  EFI_STATUS                           Status;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL    *TxtInEx;
  EFI_HANDLE                           *Handles;
  UINTN                                HandleCount;
  UINTN                                HandleIndex;
  UINTN                                Index;
  BM_HOTKEY                            *Hotkey;
  UINTN                                KeyIndex;
  //
  // 16 is enough to enumerate all the possible combination of LEFT_XXX and RIGHT_XXX
  //
  UINT32                               KeyShiftStates[16];
  UINTN                                KeyShiftStateCount;

  if (KeyOption->KeyData.Options.InputKeyCount > mBmHotkeySupportCount) {
    return EFI_UNSUPPORTED;
  }

  KeyShiftStateCount = 0;
  BmGenerateKeyShiftState (0, KeyOption, EFI_SHIFT_STATE_VALID, KeyShiftStates, &KeyShiftStateCount);
  ASSERT (KeyShiftStateCount <= sizeof (KeyShiftStates) / sizeof (KeyShiftStates[0]));

  EfiAcquireLock (&mBmHotkeyLock);

  for (Index = 0; Index < KeyShiftStateCount; Index++) {
    Hotkey = AllocateZeroPool (sizeof (BM_HOTKEY));
    ASSERT (Hotkey != NULL);

    Hotkey->Signature  = BM_HOTKEY_SIGNATURE;
    Hotkey->BootOption = KeyOption->BootOption;
    Hotkey->IsContinue = (BOOLEAN) (KeyOption == mBmContinueKeyOption);
    Hotkey->CodeCount  = (UINT8) KeyOption->KeyData.Options.InputKeyCount;

    for (KeyIndex = 0; KeyIndex < Hotkey->CodeCount; KeyIndex++) {
      CopyMem (&Hotkey->KeyData[KeyIndex].Key, &KeyOption->Keys[KeyIndex], sizeof (EFI_INPUT_KEY));
      Hotkey->KeyData[KeyIndex].KeyState.KeyShiftState = KeyShiftStates[Index];
    }
    InsertTailList (&mBmHotkeyList, &Hotkey->Link);

    gBS->LocateHandleBuffer (
            ByProtocol,
            &gEfiSimpleTextInputExProtocolGuid,
            NULL,
            &HandleCount,
            &Handles
            );
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
      Status = gBS->HandleProtocol (Handles[HandleIndex], &gEfiSimpleTextInputExProtocolGuid, (VOID **) &TxtInEx);
      ASSERT_EFI_ERROR (Status);
      BmRegisterHotkeyNotify (TxtInEx, Hotkey);
    }
  }

  EfiReleaseLock (&mBmHotkeyLock);

  return EFI_SUCCESS;
}

/**
  Callback function for SimpleTextInEx protocol install events

  @param Event           the event that is signaled.
  @param Context         not used here.

**/
VOID
EFIAPI
BmTxtInExCallback (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_STATUS                         Status;
  UINTN                              BufferSize;
  EFI_HANDLE                         Handle;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TxtInEx;
  LIST_ENTRY                         *Link;

  while (TRUE) {
    BufferSize = sizeof (EFI_HANDLE);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    NULL,
                    mBmTxtInExRegistration,
                    &BufferSize,
                    &Handle
                    );
    if (EFI_ERROR (Status)) {
      //
      // If no more notification events exist
      //
      return ;
    }

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiSimpleTextInputExProtocolGuid,
                    (VOID **) &TxtInEx
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Register the hot key notification for the existing items in the list
    //
    EfiAcquireLock (&mBmHotkeyLock);
    for (Link = GetFirstNode (&mBmHotkeyList); !IsNull (&mBmHotkeyList, Link); Link = GetNextNode (&mBmHotkeyList, Link)) {
      BmRegisterHotkeyNotify (TxtInEx, BM_HOTKEY_FROM_LINK (Link));
    }
    EfiReleaseLock (&mBmHotkeyLock);
  }
}

/**
  Free the key options returned from BmGetKeyOptions.

  @param KeyOptions     Pointer to the key options.
  @param KeyOptionCount Number of the key options.

  @retval EFI_SUCCESS   The key options are freed.
  @retval EFI_NOT_FOUND KeyOptions is NULL.
**/
EFI_STATUS
BmFreeKeyOptions (
  IN EFI_BOOT_MANAGER_KEY_OPTION    *KeyOptions,
  IN UINTN                          KeyOptionCount
  )
{
  if (KeyOptions != NULL) {
    FreePool (KeyOptions);
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

/**
  Register the key option to exit the waiting of the Boot Manager timeout.
  Platform should ensure that the continue key option isn't conflict with
  other boot key options.

  @param Modifier     Key shift state.
  @param  ...         Parameter list of pointer of EFI_INPUT_KEY.

  @retval EFI_SUCCESS         Successfully register the continue key option.
  @retval EFI_ALREADY_STARTED The continue key option is already registered.
**/
EFI_STATUS
EFIAPI
EfiBootManagerRegisterContinueKeyOption (
  IN UINT32           Modifier,
  ...
  )
{
  EFI_STATUS                   Status;
  EFI_BOOT_MANAGER_KEY_OPTION  KeyOption;
  VA_LIST                      Args;
  
  if (mBmContinueKeyOption != NULL) {
    return EFI_ALREADY_STARTED;
  }

  ZeroMem (&KeyOption, sizeof (EFI_BOOT_MANAGER_KEY_OPTION));
  VA_START (Args, Modifier);
  Status = BmInitializeKeyFields (Modifier, Args, &KeyOption);
  VA_END (Args);

  if (!EFI_ERROR (Status)) {
    mBmContinueKeyOption = AllocateCopyPool (sizeof (EFI_BOOT_MANAGER_KEY_OPTION), &KeyOption);
    ASSERT (mBmContinueKeyOption != NULL);
    if (mBmHotkeyServiceStarted) {
      BmProcessKeyOption (mBmContinueKeyOption);
    }
  }

  return Status;
}

/**
  Stop the hotkey processing.
  
  @param    Event          Event pointer related to hotkey service.
  @param    Context        Context pass to this function.
**/
VOID
EFIAPI
BmStopHotkeyService (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  LIST_ENTRY            *Link;
  BM_HOTKEY             *Hotkey;

  DEBUG ((EFI_D_INFO, "[Bds]Stop Hotkey Service!\n"));
  gBS->CloseEvent (Event);

  EfiAcquireLock (&mBmHotkeyLock);
  for (Link = GetFirstNode (&mBmHotkeyList); !IsNull (&mBmHotkeyList, Link); ) {
    Hotkey = BM_HOTKEY_FROM_LINK (Link);
    BmUnregisterHotkeyNotify (Hotkey);
    Link   = RemoveEntryList (Link);
    FreePool (Hotkey);
  }
  EfiReleaseLock (&mBmHotkeyLock);
}

/**
  Start the hot key service so that the key press can trigger the boot option.

  @param HotkeyTriggered  Return the waitable event and it will be signaled 
                          when a valid hot key is pressed.

  @retval EFI_SUCCESS     The hot key service is started.
**/
EFI_STATUS
EFIAPI
EfiBootManagerStartHotkeyService (
  IN EFI_EVENT                 *HotkeyTriggered
  )
{
  EFI_STATUS                   Status;
  EFI_BOOT_MANAGER_KEY_OPTION  *KeyOptions;
  UINTN                        KeyOptionCount;
  UINTN                        Index;
  EFI_EVENT                    Event;
  UINT32                       *BootOptionSupport;

  Status = GetEfiGlobalVariable2 (EFI_BOOT_OPTION_SUPPORT_VARIABLE_NAME, (VOID **) &BootOptionSupport, NULL);
  ASSERT (BootOptionSupport != NULL);

  if ((*BootOptionSupport & EFI_BOOT_OPTION_SUPPORT_KEY)  != 0) {
    mBmHotkeySupportCount = ((*BootOptionSupport & EFI_BOOT_OPTION_SUPPORT_COUNT) >> LowBitSet32 (EFI_BOOT_OPTION_SUPPORT_COUNT));
  }
  FreePool (BootOptionSupport);

  if (mBmHotkeySupportCount == 0) {
    DEBUG ((EFI_D_INFO, "Bds: BootOptionSupport NV variable forbids starting the hotkey service.\n"));
    return EFI_UNSUPPORTED;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_CALLBACK,
                  BmEmptyFunction,
                  NULL,
                  &mBmHotkeyTriggered
                  );
  ASSERT_EFI_ERROR (Status);

  if (HotkeyTriggered != NULL) {
    *HotkeyTriggered = mBmHotkeyTriggered;
  }

  KeyOptions = BmGetKeyOptions (&KeyOptionCount);
  for (Index = 0; Index < KeyOptionCount; Index ++) {
    BmProcessKeyOption (&KeyOptions[Index]);
  }
  BmFreeKeyOptions (KeyOptions, KeyOptionCount);

  if (mBmContinueKeyOption != NULL) {
    BmProcessKeyOption (mBmContinueKeyOption);
  }

  EfiCreateProtocolNotifyEvent (
    &gEfiSimpleTextInputExProtocolGuid,
    TPL_CALLBACK,
    BmTxtInExCallback,
    NULL,
    &mBmTxtInExRegistration
    );

  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             BmStopHotkeyService,
             NULL,
             &Event
             );
  ASSERT_EFI_ERROR (Status);


  mBmHotkeyServiceStarted = TRUE;
  return Status;
}

/**
  Add the key option.
  It adds the key option variable and the key option takes affect immediately.

  @param AddedOption      Return the added key option.
  @param BootOptionNumber The boot option number for the key option.
  @param Modifier         Key shift state.
  @param ...              Parameter list of pointer of EFI_INPUT_KEY.

  @retval EFI_SUCCESS         The key option is added.
  @retval EFI_ALREADY_STARTED The hot key is already used by certain key option.
**/
EFI_STATUS
EFIAPI
EfiBootManagerAddKeyOptionVariable (
  OUT EFI_BOOT_MANAGER_KEY_OPTION *AddedOption,   OPTIONAL
  IN UINT16                       BootOptionNumber,
  IN UINT32                       Modifier,
  ...
  )
{
  EFI_STATUS                     Status;
  VA_LIST                        Args;
  VOID                           *BootOption;
  UINTN                          BootOptionSize;
  CHAR16                         BootOptionName[sizeof (L"Boot####")];
  EFI_BOOT_MANAGER_KEY_OPTION    KeyOption;
  EFI_BOOT_MANAGER_KEY_OPTION    *KeyOptions;
  UINTN                          KeyOptionCount;
  UINTN                          Index;
  UINTN                          KeyOptionNumber;
  CHAR16                         KeyOptionName[sizeof (L"Key####")];

  UnicodeSPrint (BootOptionName, sizeof (BootOptionName), L"Boot%04x", BootOptionNumber);
  GetEfiGlobalVariable2 (BootOptionName, &BootOption, &BootOptionSize);

  if (BootOption == NULL) {
    return EFI_NOT_FOUND;
  }

  ZeroMem (&KeyOption, sizeof (EFI_BOOT_MANAGER_KEY_OPTION));
  KeyOption.BootOption = BootOptionNumber;
  Status = gBS->CalculateCrc32 (BootOption, BootOptionSize, &KeyOption.BootOptionCrc);
  ASSERT_EFI_ERROR (Status);
  FreePool (BootOption);

  VA_START (Args, Modifier);
  Status = BmInitializeKeyFields (Modifier, Args, &KeyOption);
  VA_END (Args);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  KeyOptionNumber = LoadOptionNumberUnassigned;
  //
  // Check if the hot key sequence was defined already
  //
  KeyOptions = BmGetKeyOptions (&KeyOptionCount);
  for (Index = 0; Index < KeyOptionCount; Index++) {
    if ((KeyOptions[Index].KeyData.PackedValue == KeyOption.KeyData.PackedValue) &&
      (CompareMem (KeyOptions[Index].Keys, KeyOption.Keys, KeyOption.KeyData.Options.InputKeyCount * sizeof (EFI_INPUT_KEY)) == 0)) {
      break;
    }

    if ((KeyOptionNumber == LoadOptionNumberUnassigned) &&
        (KeyOptions[Index].OptionNumber > Index)
       ){
      KeyOptionNumber = Index;
    }
  }
  BmFreeKeyOptions (KeyOptions, KeyOptionCount);

  if (Index < KeyOptionCount) {
    return EFI_ALREADY_STARTED;
  }

  if (KeyOptionNumber == LoadOptionNumberUnassigned) {
    KeyOptionNumber = KeyOptionCount;
  }

  UnicodeSPrint (KeyOptionName, sizeof (KeyOptionName), L"Key%04x", KeyOptionNumber);

  Status = gRT->SetVariable (
                  KeyOptionName,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  BmSizeOfKeyOption (&KeyOption),
                  &KeyOption
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Return the Key Option in case needed by caller
    //
    if (AddedOption != NULL) {
      CopyMem (AddedOption, &KeyOption, sizeof (EFI_BOOT_MANAGER_KEY_OPTION));
    }

    //
    // Register the newly added hot key
    // Calling this function before EfiBootManagerStartHotkeyService doesn't
    // need to call BmProcessKeyOption
    //
    if (mBmHotkeyServiceStarted) {
      BmProcessKeyOption (&KeyOption);
    }
  }

  return Status;
}

/**
  Delete the Key Option variable and unregister the hot key

  @param DeletedOption  Return the deleted key options.
  @param Modifier       Key shift state.
  @param ...            Parameter list of pointer of EFI_INPUT_KEY.

  @retval EFI_SUCCESS   The key option is deleted.
  @retval EFI_NOT_FOUND The key option cannot be found.
**/
EFI_STATUS
EFIAPI
EfiBootManagerDeleteKeyOptionVariable (
  IN EFI_BOOT_MANAGER_KEY_OPTION *DeletedOption, OPTIONAL
  IN UINT32                      Modifier,
  ...
  )
{
  EFI_STATUS                     Status;
  UINTN                          Index;
  VA_LIST                        Args;
  EFI_BOOT_MANAGER_KEY_OPTION    KeyOption;
  EFI_BOOT_MANAGER_KEY_OPTION    *KeyOptions;
  UINTN                          KeyOptionCount;
  LIST_ENTRY                     *Link;
  BM_HOTKEY                      *Hotkey;
  UINT32                         ShiftState;
  BOOLEAN                        Match;
  CHAR16                         KeyOptionName[sizeof (L"Key####")];

  ZeroMem (&KeyOption, sizeof (EFI_BOOT_MANAGER_KEY_OPTION));
  VA_START (Args, Modifier);
  Status = BmInitializeKeyFields (Modifier, Args, &KeyOption);
  VA_END (Args);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  EfiAcquireLock (&mBmHotkeyLock);
  //
  // Delete the key option from active hot key list
  // Could have multiple entries when modifier isn't 0 because we map the ShiftPressed to RIGHT_SHIFT and RIGHT_SHIFT
  //
  for (Link = GetFirstNode (&mBmHotkeyList); !IsNull (&mBmHotkeyList, Link); ) {
    Hotkey = BM_HOTKEY_FROM_LINK (Link);
    Match  = (BOOLEAN) (Hotkey->CodeCount == KeyOption.KeyData.Options.InputKeyCount);

    for (Index = 0; Match && (Index < Hotkey->CodeCount); Index++) {
      ShiftState = Hotkey->KeyData[Index].KeyState.KeyShiftState;
      if (
        (BmBitSet (ShiftState, EFI_RIGHT_SHIFT_PRESSED | EFI_LEFT_SHIFT_PRESSED) != KeyOption.KeyData.Options.ShiftPressed) ||
        (BmBitSet (ShiftState, EFI_RIGHT_CONTROL_PRESSED | EFI_LEFT_CONTROL_PRESSED) != KeyOption.KeyData.Options.ControlPressed) ||
        (BmBitSet (ShiftState, EFI_RIGHT_ALT_PRESSED | EFI_LEFT_ALT_PRESSED) != KeyOption.KeyData.Options.AltPressed) ||
        (BmBitSet (ShiftState, EFI_RIGHT_LOGO_PRESSED | EFI_LEFT_LOGO_PRESSED) != KeyOption.KeyData.Options.LogoPressed) ||
        (BmBitSet (ShiftState, EFI_MENU_KEY_PRESSED) != KeyOption.KeyData.Options.MenuPressed) ||
        (BmBitSet (ShiftState, EFI_SYS_REQ_PRESSED) != KeyOption.KeyData.Options.SysReqPressed) ||
        (CompareMem (&Hotkey->KeyData[Index].Key, &KeyOption.Keys[Index], sizeof (EFI_INPUT_KEY)) != 0)
        ) {
        //
        // Break when any field doesn't match
        //
        Match = FALSE;
        break;
      }
    }

    if (Match) {
      Link = RemoveEntryList (Link);
      FreePool (Hotkey);
    } else {
      Link = GetNextNode (&mBmHotkeyList, Link);
    }
  }

  //
  // Delete the key option from the variable
  //
  Status     = EFI_NOT_FOUND;
  KeyOptions = BmGetKeyOptions (&KeyOptionCount);
  for (Index = 0; Index < KeyOptionCount; Index++) {
    if ((KeyOptions[Index].KeyData.PackedValue == KeyOption.KeyData.PackedValue) &&
        (CompareMem (
           KeyOptions[Index].Keys, KeyOption.Keys,
           KeyOption.KeyData.Options.InputKeyCount * sizeof (EFI_INPUT_KEY)) == 0)
       ) {
      UnicodeSPrint (KeyOptionName, sizeof (KeyOptionName), L"Key%04x", KeyOptions[Index].OptionNumber);
      Status = gRT->SetVariable (
                 KeyOptionName,
                 &gEfiGlobalVariableGuid,
                 EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                 0,
                 NULL
                 );
      //
      // Return the deleted key option in case needed by caller
      //
      if (DeletedOption != NULL) {
        CopyMem (DeletedOption, &KeyOptions[Index], sizeof (EFI_BOOT_MANAGER_KEY_OPTION));
      }
      break;
    }
  }
  BmFreeKeyOptions (KeyOptions, KeyOptionCount);

  EfiReleaseLock (&mBmHotkeyLock);

  return Status;
}
