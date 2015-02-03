/** @file
  Tools of clarify the content of the smbios table.

  Copyright (c) 2015, Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "../UefiShellDebug1CommandsLib.h"
#include "LibSmbiosView.h"
#include "SmbiosView.h"
#include "PrintInfo.h"
#include "QueryTable.h"

UINT8                       gShowType         = SHOW_DETAIL;
STATIC STRUCTURE_STATISTICS *mStatisticsTable = NULL;

UINT8  SmbiosMajorVersion;
UINT8  SmbiosMinorVersion;

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-t", TypeValue},
  {L"-h", TypeValue},
  {L"-s", TypeFlag},
  {L"-a", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'smbiosview' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunSmbiosView (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT8               StructType;
  UINT16              StructHandle;
  EFI_STATUS          Status;
  BOOLEAN             RandomView;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  CONST CHAR16        *Temp;

  mStatisticsTable    = NULL;
  Package             = NULL;
  ShellStatus         = SHELL_SUCCESS;

  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"smbiosview", ProblemParam);  
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 1) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"smbiosview");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag(Package, L"-t") && ShellCommandLineGetValue(Package, L"-t") == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle, L"smbiosview", L"-t");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag(Package, L"-h") && ShellCommandLineGetValue(Package, L"-h") == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle,  L"smbiosview", L"-h");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (
        (ShellCommandLineGetFlag(Package, L"-t") && ShellCommandLineGetFlag(Package, L"-h")) ||
        (ShellCommandLineGetFlag(Package, L"-t") && ShellCommandLineGetFlag(Package, L"-s")) ||
        (ShellCommandLineGetFlag(Package, L"-t") && ShellCommandLineGetFlag(Package, L"-a")) ||
        (ShellCommandLineGetFlag(Package, L"-h") && ShellCommandLineGetFlag(Package, L"-s")) ||
        (ShellCommandLineGetFlag(Package, L"-h") && ShellCommandLineGetFlag(Package, L"-a")) ||
        (ShellCommandLineGetFlag(Package, L"-s") && ShellCommandLineGetFlag(Package, L"-a"))
      ) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"smbiosview");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {

      //
      // Init Lib
      
      Status = LibSmbiosInit ();
      if (EFI_ERROR (Status)) {
        ShellStatus = SHELL_NOT_FOUND;
        goto Done;
      }
      //
      // build statistics table
      //
      Status = InitSmbiosTableStatistics ();
      if (EFI_ERROR (Status)) {
        ShellStatus = SHELL_NOT_FOUND;
        goto Done;
      }

      StructType    = STRUCTURE_TYPE_RANDOM;
      RandomView    = TRUE;
      //
      // Initialize the StructHandle to be the first handle
      //
      StructHandle  = INVALID_HANDLE;
      LibGetSmbiosStructure (&StructHandle, NULL, NULL);

      Temp          = ShellCommandLineGetValue(Package, L"-t");
      if (Temp != NULL) {
        StructType = (UINT8) ShellStrToUintn (Temp);
      }

      Temp = ShellCommandLineGetValue(Package, L"-h");
      if (Temp != NULL) {
        RandomView   = FALSE;
        StructHandle = (UINT16) ShellStrToUintn(Temp);
      }

      if (ShellCommandLineGetFlag(Package, L"-s")) {
        Status = DisplayStatisticsTable (SHOW_DETAIL);
        if (EFI_ERROR(Status)) {
          ShellStatus = SHELL_NOT_FOUND;
        }
        goto Done;
      }

      if (ShellCommandLineGetFlag(Package, L"-a")) {
        gShowType = SHOW_ALL;
      }
      //
      // Show SMBIOS structure information
      //
      Status = SMBiosView (StructType, StructHandle, gShowType, RandomView);
      if (EFI_ERROR(Status)) {
        ShellStatus = SHELL_NOT_FOUND;
      }
    }
  }
Done:
  //
  // Release resources
  //
  if (mStatisticsTable != NULL) {
    //
    // Release statistics table
    //
    FreePool (mStatisticsTable);
    mStatisticsTable = NULL;
  }

  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }

  LibSmbiosCleanup ();

  return ShellStatus;
}

/**
  Query all structures Data from SMBIOS table and Display
  the information to users as required display option.

  @param[in] QueryType      Structure type to view.
  @param[in] QueryHandle    Structure handle to view.
  @param[in] Option         Display option: none,outline,normal,detail.
  @param[in] RandomView     Support for -h parameter.

  @retval EFI_SUCCESS           print is successful.
  @retval EFI_BAD_BUFFER_SIZE   structure is out of the range of SMBIOS table.
**/
EFI_STATUS
EFIAPI
SMBiosView (
  IN  UINT8   QueryType,
  IN  UINT16  QueryHandle,
  IN  UINT8   Option,
  IN  BOOLEAN RandomView
  )
{
  UINT16                    Handle;
  UINT8                     *Buffer;
  UINT16                    Length;
  UINTN                     Index;

  SMBIOS_STRUCTURE_POINTER  SmbiosStruct;
  SMBIOS_TABLE_ENTRY_POINT  *SMBiosTable;

  SMBiosTable = NULL;
  LibSmbiosGetEPS (&SMBiosTable);
  if (SMBiosTable == NULL) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_CANNOT_ACCESS_TABLE), gShellDebug1HiiHandle);
    return EFI_BAD_BUFFER_SIZE;
  }

  if (CompareMem (SMBiosTable->AnchorString, "_SM_", 4) == 0) {
    //
    // Have got SMBIOS table
    //
    SmbiosPrintEPSInfo (SMBiosTable, Option);

    SmbiosMajorVersion = SMBiosTable->MajorVersion;
    SmbiosMinorVersion = SMBiosTable->MinorVersion;

    ShellPrintEx(-1,-1,L"=========================================================\n");
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_QUERY_STRUCT_COND), gShellDebug1HiiHandle);

    if (QueryType == STRUCTURE_TYPE_RANDOM) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_QUERYTYPE_RANDOM), gShellDebug1HiiHandle);
    } else {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_QUERYTYPE), gShellDebug1HiiHandle, QueryType);
    }

    if (RandomView) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_QUERYHANDLE_RANDOM), gShellDebug1HiiHandle);
    } else {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_QUERYHANDLE), gShellDebug1HiiHandle, QueryHandle);
    }

    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_SHOWTYPE), gShellDebug1HiiHandle);
    ShellPrintEx(-1,-1,GetShowTypeString (gShowType));
    ShellPrintEx(-1,-1,L"\n\n");

/*
    //
    // Get internal commands, such as change options.
    //
    Status = WaitEnter ();
    if (EFI_ERROR (Status)) {
      if (Status == EFI_ABORTED) {
        return EFI_SUCCESS;
      }

      return Status;
    }
*/

    //
    // Searching and display structure info
    //
    Handle    = QueryHandle;
    for (Index = 0; Index < SMBiosTable->NumberOfSmbiosStructures; Index++) {
      //
      // if reach the end of table, break..
      //
      if (Handle == INVALID_HANDLE) {
        break;
      }
      //
      // handle then point to the next!
      //
      if (LibGetSmbiosStructure (&Handle, &Buffer, &Length) != DMI_SUCCESS) {
        break;
      }

      SmbiosStruct.Raw = Buffer;

      //
      // if QueryType==Random, print this structure.
      // if QueryType!=Random, but Hdr->Type==QueryType, also print it.
      // only if QueryType != Random and Hdr->Type != QueryType, skiped it.
      //
      if (QueryType != STRUCTURE_TYPE_RANDOM && SmbiosStruct.Hdr->Type != QueryType) {
        continue;
      }

      ShellPrintEx(-1,-1,L"\n=========================================================\n");
      ShellPrintHiiEx(-1,-1,NULL,
        STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_TYPE_HANDLE_DUMP_STRUCT),
        gShellDebug1HiiHandle,
        SmbiosStruct.Hdr->Type,
        SmbiosStruct.Hdr->Handle
       );
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_INDEX_LENGTH), gShellDebug1HiiHandle, Index, Length);
      //
      // Addr of structure in structure in table
      //
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_ADDR), gShellDebug1HiiHandle, (UINTN) Buffer);
      DumpHex (0, 0, Length, Buffer);

/*
      //
      // Get internal commands, such as change options.
      //
      Status = WaitEnter ();
      if (EFI_ERROR (Status)) {
        if (Status == EFI_ABORTED) {
          return EFI_SUCCESS;
        }

        return Status;
      }
*/

      if (gShowType != SHOW_NONE) {
        //
        // Print structure information
        //
        SmbiosPrintStructure (&SmbiosStruct, gShowType);
        ShellPrintEx(-1,-1,L"\n");

/*
        //
        // Get internal commands, such as change options.
        //
        Status = WaitEnter ();
        if (EFI_ERROR (Status)) {
          if (Status == EFI_ABORTED) {
            return EFI_SUCCESS;
          }

          return Status;
        }
*/
      }
      if (!RandomView) {
        break;
      }
      //
      // Support Execution Interrupt.
      //
      if (ShellGetExecutionBreakFlag ()) {
        return EFI_ABORTED;
      }
    }

    ShellPrintEx(-1,-1,L"\n=========================================================\n");
    return EFI_SUCCESS;
  }

  return EFI_BAD_BUFFER_SIZE;
}

/**
  Function to initialize the global mStatisticsTable object.

  @retval EFI_SUCCESS           print is successful.
**/
EFI_STATUS
EFIAPI
InitSmbiosTableStatistics (
  VOID
  )
{
  UINT16                    Handle;
  UINT8                     *Buffer;
  UINT16                    Length;
  UINT16                    Offset;
  UINT16                    Index;

  SMBIOS_STRUCTURE_POINTER  SmbiosStruct;
  SMBIOS_TABLE_ENTRY_POINT  *SMBiosTable;
  STRUCTURE_STATISTICS      *StatisticsPointer;

  SMBiosTable = NULL;
  LibSmbiosGetEPS (&SMBiosTable);
  if (SMBiosTable == NULL) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_CANNOT_ACCESS_TABLE), gShellDebug1HiiHandle);
    return EFI_NOT_FOUND;
  }

  if (CompareMem (SMBiosTable->AnchorString, "_SM_", 4) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_SMBIOS_TABLE), gShellDebug1HiiHandle);
    return EFI_INVALID_PARAMETER;
  }
  //
  // Allocate memory to mStatisticsTable
  //
  if (mStatisticsTable != NULL) {
    FreePool (mStatisticsTable);
    mStatisticsTable = NULL;
  }

  mStatisticsTable = (STRUCTURE_STATISTICS *) AllocateZeroPool (SMBiosTable->NumberOfSmbiosStructures * sizeof (STRUCTURE_STATISTICS));

  if (mStatisticsTable == NULL) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_OUT_OF_MEM), gShellDebug1HiiHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  Offset      = 0;
  StatisticsPointer = mStatisticsTable;

  //
  // search from the first one
  //
  Handle = INVALID_HANDLE;
  LibGetSmbiosStructure (&Handle, NULL, NULL);
  for (Index = 1; Index <= SMBiosTable->NumberOfSmbiosStructures; Index++) {
    //
    // If reach the end of table, break..
    //
    if (Handle == INVALID_HANDLE) {
      break;
    }
    //
    // After LibGetSmbiosStructure(), handle then point to the next!
    //
    if (LibGetSmbiosStructure (&Handle, &Buffer, &Length) != DMI_SUCCESS) {
      break;
    }

    SmbiosStruct.Raw = Buffer;

    //
    // general statistics
    //
    StatisticsPointer->Index  = Index;
    StatisticsPointer->Type   = SmbiosStruct.Hdr->Type;
    StatisticsPointer->Handle = SmbiosStruct.Hdr->Handle;
    StatisticsPointer->Length = Length;
    StatisticsPointer->Addr   = Offset;

    Offset = (UINT16) (Offset + Length);

    StatisticsPointer         = &mStatisticsTable[Index];
  }

  return EFI_SUCCESS;
}

/**
  Function to display the global mStatisticsTable object.

  @param[in] Option             ECHO, NORMAL, or DETAIL control the amount of detail displayed.

  @retval EFI_SUCCESS           print is successful.
**/
EFI_STATUS
EFIAPI
DisplayStatisticsTable (
  IN   UINT8   Option
  )
{
  UINTN                    Index;
  UINTN                    Num;
  STRUCTURE_STATISTICS     *StatisticsPointer;
  SMBIOS_TABLE_ENTRY_POINT *SMBiosTable;

  SMBiosTable = NULL;
  if (Option < SHOW_OUTLINE) {
    return EFI_SUCCESS;
  }
  //
  // display EPS information firstly
  //
  LibSmbiosGetEPS (&SMBiosTable);
  if (SMBiosTable == NULL) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_CANNOT_ACCESS_TABLE), gShellDebug1HiiHandle);
    return EFI_UNSUPPORTED;
  }

  ShellPrintEx(-1,-1,L"\n============================================================\n");
  SmbiosPrintEPSInfo (SMBiosTable, Option);

  if (Option < SHOW_NORMAL) {
    return EFI_SUCCESS;
  }

  if (mStatisticsTable == NULL) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_CANNOT_ACCESS_STATS), gShellDebug1HiiHandle);
    return EFI_NOT_FOUND;
  }

  ShellPrintEx(-1,-1,L"============================================================\n");
  StatisticsPointer = &mStatisticsTable[0];
  Num         = SMBiosTable->NumberOfSmbiosStructures;
  //
  // display statistics table content
  //
  for (Index = 1; Index <= Num; Index++) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_INDEX), gShellDebug1HiiHandle, StatisticsPointer->Index);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_TYPE), gShellDebug1HiiHandle, StatisticsPointer->Type);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_HANDLE), gShellDebug1HiiHandle, StatisticsPointer->Handle);
    if (Option >= SHOW_DETAIL) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_OFFSET), gShellDebug1HiiHandle, StatisticsPointer->Addr);
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_LENGTH), gShellDebug1HiiHandle, StatisticsPointer->Length);
    }

    ShellPrintEx(-1,-1,L"\n");
    StatisticsPointer = &mStatisticsTable[Index];
/*
    //
    // Display 20 lines and wait for a page break
    //
    if (Index % 20 == 0) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_ENTER_CONTINUE), gShellDebug1HiiHandle);
      Status = WaitEnter ();
      if (EFI_ERROR (Status)) {
        if (Status == EFI_ABORTED) {
          return EFI_SUCCESS;
        }

        return Status;
      }
    }
*/
  }

  return EFI_SUCCESS;
}

/**
  function to return a string of the detail level.

  @param[in] ShowType         The detail level whose name is desired in clear text.

  @return   A pointer to a string representing the ShowType (or 'undefined type' if not known).
**/
CHAR16 *
EFIAPI
GetShowTypeString (
  UINT8 ShowType
  )
{
  //
  // show type
  //
  switch (ShowType) {

  case SHOW_NONE:
    return L"SHOW_NONE";

  case SHOW_OUTLINE:
    return L"SHOW_OUTLINE";

  case SHOW_NORMAL:
    return L"SHOW_NORMAL";

  case SHOW_DETAIL:
    return L"SHOW_DETAIL";

  case SHOW_ALL:
    return L"SHOW_ALL";

  default:
    return L"Undefined type";
  }
}

