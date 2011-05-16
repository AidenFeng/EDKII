/** @file
  Main file for NULL named library for level 3 shell command functions.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "UefiShellLevel3CommandsLib.h"

CONST CHAR16 gShellLevel3FileName[] = L"ShellCommands";
EFI_HANDLE gShellLevel3HiiHandle = NULL;
STATIC CONST EFI_GUID gShellLevel3HiiGuid = \
  { \
    0x4344558d, 0x4ef9, 0x4725, { 0xb1, 0xe4, 0x33, 0x76, 0xe8, 0xd6, 0x97, 0x4f } \
  };

/**
  return the filename to get help from is not using HII.

  @retval The filename.
**/
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameLevel3 (
  VOID
  )
{
  return (gShellLevel3FileName);
}

/**
  Constructor for the Shell Level 3 Commands library.

  Install the handlers for level 3 UEFI Shell 2.0 commands.

  @param ImageHandle    the image handle of the process
  @param SystemTable    the EFI System Table pointer

  @retval EFI_SUCCESS        the shell command handlers were installed sucessfully
  @retval EFI_UNSUPPORTED    the shell level required was not found.
**/
EFI_STATUS
EFIAPI
ShellLevel3CommandsLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  gShellLevel3HiiHandle = NULL;
  //
  // if shell level is less than 3 do nothing
  //
  if (PcdGet8(PcdShellSupportLevel) < 3) {
    return (EFI_SUCCESS);
  }

  gShellLevel3HiiHandle = HiiAddPackages (&gShellLevel3HiiGuid, gImageHandle, UefiShellLevel3CommandsLibStrings, NULL);
  if (gShellLevel3HiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }
  //
  // install our shell command handlers that are always installed
  //
  // Note: that Time, Timezone, and Date are part of level 2 library
  //
  ShellCommandRegisterCommandName(L"type",    ShellCommandRunType   , ShellCommandGetManFileNameLevel3, 3, L"", TRUE , gShellLevel3HiiHandle, STRING_TOKEN(STR_GET_HELP_TYPE));
  ShellCommandRegisterCommandName(L"touch",   ShellCommandRunTouch  , ShellCommandGetManFileNameLevel3, 3, L"", TRUE , gShellLevel3HiiHandle, STRING_TOKEN(STR_GET_HELP_TOUCH));
  ShellCommandRegisterCommandName(L"ver",     ShellCommandRunVer    , ShellCommandGetManFileNameLevel3, 3, L"", TRUE , gShellLevel3HiiHandle, STRING_TOKEN(STR_GET_HELP_VER));
  ShellCommandRegisterCommandName(L"alias",   ShellCommandRunAlias  , ShellCommandGetManFileNameLevel3, 3, L"", TRUE , gShellLevel3HiiHandle, STRING_TOKEN(STR_GET_HELP_ALIAS));
  ShellCommandRegisterCommandName(L"cls",     ShellCommandRunCls    , ShellCommandGetManFileNameLevel3, 3, L"", TRUE , gShellLevel3HiiHandle, STRING_TOKEN(STR_GET_HELP_CLS));
  ShellCommandRegisterCommandName(L"echo",    ShellCommandRunEcho   , ShellCommandGetManFileNameLevel3, 3, L"", FALSE, gShellLevel3HiiHandle, STRING_TOKEN(STR_GET_HELP_ECHO));
  ShellCommandRegisterCommandName(L"pause",   ShellCommandRunPause  , ShellCommandGetManFileNameLevel3, 3, L"", TRUE , gShellLevel3HiiHandle, STRING_TOKEN(STR_GET_HELP_PAUSE));
  ShellCommandRegisterCommandName(L"getmtc",  ShellCommandRunGetMtc , ShellCommandGetManFileNameLevel3, 3, L"", TRUE , gShellLevel3HiiHandle, STRING_TOKEN(STR_GET_HELP_GETMTC));
  ShellCommandRegisterCommandName(L"help",    ShellCommandRunHelp   , ShellCommandGetManFileNameLevel3, 3, L"", TRUE , gShellLevel3HiiHandle, STRING_TOKEN(STR_GET_HELP_HELP));

  return (EFI_SUCCESS);
}

/**
  Destructor for the library.  free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
ShellLevel3CommandsLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gShellLevel3HiiHandle != NULL) {
    HiiRemovePackages(gShellLevel3HiiHandle);
  }
  return (EFI_SUCCESS);
}
