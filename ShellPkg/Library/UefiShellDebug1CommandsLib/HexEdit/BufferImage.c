/** @file
  Defines HBufferImage - the view of the file that is visible at any point,
  as well as the event handlers for editing the file
  
  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HexEditor.h"

extern EFI_HANDLE                 HImageHandleBackup;

extern HEFI_EDITOR_FILE_IMAGE     HFileImage;
extern HEFI_EDITOR_DISK_IMAGE     HDiskImage;
extern HEFI_EDITOR_MEM_IMAGE      HMemImage;

extern HEFI_EDITOR_FILE_IMAGE     HFileImageBackupVar;
extern HEFI_EDITOR_DISK_IMAGE     HDiskImageBackupVar;
extern HEFI_EDITOR_MEM_IMAGE      HMemImageBackupVar;

extern BOOLEAN                    HEditorMouseAction;

extern HEFI_EDITOR_GLOBAL_EDITOR  HMainEditor;
extern HEFI_EDITOR_GLOBAL_EDITOR  HMainEditorBackupVar;

HEFI_EDITOR_BUFFER_IMAGE          HBufferImage;
HEFI_EDITOR_BUFFER_IMAGE          HBufferImageBackupVar;

//
// for basic initialization of HBufferImage
//
HEFI_EDITOR_BUFFER_IMAGE          HBufferImageConst = {
  NULL,
  NULL,
  0,
  NULL,
  {
    0,
    0
  },
  {
    0,
    0
  },
  {
    0,
    0
  },
  0,
  TRUE,
  FALSE,
  FileTypeNone,
  NULL,
  NULL,
  NULL
};

//
// the whole edit area needs to be refreshed
//
BOOLEAN                           HBufferImageNeedRefresh;

//
// only the current line in edit area needs to be refresh
//
BOOLEAN                           HBufferImageOnlyLineNeedRefresh;

BOOLEAN                           HBufferImageMouseNeedRefresh;

EFI_STATUS
HBufferImageInit (
  VOID
  )
/*++

Routine Description: 

  Initialization function for HBufferImage

Arguments:  

  None

Returns:  

  EFI_SUCCESS
  EFI_LOAD_ERROR

--*/
{
  EFI_STATUS  Status;

  //
  // basically initialize the HBufferImage
  //
  CopyMem (&HBufferImage, &HBufferImageConst, sizeof (HBufferImage));

  //
  // INIT listhead
  //
  HBufferImage.ListHead = AllocateZeroPool (sizeof (LIST_ENTRY));
  if (HBufferImage.ListHead == NULL) {
    return EFI_LOAD_ERROR;
  }

  InitializeListHead (HBufferImage.ListHead);

  HBufferImage.DisplayPosition.Row    = 2;
  HBufferImage.DisplayPosition.Column = 10;
  HBufferImage.MousePosition.Row      = 2;
  HBufferImage.MousePosition.Column   = 10;

  HBufferImage.FileImage              = &HFileImage;
  HBufferImage.DiskImage              = &HDiskImage;
  HBufferImage.MemImage               = &HMemImage;

  HBufferImageNeedRefresh             = FALSE;
  HBufferImageOnlyLineNeedRefresh     = FALSE;
  HBufferImageMouseNeedRefresh        = FALSE;

  HBufferImageBackupVar.FileImage     = &HFileImageBackupVar;
  HBufferImageBackupVar.DiskImage     = &HDiskImageBackupVar;
  HBufferImageBackupVar.MemImage      = &HMemImageBackupVar;

  Status = HFileImageInit ();
  if (EFI_ERROR (Status)) {
    return EFI_LOAD_ERROR;
  }

  Status = HDiskImageInit ();
  if (EFI_ERROR (Status)) {
    return EFI_LOAD_ERROR;
  }

  Status = HMemImageInit ();
  if (EFI_ERROR (Status)) {
    return EFI_LOAD_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageBackup (
  VOID
  )
/*++

Routine Description: 

  Backup function for HBufferImage
  Only a few fields need to be backup. 
  This is for making the file buffer refresh 
  as few as possible.

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{
  HBufferImageBackupVar.MousePosition   = HBufferImage.MousePosition;

  HBufferImageBackupVar.BufferPosition  = HBufferImage.BufferPosition;

  HBufferImageBackupVar.Modified        = HBufferImage.Modified;

  HBufferImageBackupVar.BufferType      = HBufferImage.BufferType;
  HBufferImageBackupVar.LowVisibleRow   = HBufferImage.LowVisibleRow;
  HBufferImageBackupVar.HighBits        = HBufferImage.HighBits;

  //
  // three kinds of buffer supported
  //   file buffer
  //   disk buffer
  //   memory buffer
  //
  switch (HBufferImage.BufferType) {
  case FileTypeFileBuffer:
    HFileImageBackup ();
    break;

  case FileTypeDiskBuffer:
    HDiskImageBackup ();
    break;

  case FileTypeMemBuffer:
    HMemImageBackup ();
    break;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageFreeLines (
  VOID
  )
/*++

Routine Description: 

  Free all the lines in HBufferImage
    Fields affected:
    Lines
    CurrentLine
    NumLines
    ListHead 

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{
  HFreeLines (HBufferImage.ListHead, HBufferImage.Lines);

  HBufferImage.Lines        = NULL;
  HBufferImage.CurrentLine  = NULL;
  HBufferImage.NumLines     = 0;

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageCleanup (
  VOID
  )
/*++

Routine Description: 

  Cleanup function for HBufferImage

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{
  EFI_STATUS  Status;

  //
  // free all the lines
  //
  Status = HBufferImageFreeLines ();

  SHELL_FREE_NON_NULL (HBufferImage.ListHead);
  HBufferImage.ListHead = NULL;

  HFileImageCleanup ();
  HDiskImageCleanup ();
  HMemImageCleanup ();

  return Status;

}

EFI_STATUS
HBufferImagePrintLine (
  IN HEFI_EDITOR_LINE           *Line,
  IN UINTN                      Row,
  IN UINTN                      FRow,
  IN HEFI_EDITOR_COLOR_UNION    Orig,
  IN HEFI_EDITOR_COLOR_UNION    New

  )
/*++

Routine Description: 

  Print Line on Row

Arguments:  

  Line - Line to print
  Row  - Row on screen ( begin from 1 )
  FRow - FRow
  Orig - Orig
  New  - Light display

Returns:  

  EFI_SUCCESS

--*/
{

  UINTN   Index;
  UINTN   Pos;
  BOOLEAN Selected;
  BOOLEAN BeNewColor;
  UINTN   RowStart;
  UINTN   RowEnd;
  UINTN   ColStart;
  UINTN   ColEnd;

  //
  // variable initialization
  //
  ColStart  = 0;
  ColEnd    = 0;
  Selected  = FALSE;

  //
  // print the selected area in opposite color
  //
  if (HMainEditor.SelectStart != 0 && HMainEditor.SelectEnd != 0) {
    RowStart  = (HMainEditor.SelectStart - 1) / 0x10 + 1;
    RowEnd    = (HMainEditor.SelectEnd - 1) / 0x10 + 1;

    ColStart  = (HMainEditor.SelectStart - 1) % 0x10 + 1;
    ColEnd    = (HMainEditor.SelectEnd - 1) % 0x10 + 1;

    if (FRow >= RowStart && FRow <= RowEnd) {
      Selected = TRUE;
    }

    if (FRow > RowStart) {
      ColStart = 1;
    }

    if (FRow < RowEnd) {
      ColEnd = 0x10;
    }

  }

  if (HEditorMouseAction == FALSE) {
    ShellPrintEx (
      0,
      (INT32)Row - 1,
      L"%8X ",
      ((INT32)Row - 2 + HBufferImage.LowVisibleRow - 1) * 0x10
      );

  }

  for (Index = 0; Index < 0x08 && Index < Line->Size; Index++) {

    BeNewColor = FALSE;

    if (Selected) {
      if (Index + 1 >= ColStart && Index + 1 <= ColEnd) {
        BeNewColor = TRUE;
      }
    }

    if (BeNewColor) {
      gST->ConOut->SetAttribute (gST->ConOut, New.Data);
    } else {
      gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);
    }

    Pos = 10 + (Index * 3);
    if (Line->Buffer[Index] < 0x10) {
      ShellPrintEx ((INT32)Pos - 1, (INT32)Row - 1, L"0");
      Pos++;
    }

    if (Index < 0x07) {
      ShellPrintEx ((INT32)Pos - 1, (INT32)Row - 1, L"%x ", Line->Buffer[Index]);
    } else {
      ShellPrintEx ((INT32)Pos - 1, (INT32)Row - 1, L"%x  ", Line->Buffer[Index]);
    }

  }

  gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);
  while (Index < 0x08) {
    Pos = 10 + (Index * 3);
    ShellPrintEx ((INT32)Pos - 1, (INT32)Row - 1, L"    ");
    Index++;
  }

  while (Index < 0x10 && Index < Line->Size) {

    BeNewColor = FALSE;

    if (Selected) {
      if (Index + 1 >= ColStart && Index + 1 <= ColEnd) {
        BeNewColor = TRUE;
      }
    }

    if (BeNewColor) {
      gST->ConOut->SetAttribute (gST->ConOut, New.Data);
    } else {
      gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);
    }

    Pos = 10 + (Index * 3) + 1;
    if (Line->Buffer[Index] < 0x10) {
      ShellPrintEx ((INT32)Pos - 1, (INT32)Row - 1, L"0");
      Pos++;
    }

    ShellPrintEx ((INT32)Pos - 1, (INT32)Row - 1, L"%x ", Line->Buffer[Index]);
    Index++;
  }

  gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);
  while (Index < 0x10) {
    Pos = 10 + (Index * 3) + 1;
    ShellPrintEx ((INT32)Pos - 1, (INT32)Row - 1, L"   ");
    Index++;
  }
  //
  // restore the original color
  //
  gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);

  //
  // PRINT the buffer content
  //
  if (HEditorMouseAction == FALSE) {
    for (Index = 0; Index < 0x10 && Index < Line->Size; Index++) {
      Pos = ASCII_POSITION + Index;

      //
      // learned from shelle.h -- IsValidChar
      //
      if (Line->Buffer[Index] >= L' ') {
        ShellPrintEx ((INT32)Pos - 1, (INT32)Row - 1, L"%c", (CHAR16) Line->Buffer[Index]);
      } else {
        ShellPrintEx ((INT32)Pos - 1, (INT32)Row - 1, L"%c", '.');
      }
    }

    while (Index < 0x10) {
      Pos = ASCII_POSITION + Index;
      ShellPrintEx ((INT32)Pos - 1, (INT32)Row - 1, L" ");
      Index++;
    }
  }
  //
  // restore the abundant blank in hex edit area to original color
  //
  if (Selected) {
    if (ColEnd <= 7) {
      Pos = 10 + (ColEnd - 1) * 3 + 2;
      ShellPrintEx ((INT32)Pos - 1, (INT32)Row - 1, L" ");
    } else if (ColEnd == 8) {
      Pos = 10 + (ColEnd - 1) * 3 + 2;
      ShellPrintEx ((INT32)Pos - 1, (INT32)Row - 1, L"  ");
    } else {
      Pos = 10 + (ColEnd - 1) * 3 + 3;
      ShellPrintEx ((INT32)Pos - 1, (INT32)Row - 1, L" ");
    }
  }

  return EFI_SUCCESS;
}

BOOLEAN
HBufferImageIsAtHighBits (
  IN  UINTN Column,
  OUT UINTN *FCol
  )
{
  Column -= 10;

  //
  // NOW AFTER THE SUB, Column start from 0
  // 23 AND 24 ARE BOTH BLANK
  //
  if (Column == 24) {
    *FCol = 0;
    return FALSE;
  }

  if (Column > 24) {
    Column--;
  }

  *FCol = (Column / 3) + 1;

  if (!(Column % 3)) {
    return TRUE;
  }

  if ((Column % 3 == 2)) {
    *FCol = 0;
  }

  return FALSE;
}

BOOLEAN
HBufferImageIsInSelectedArea (
  IN UINTN MouseRow,
  IN UINTN MouseCol
  )
{
  UINTN FRow;
  UINTN RowStart;
  UINTN RowEnd;
  UINTN ColStart;
  UINTN ColEnd;
  UINTN MouseColStart;
  UINTN MouseColEnd;

  //
  // judge mouse position whether is in selected area
  //
  //
  // not select
  //
  if (HMainEditor.SelectStart == 0 || HMainEditor.SelectEnd == 0) {
    return FALSE;
  }
  //
  // calculate the select area
  //
  RowStart  = (HMainEditor.SelectStart - 1) / 0x10 + 1;
  RowEnd    = (HMainEditor.SelectEnd - 1) / 0x10 + 1;

  ColStart  = (HMainEditor.SelectStart - 1) % 0x10 + 1;
  ColEnd    = (HMainEditor.SelectEnd - 1) % 0x10 + 1;

  FRow      = HBufferImage.LowVisibleRow + MouseRow - 2;
  if (FRow < RowStart || FRow > RowEnd) {
    return FALSE;
  }

  if (FRow > RowStart) {
    ColStart = 1;
  }

  if (FRow < RowEnd) {
    ColEnd = 0x10;
  }

  MouseColStart = 10 + (ColStart - 1) * 3;
  if (ColStart > 8) {
    MouseColStart++;
  }

  MouseColEnd = 10 + (ColEnd - 1) * 3 + 1;
  if (ColEnd > 8) {
    MouseColEnd++;
  }

  if (MouseCol < MouseColStart || MouseCol > MouseColEnd) {
    return FALSE;
  }

  return TRUE;
}

EFI_STATUS
HBufferImageRestoreMousePosition (
  VOID
  )
{
  HEFI_EDITOR_COLOR_UNION Orig;
  HEFI_EDITOR_COLOR_UNION New;
  UINTN                   FRow;
  UINTN                   FColumn;
  BOOLEAN                 HasCharacter;
  HEFI_EDITOR_LINE        *CurrentLine;
  HEFI_EDITOR_LINE        *Line;
  UINT8                   Value;
  BOOLEAN                 HighBits;

  Line = NULL;
  if (HMainEditor.MouseSupported) {

    if (HBufferImageMouseNeedRefresh) {

      HBufferImageMouseNeedRefresh = FALSE;

      //
      // if mouse position not moved and only mouse action
      // so do not need to refresh mouse position
      //
      if ((
            HBufferImage.MousePosition.Row == HBufferImageBackupVar.MousePosition.Row &&
          HBufferImage.MousePosition.Column == HBufferImageBackupVar.MousePosition.Column
        ) &&
          HEditorMouseAction
          ) {
        return EFI_SUCCESS;
      }
      //
      // backup the old screen attributes
      //
      Orig                  = HMainEditor.ColorAttributes;
      New.Colors.Foreground = Orig.Colors.Background;
      New.Colors.Background = Orig.Colors.Foreground;

      //
      // if in selected area,
      // so do not need to refresh mouse
      //
      if (!HBufferImageIsInSelectedArea (
            HBufferImageBackupVar.MousePosition.Row,
            HBufferImageBackupVar.MousePosition.Column
            )) {
        gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);
      } else {
        gST->ConOut->SetAttribute (gST->ConOut, New.Data);
      }
      //
      // clear the old mouse position
      //
      FRow = HBufferImage.LowVisibleRow + HBufferImageBackupVar.MousePosition.Row - 2;

      HighBits = HBufferImageIsAtHighBits (
                  HBufferImageBackupVar.MousePosition.Column,
                  &FColumn
                  );

      HasCharacter = TRUE;
      if (FRow > HBufferImage.NumLines || FColumn == 0) {
        HasCharacter = FALSE;
      } else {
        CurrentLine = HBufferImage.CurrentLine;
        Line        = HMoveLine (FRow - HBufferImage.BufferPosition.Row);

        if (Line == NULL || FColumn > Line->Size) {
          HasCharacter = FALSE;
        }

        HBufferImage.CurrentLine = CurrentLine;
      }

      ShellPrintEx (
        (INT32)HBufferImageBackupVar.MousePosition.Column - 1,
        (INT32)HBufferImageBackupVar.MousePosition.Row - 1,
        L" "
        );

      if (HasCharacter) {
        if (HighBits) {
          Value = (UINT8) (Line->Buffer[FColumn - 1] & 0xf0);
          Value = (UINT8) (Value >> 4);
        } else {
          Value = (UINT8) (Line->Buffer[FColumn - 1] & 0xf);
        }

        ShellPrintEx (
          (INT32)HBufferImageBackupVar.MousePosition.Column - 1,
          (INT32)HBufferImageBackupVar.MousePosition.Row - 1,
          L"%x",
          Value
          );
      }

      if (!HBufferImageIsInSelectedArea (
            HBufferImage.MousePosition.Row,
            HBufferImage.MousePosition.Column
            )) {
        gST->ConOut->SetAttribute (gST->ConOut, New.Data);
      } else {
        gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);
      }
      //
      // clear the old mouse position
      //
      FRow = HBufferImage.LowVisibleRow + HBufferImage.MousePosition.Row - 2;

      HighBits = HBufferImageIsAtHighBits (
                  HBufferImage.MousePosition.Column,
                  &FColumn
                  );

      HasCharacter = TRUE;
      if (FRow > HBufferImage.NumLines || FColumn == 0) {
        HasCharacter = FALSE;
      } else {
        CurrentLine = HBufferImage.CurrentLine;
        Line        = HMoveLine (FRow - HBufferImage.BufferPosition.Row);

        if (FColumn > Line->Size) {
          HasCharacter = FALSE;
        }

        HBufferImage.CurrentLine = CurrentLine;
      }

      ShellPrintEx (
        (INT32)HBufferImage.MousePosition.Column - 1,
        (INT32)HBufferImage.MousePosition.Row - 1,
        L" "
        );

      if (HasCharacter) {
        if (HighBits) {
          Value = (UINT8) (Line->Buffer[FColumn - 1] & 0xf0);
          Value = (UINT8) (Value >> 4);
        } else {
          Value = (UINT8) (Line->Buffer[FColumn - 1] & 0xf);
        }

        ShellPrintEx (
          (INT32)HBufferImage.MousePosition.Column - 1,
          (INT32)HBufferImage.MousePosition.Row - 1,
          L"%x",
          Value
          );
      }
      //
      // end of HasCharacter
      //
      gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);
    }
    //
    // end of MouseNeedRefresh
    //
  }
  //
  // end of MouseSupported
  //
  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageRestorePosition (
  VOID
  )
/*++

Routine Description: 

  Set cursor position according to HBufferImage.DisplayPosition.

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{
  //
  // set cursor position
  //
  gST->ConOut->SetCursorPosition (
        gST->ConOut,
        HBufferImage.DisplayPosition.Column - 1,
        HBufferImage.DisplayPosition.Row - 1
        );

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageRefresh (
  VOID
  )
/*++

Routine Description: 

  Refresh function for HBufferImage

Arguments:  

  None

Returns:  

  EFI_SUCCESS
  EFI_LOAD_ERROR 

--*/
{
  LIST_ENTRY          *Link;
  HEFI_EDITOR_LINE        *Line;
  UINTN                   Row;
  HEFI_EDITOR_COLOR_UNION Orig;
  HEFI_EDITOR_COLOR_UNION New;

  UINTN                   StartRow;
  UINTN                   EndRow;
  UINTN                   FStartRow;
  UINTN                   FEndRow;
  UINTN                   Tmp;

  Orig                  = HMainEditor.ColorAttributes;
  New.Colors.Foreground = Orig.Colors.Background;
  New.Colors.Background = Orig.Colors.Foreground;

  //
  // if it's the first time after editor launch, so should refresh
  //
  if (HEditorFirst == FALSE) {
    //
    // no definite required refresh
    // and file position displayed on screen has not been changed
    //
    if (HBufferImageNeedRefresh == FALSE &&
        HBufferImageOnlyLineNeedRefresh == FALSE &&
        HBufferImageBackupVar.LowVisibleRow == HBufferImage.LowVisibleRow
        ) {
      HBufferImageRestoreMousePosition ();
      HBufferImageRestorePosition ();
      return EFI_SUCCESS;
    }
  }

  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  //
  // only need to refresh current line
  //
  if (HBufferImageOnlyLineNeedRefresh == TRUE && HBufferImageBackupVar.LowVisibleRow == HBufferImage.LowVisibleRow) {

    HBufferImagePrintLine (
      HBufferImage.CurrentLine,
      HBufferImage.DisplayPosition.Row,
      HBufferImage.BufferPosition.Row,
      Orig,
      New
      );
  } else {
    //
    // the whole edit area need refresh
    //
    if (HEditorMouseAction && HMainEditor.SelectStart != 0 && HMainEditor.SelectEnd != 0) {
      if (HMainEditor.SelectStart != HMainEditorBackupVar.SelectStart) {
        if (HMainEditor.SelectStart >= HMainEditorBackupVar.SelectStart && HMainEditorBackupVar.SelectStart != 0) {
          StartRow = (HMainEditorBackupVar.SelectStart - 1) / 0x10 + 1;
        } else {
          StartRow = (HMainEditor.SelectStart - 1) / 0x10 + 1;
        }
      } else {
        StartRow = (HMainEditor.SelectStart - 1) / 0x10 + 1;
      }

      if (HMainEditor.SelectEnd <= HMainEditorBackupVar.SelectEnd) {
        EndRow = (HMainEditorBackupVar.SelectEnd - 1) / 0x10 + 1;
      } else {
        EndRow = (HMainEditor.SelectEnd - 1) / 0x10 + 1;
      }
      //
      // swap
      //
      if (StartRow > EndRow) {
        Tmp       = StartRow;
        StartRow  = EndRow;
        EndRow    = Tmp;
      }

      FStartRow = StartRow;
      FEndRow   = EndRow;

      StartRow  = 2 + StartRow - HBufferImage.LowVisibleRow;
      EndRow    = 2 + EndRow - HBufferImage.LowVisibleRow;

    } else {
      //
      // not mouse selection actions
      //
      FStartRow = HBufferImage.LowVisibleRow;
      StartRow  = 2;
      EndRow    = (HMainEditor.ScreenSize.Row - 4);
    }
    //
    // no line
    //
    if (HBufferImage.Lines == NULL) {
      HBufferImageRestoreMousePosition ();
      HBufferImageRestorePosition ();
      gST->ConOut->EnableCursor (gST->ConOut, TRUE);
      return EFI_SUCCESS;
    }
    //
    // get the first line that will be displayed
    //
    Line = HMoveLine (FStartRow - HBufferImage.BufferPosition.Row);
    if (Line == NULL) {
      gST->ConOut->EnableCursor (gST->ConOut, TRUE);
      return EFI_LOAD_ERROR;
    }

    Link  = &(Line->Link);
    Row   = StartRow;
    do {
      Line = CR (Link, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);

      //
      // print line at row
      //
      HBufferImagePrintLine (
        Line,
        Row,
        HBufferImage.LowVisibleRow + Row - 2,
        Orig,
        New
        );

      Link = Link->ForwardLink;
      Row++;
    } while (Link != HBufferImage.ListHead && Row <= EndRow);

    while (Row <= EndRow) {
      HEditorClearLine (Row);
      Row++;
    }
    //
    // while not file end and not screen full
    //
  }

  HBufferImageRestoreMousePosition ();
  HBufferImageRestorePosition ();

  HBufferImageNeedRefresh         = FALSE;
  HBufferImageOnlyLineNeedRefresh = FALSE;
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageRead (
  IN CONST CHAR16                   *FileName,
  IN CONST CHAR16                   *DiskName,
  IN UINTN                          DiskOffset,
  IN UINTN                          DiskSize,
  IN UINTN                          MemOffset,
  IN UINTN                          MemSize,
  IN EDIT_FILE_TYPE                 BufferType,
  IN BOOLEAN                        Recover
  )
{
  EFI_STATUS                      Status;
  EDIT_FILE_TYPE                  BufferTypeBackup;

  //
  // variable initialization
  //
  Status = EFI_SUCCESS;
  HBufferImage.BufferType = BufferType;

  //
  // three types of buffer supported
  //   file buffer
  //   disk buffer
  //   memory buffer
  //
  BufferTypeBackup = HBufferImage.BufferType;

  switch (BufferType) {
  case FileTypeFileBuffer:
    Status = HFileImageRead (FileName, Recover);
    break;

  case FileTypeDiskBuffer:
    Status = HDiskImageRead (DiskName, DiskOffset, DiskSize, Recover);
    break;

  case FileTypeMemBuffer:
    Status = HMemImageRead (MemOffset, MemSize, Recover);
    break;
  }

  if (EFI_ERROR (Status)) {
    HBufferImage.BufferType = BufferTypeBackup;
  }

  return Status;
}

EFI_STATUS
HBufferImageSave (
  IN CHAR16                         *FileName,
  IN CHAR16                         *DiskName,
  IN UINTN                          DiskOffset,
  IN UINTN                          DiskSize,
  IN UINTN                          MemOffset,
  IN UINTN                          MemSize,
  IN EDIT_FILE_TYPE                 BufferType
  )
{
  EFI_STATUS                      Status;
  EDIT_FILE_TYPE                  BufferTypeBackup;

  //
  // variable initialization
  //
  Status            = EFI_SUCCESS;
  BufferTypeBackup  = HBufferImage.BufferType;

  switch (HBufferImage.BufferType) {
  //
  // file buffer
  //
  case FileTypeFileBuffer:
    Status = HFileImageSave (FileName);
    break;

  //
  // disk buffer
  //
  case FileTypeDiskBuffer:
    Status = HDiskImageSave (DiskName, DiskOffset, DiskSize);
    break;

  //
  // memory buffer
  //
  case FileTypeMemBuffer:
    Status = HMemImageSave (MemOffset, MemSize);
    break;
  }

  if (EFI_ERROR (Status)) {
    HBufferImage.BufferType = BufferTypeBackup;
  }

  return Status;
}

HEFI_EDITOR_LINE *
HBufferImageCreateLine (
  VOID
  )
/*++

Routine Description: 

  Create a new line and append it to the line list
    Fields affected:
    NumLines
    Lines 

Arguments:  

  None

Returns:  

  NULL -- create line failed
  Not NULL -- the line created

--*/
{
  HEFI_EDITOR_LINE  *Line;

  //
  // allocate for line structure
  //
  Line = AllocateZeroPool (sizeof (HEFI_EDITOR_LINE));
  if (Line == NULL) {
    return NULL;
  }

  Line->Signature = EFI_EDITOR_LINE_LIST;
  Line->Size      = 0;

  HBufferImage.NumLines++;

  //
  // insert to line list
  //
  InsertTailList (HBufferImage.ListHead, &Line->Link);

  if (HBufferImage.Lines == NULL) {
    HBufferImage.Lines = CR (
                          HBufferImage.ListHead->ForwardLink,
                          HEFI_EDITOR_LINE,
                          Link,
                          EFI_EDITOR_LINE_LIST
                          );
  }

  return Line;
}

EFI_STATUS
HBufferImageFree (
  VOID
  )
/*++

Routine Description: 

  Function called when load a new file in. It will free all the old lines
  and set FileModified field to FALSE

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{
  //
  // free all lines
  //
  HBufferImageFreeLines ();

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageHandleInput (
  IN  EFI_INPUT_KEY *Key
  )
/*++

Routine Description: 

  Dispatch input to different handler

Arguments:  

  Key -- input key
   the keys can be:
     ASCII KEY
      Backspace/Delete
      Direction key: up/down/left/right/pgup/pgdn
      Home/End
      INS

Returns:  

  EFI_SUCCESS
  EFI_LOAD_ERROR
  EFI_OUT_OF_RESOURCES

--*/
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  switch (Key->ScanCode) {
  //
  // ordinary key
  //
  case SCAN_NULL:
    Status = HBufferImageDoCharInput (Key->UnicodeChar);
    break;

  //
  // up arrow
  //
  case SCAN_UP:
    Status = HBufferImageScrollUp ();
    break;

  //
  // down arrow
  //
  case SCAN_DOWN:
    Status = HBufferImageScrollDown ();
    break;

  //
  // right arrow
  //
  case SCAN_RIGHT:
    Status = HBufferImageScrollRight ();
    break;

  //
  // left arrow
  //
  case SCAN_LEFT:
    Status = HBufferImageScrollLeft ();
    break;

  //
  // page up
  //
  case SCAN_PAGE_UP:
    Status = HBufferImagePageUp ();
    break;

  //
  // page down
  //
  case SCAN_PAGE_DOWN:
    Status = HBufferImagePageDown ();
    break;

  //
  // delete
  //
  case SCAN_DELETE:
    Status = HBufferImageDoDelete ();
    break;

  //
  // home
  //
  case SCAN_HOME:
    Status = HBufferImageHome ();
    break;

  //
  // end
  //
  case SCAN_END:
    Status = HBufferImageEnd ();
    break;

  default:
    Status = StatusBarSetStatusString (L"Unknown Command");
    break;
  }

  return Status;
}

EFI_STATUS
HBufferImageDoCharInput (
  IN  CHAR16  Char
  )
/*++

Routine Description: 

  ASCII key + Backspace + return

Arguments:  

  Char -- input char

Returns:  

  EFI_SUCCESS
  EFI_LOAD_ERROR
  EFI_OUT_OF_RESOURCES

--*/
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  switch (Char) {
  case 0:
    break;

  case 0x08:
    Status = HBufferImageDoBackspace ();
    break;

  case 0x09:
  case 0x0a:
  case 0x0d:
    //
    // Tabs, Returns are thought as nothing
    //
    break;

  default:
    //
    // DEAL WITH ASCII CHAR, filter out thing like ctrl+f
    //
    if (Char > 127 || Char < 32) {
      Status = StatusBarSetStatusString (L"Unknown Command");
    } else {
      Status = HBufferImageAddChar (Char);
    }

    break;
  }

  return Status;
}

INTN
HBufferImageCharToHex (
  IN CHAR16 Char
  )
/*++

Routine Description: 

  change char to int value based on Hex

Arguments:  

  Char -- input char

Returns:  

  int value;


--*/
{
  //
  // change the character to hex
  //
  if (Char >= L'0' && Char <= L'9') {
    return (INTN) (Char - L'0');
  }

  if (Char >= L'a' && Char <= L'f') {
    return (INTN) (Char - L'a' + 10);
  }

  if (Char >= L'A' && Char <= L'F') {
    return (INTN) (Char - L'A' + 10);
  }

  return -1;
}

EFI_STATUS
HBufferImageAddChar (
  IN  CHAR16  Char
  )
/*++

Routine Description: 

  Add character

Arguments:  

  Char -- input char

Returns:  

  EFI_SUCCESS
  EFI_OUT_OF_RESOURCES

--*/
{
  HEFI_EDITOR_LINE  *Line;
  HEFI_EDITOR_LINE  *NewLine;
  INTN              Value;
  UINT8             Old;
  UINTN             FRow;
  UINTN             FCol;
  BOOLEAN           High;

  Value = HBufferImageCharToHex (Char);

  //
  // invalid input
  //
  if (Value == -1) {
    return EFI_SUCCESS;
  }

  Line  = HBufferImage.CurrentLine;
  FRow  = HBufferImage.BufferPosition.Row;
  FCol  = HBufferImage.BufferPosition.Column;
  High  = HBufferImage.HighBits;

  //
  // only needs to refresh current line
  //
  HBufferImageOnlyLineNeedRefresh = TRUE;

  //
  // not a full line and beyond the last character
  //
  if (FCol > Line->Size) {
    //
    // cursor always at high 4 bits
    // and always put input to the low 4 bits
    //
    Line->Buffer[Line->Size] = (UINT8) Value;
    Line->Size++;
    High = FALSE;
  } else {

    Old = Line->Buffer[FCol - 1];

    //
    // always put the input to the low 4 bits
    //
    Old                     = (UINT8) (Old & 0x0f);
    Old                     = (UINT8) (Old << 4);
    Old                     = (UINT8) (Value + Old);
    Line->Buffer[FCol - 1]  = Old;

    //
    // at the low 4 bits of the last character of a full line
    // so if no next line, need to create a new line
    //
    if (High == FALSE && FCol == 0x10) {

      HBufferImageOnlyLineNeedRefresh = FALSE;
      HBufferImageNeedRefresh         = TRUE;

      if (Line->Link.ForwardLink == HBufferImage.ListHead) {
        //
        // last line
        //
        // create a new line
        //
        NewLine = HBufferImageCreateLine ();
        if (NewLine == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }
        //
        // end of NULL
        //
      }
      //
      // end of == ListHead
      //
    }
    //
    // end of == 0x10
    //
    // if already at end of this line, scroll it to the start of next line
    //
    if (FCol == 0x10 && High == FALSE) {
      //
      // definitely has next line
      //
      FRow++;
      FCol  = 1;
      High  = TRUE;
    } else {
      //
      // if not at end of this line, just move to next column
      //
      if (!High) {
        FCol++;
      }

      if (High) {
        High = FALSE;
      } else {
        High = TRUE;
      }

    }
    //
    // end of ==FALSE
    //
  }
  //
  // move cursor to right
  //
  HBufferImageMovePosition (FRow, FCol, High);

  if (!HBufferImage.Modified) {
    HBufferImage.Modified = TRUE;
  }

  return EFI_SUCCESS;
}

BOOLEAN
HInCurrentScreen (
  IN  UINTN FileRow
  )
/*++

Routine Description: 

  Check user specified FileRow and FileCol is in current screen

Arguments:  

  FileRow -- Row of file position ( start from 1 )


Returns:  

  TRUE
  FALSE

--*/
{
  if (FileRow >= HBufferImage.LowVisibleRow && FileRow <= HBufferImage.LowVisibleRow + (HMainEditor.ScreenSize.Row - 5) - 1) {
    return TRUE;
  }

  return FALSE;
}

BOOLEAN
HAboveCurrentScreen (
  IN  UINTN FileRow
  )
/*++

Routine Description: 

  Check user specified FileRow is above current screen

Arguments:  

  FileRow -- Row of file position ( start from 1 )
  
Returns:  

  TRUE
  FALSE

--*/
{
  if (FileRow < HBufferImage.LowVisibleRow) {
    return TRUE;
  }

  return FALSE;
}

BOOLEAN
HUnderCurrentScreen (
  IN  UINTN FileRow
  )
/*++

Routine Description: 

  Check user specified FileRow is under current screen

Arguments:  

  FileRow -- Row of file position ( start from 1 )

Returns:  

  TRUE
  FALSE

--*/
{
  if (FileRow > HBufferImage.LowVisibleRow + (HMainEditor.ScreenSize.Row - 5) - 1) {
    return TRUE;
  }

  return FALSE;
}

VOID
HBufferImageMovePosition (
  IN UINTN    NewFilePosRow,
  IN UINTN    NewFilePosCol,
  IN BOOLEAN  HighBits
  )
/*++

Routine Description: 

  According to cursor's file position, adjust screen display

Arguments:  

  NewFilePosRow -- Row of file position ( start from 1 )
  NewFilePosCol -- Column of file position ( start from 1 )   
  HighBits      -- cursor will on high4 bits or low4 bits

Returns:  

  None

--*/
{
  INTN    RowGap;
  UINTN   Abs;
  BOOLEAN Above;
  BOOLEAN Under;
  UINTN   NewDisplayCol;

  //
  // CALCULATE gap between current file position and new file position
  //
  RowGap                = NewFilePosRow - HBufferImage.BufferPosition.Row;

  Under                 = HUnderCurrentScreen (NewFilePosRow);
  Above                 = HAboveCurrentScreen (NewFilePosRow);

  HBufferImage.HighBits = HighBits;

  //
  // if is below current screen
  //
  if (Under) {
    //
    // display row will be unchanged
    //
    HBufferImage.BufferPosition.Row = NewFilePosRow;
  } else {
    if (Above) {
      //
      // has enough above line, so display row unchanged
      // not has enough above lines, so the first line is
      // at the first display line
      //
      if (NewFilePosRow < (HBufferImage.DisplayPosition.Row - 2 + 1)) {
        HBufferImage.DisplayPosition.Row = NewFilePosRow + 2 - 1;
      }

      HBufferImage.BufferPosition.Row = NewFilePosRow;
    } else {
      //
      // in current screen
      //
      HBufferImage.BufferPosition.Row = NewFilePosRow;
      if (RowGap <= 0) {
        Abs = (UINTN)ABS(RowGap);
        HBufferImage.DisplayPosition.Row -= Abs;
      } else {
        HBufferImage.DisplayPosition.Row += RowGap;
      }

    }
  }

  HBufferImage.LowVisibleRow = HBufferImage.BufferPosition.Row - (HBufferImage.DisplayPosition.Row - 2);

  //
  // always in current screen
  //
  HBufferImage.BufferPosition.Column  = NewFilePosCol;

  NewDisplayCol                       = 10 + (NewFilePosCol - 1) * 3;
  if (NewFilePosCol > 0x8) {
    NewDisplayCol++;
  }

  if (HighBits == FALSE) {
    NewDisplayCol++;
  }

  HBufferImage.DisplayPosition.Column = NewDisplayCol;

  //
  // let CurrentLine point to correct line;
  //
  HBufferImage.CurrentLine = HMoveCurrentLine (RowGap);

}

EFI_STATUS
HBufferImageScrollRight (
  VOID
  )
/*++

Routine Description: 

  Scroll cursor to right

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{
  HEFI_EDITOR_LINE  *Line;
  UINTN             FRow;
  UINTN             FCol;

  //
  // scroll right will always move to the high4 bits of the next character
  //
  HBufferImageNeedRefresh         = FALSE;
  HBufferImageOnlyLineNeedRefresh = FALSE;

  Line = HBufferImage.CurrentLine;

  FRow = HBufferImage.BufferPosition.Row;
  FCol = HBufferImage.BufferPosition.Column;

  //
  // this line is not full and no next line
  //
  if (FCol > Line->Size) {
    return EFI_SUCCESS;
  }
  //
  // if already at end of this line, scroll it to the start of next line
  //
  if (FCol == 0x10) {
    //
    // has next line
    //
    if (Line->Link.ForwardLink != HBufferImage.ListHead) {
      FRow++;
      FCol = 1;

    } else {
      return EFI_SUCCESS;
    }
  } else {
    //
    // if not at end of this line, just move to next column
    //
    FCol++;

  }

  HBufferImageMovePosition (FRow, FCol, TRUE);

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageScrollLeft (
  VOID
  )
/*++

Routine Description: 

  Scroll cursor to left

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{

  HEFI_EDITOR_LINE  *Line;
  UINTN             FRow;
  UINTN             FCol;

  HBufferImageNeedRefresh         = FALSE;
  HBufferImageOnlyLineNeedRefresh = FALSE;

  Line = HBufferImage.CurrentLine;

  FRow = HBufferImage.BufferPosition.Row;
  FCol = HBufferImage.BufferPosition.Column;

  //
  // if already at start of this line, so move to the end of previous line
  //
  if (FCol <= 1) {
    //
    // has previous line
    //
    if (Line->Link.BackLink != HBufferImage.ListHead) {
      FRow--;
      Line  = CR (Line->Link.BackLink, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);
      FCol  = Line->Size;
    } else {
      return EFI_SUCCESS;
    }
  } else {
    //
    // if not at start of this line, just move to previous column
    //
    FCol--;
  }

  HBufferImageMovePosition (FRow, FCol, TRUE);

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageScrollDown (
  VOID
  )
/*++

Routine Description: 

  Scroll cursor to the next line

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{
  HEFI_EDITOR_LINE  *Line;
  UINTN             FRow;
  UINTN             FCol;
  BOOLEAN           HighBits;

  Line      = HBufferImage.CurrentLine;

  FRow      = HBufferImage.BufferPosition.Row;
  FCol      = HBufferImage.BufferPosition.Column;
  HighBits  = HBufferImage.HighBits;

  //
  // has next line
  //
  if (Line->Link.ForwardLink != HBufferImage.ListHead) {
    FRow++;
    Line = CR (Line->Link.ForwardLink, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);

    //
    // if the next line is not that long, so move to end of next line
    //
    if (FCol > Line->Size) {
      FCol      = Line->Size + 1;
      HighBits  = TRUE;
    }

  } else {
    return EFI_SUCCESS;
  }

  HBufferImageMovePosition (FRow, FCol, HighBits);

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageScrollUp (
  VOID
  )
/*++

Routine Description: 

  Scroll cursor to previous line

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{
  HEFI_EDITOR_LINE  *Line;
  UINTN             FRow;
  UINTN             FCol;

  Line  = HBufferImage.CurrentLine;

  FRow  = HBufferImage.BufferPosition.Row;
  FCol  = HBufferImage.BufferPosition.Column;

  //
  // has previous line
  //
  if (Line->Link.BackLink != HBufferImage.ListHead) {
    FRow--;

  } else {
    return EFI_SUCCESS;
  }

  HBufferImageMovePosition (FRow, FCol, HBufferImage.HighBits);

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImagePageDown (
  VOID
  )
/*++

Routine Description: 

  Scroll cursor to next page

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{
  HEFI_EDITOR_LINE  *Line;
  UINTN             FRow;
  UINTN             FCol;
  UINTN             Gap;
  BOOLEAN           HighBits;

  Line      = HBufferImage.CurrentLine;

  FRow      = HBufferImage.BufferPosition.Row;
  FCol      = HBufferImage.BufferPosition.Column;
  HighBits  = HBufferImage.HighBits;

  //
  // has next page
  //
  if (HBufferImage.NumLines >= FRow + (HMainEditor.ScreenSize.Row - 5)) {
    Gap = (HMainEditor.ScreenSize.Row - 5);
  } else {
    //
    // MOVE CURSOR TO LAST LINE
    //
    Gap = HBufferImage.NumLines - FRow;
  }
  //
  // get correct line
  //
  Line = HMoveLine (Gap);

  //
  // if that line, is not that long, so move to the end of that line
  //
  if (Line != NULL && FCol > Line->Size) {
    FCol      = Line->Size + 1;
    HighBits  = TRUE;
  }

  FRow += Gap;

  HBufferImageMovePosition (FRow, FCol, HighBits);

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImagePageUp (
  VOID
  )
/*++

Routine Description: 

  Scroll cursor to previous page

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{
  HEFI_EDITOR_LINE  *Line;
  UINTN             FRow;
  UINTN             FCol;
  UINTN             Gap;
  INTN              Retreat;

  Line  = HBufferImage.CurrentLine;

  FRow  = HBufferImage.BufferPosition.Row;
  FCol  = HBufferImage.BufferPosition.Column;

  //
  // has previous page
  //
  if (FRow > (HMainEditor.ScreenSize.Row - 5)) {
    Gap = (HMainEditor.ScreenSize.Row - 5);
  } else {
    //
    // the first line of file will displayed on the first line of screen
    //
    Gap = FRow - 1;
  }

  Retreat = Gap;
  Retreat = -Retreat;

  //
  // get correct line
  //
  Line = HMoveLine (Retreat);

  FRow -= Gap;

  HBufferImageMovePosition (FRow, FCol, HBufferImage.HighBits);

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageHome (
  VOID
  )
/*++

Routine Description: 

  Scroll cursor to start of line

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{
  HEFI_EDITOR_LINE  *Line;
  UINTN             FRow;
  UINTN             FCol;
  BOOLEAN           HighBits;

  Line = HBufferImage.CurrentLine;

  //
  // curosr will at the high bit
  //
  FRow      = HBufferImage.BufferPosition.Row;
  FCol      = 1;
  HighBits  = TRUE;

  //
  // move cursor position
  //
  HBufferImageMovePosition (FRow, FCol, HighBits);

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageEnd (
  VOID
  )
/*++

Routine Description: 

  Scroll cursor to end of line

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{
  HEFI_EDITOR_LINE  *Line;
  UINTN             FRow;
  UINTN             FCol;
  BOOLEAN           HighBits;

  //
  // need refresh mouse
  //
  HBufferImageMouseNeedRefresh  = TRUE;

  Line                          = HBufferImage.CurrentLine;

  FRow                          = HBufferImage.BufferPosition.Row;

  if (Line->Size == 0x10) {
    FCol      = Line->Size;
    HighBits  = FALSE;
  } else {
    FCol      = Line->Size + 1;
    HighBits  = TRUE;
  }
  //
  // move cursor position
  //
  HBufferImageMovePosition (FRow, FCol, HighBits);

  return EFI_SUCCESS;
}

UINTN
HBufferImageGetTotalSize (
  VOID
  )
{
  UINTN             Size;

  HEFI_EDITOR_LINE  *Line;

  //
  // calculate the total size of whole line list's buffer
  //
  if (HBufferImage.Lines == NULL) {
    return 0;
  }

  Line = CR (
          HBufferImage.ListHead->BackLink,
          HEFI_EDITOR_LINE,
          Link,
          EFI_EDITOR_LINE_LIST
          );
  //
  // one line at most 0x10
  //
  Size = 0x10 * (HBufferImage.NumLines - 1) + Line->Size;

  return Size;
}

EFI_STATUS
HBufferImageDeleteCharacterFromBuffer (
  IN  UINTN         Pos,
  IN  UINTN         Count,
  OUT UINT8         *DeleteBuffer
  )
/*++
Routine Description:

  Delete character from buffer
  
Arguments:

  Pos - Position, Pos starting from 0
  Count - Count
  DeleteBuffer - DeleteBuffer

Returns:

  EFI_SUCCESS Success
  
--*/
{
  UINTN             Index;

  VOID              *Buffer;
  UINT8             *BufferPtr;
  UINTN             Size;

  HEFI_EDITOR_LINE  *Line;
  LIST_ENTRY    *Link;
  UINTN             StartRow;

  UINTN             OldFCol;
  UINTN             OldFRow;
  UINTN             OldPos;

  UINTN             NewPos;

  EFI_STATUS        Status;

  //
  // get the line that start position is at
  //
  StartRow  = Pos / 0x10;

  Size      = HBufferImageGetTotalSize ();

  if (Size < Count) {
    return EFI_LOAD_ERROR;
  }

  if (Size == 0) {
    return EFI_SUCCESS;
  }

  //
  // relocate all the HBufferImage fields
  //
  OldFRow = HBufferImage.BufferPosition.Row;
  OldFCol = HBufferImage.BufferPosition.Column;
  OldPos  = (OldFRow - 1) * 0x10 + OldFCol - 1;

  if (Pos > 0) {
    //
    // has character before it,
    // so locate according to block's previous character
    //
    NewPos = Pos - 1;

  } else {
    //
    // has no character before it,
    // so locate according to block's next character
    //
    NewPos = 0;
  }

  HBufferImageMovePosition (NewPos / 0x10 + 1, NewPos % 0x10 + 1, TRUE);

  Buffer = AllocateZeroPool (Size);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  HBufferImageListToBuffer (Buffer, Size);

  BufferPtr = (UINT8 *) Buffer;

  //
  // pass deleted buffer out
  //
  if (DeleteBuffer != NULL) {
    for (Index = 0; Index < Count; Index++) {
      DeleteBuffer[Index] = BufferPtr[Pos + Index];
    }
  }
  //
  // delete the part from Pos
  //
  for (Index = Pos; Index < Size - Count; Index++) {
    BufferPtr[Index] = BufferPtr[Index + Count];
  }

  Size -= Count;

  HBufferImageFreeLines ();

  Status = HBufferImageBufferToList (Buffer, Size);
  FreePool (Buffer);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Link = HMainEditor.BufferImage->ListHead->ForwardLink;
  for (Index = 0; Index < NewPos / 0x10; Index++) {
    Link = Link->ForwardLink;
  }

  Line                      = CR (Link, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);
  HBufferImage.CurrentLine  = Line;

  //
  // if current cursor position if inside select area
  // then move it to the block's NEXT character
  //
  if (OldPos >= Pos && OldPos < (Pos + Count)) {
    NewPos = Pos;
  } else {
    if (OldPos < Pos) {
      NewPos = OldPos;
    } else {
      NewPos = OldPos - Count;
    }
  }

  HBufferImageMovePosition (NewPos / 0x10 + 1, NewPos % 0x10 + 1, TRUE);

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageAddCharacterToBuffer (
  IN  UINTN          Pos,
  IN  UINTN          Count,
  IN  UINT8          *AddBuffer
  )
/*++'
Routine Description:

  Add character to buffer, add before pos

Arguments:

  Pos - Position, Pos starting from 0
  Count - Count
  AddBuffer - Add buffer

Returns:

  EFI_SUCCESS Success
  
--*/
{
  INTN              Index;

  VOID              *Buffer;
  UINT8             *BufferPtr;
  UINTN             Size;

  HEFI_EDITOR_LINE  *Line;

  LIST_ENTRY    *Link;
  UINTN             StartRow;

  UINTN             OldFCol;
  UINTN             OldFRow;
  UINTN             OldPos;

  UINTN             NewPos;

  //
  // get the line that start position is at
  //
  StartRow  = Pos / 0x10;

  Size      = HBufferImageGetTotalSize ();

  //
  // relocate all the HBufferImage fields
  //
  OldFRow = HBufferImage.BufferPosition.Row;
  OldFCol = HBufferImage.BufferPosition.Column;
  OldPos  = (OldFRow - 1) * 0x10 + OldFCol - 1;

  //
  // move cursor before Pos
  //
  if (Pos > 0) {
    NewPos = Pos - 1;
  } else {
    NewPos = 0;
  }

  HBufferImageMovePosition (NewPos / 0x10 + 1, NewPos % 0x10 + 1, TRUE);

  Buffer = AllocateZeroPool (Size + Count);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  HBufferImageListToBuffer (Buffer, Size);

  BufferPtr = (UINT8 *) Buffer;

  //
  // get a place to add
  //
  for (Index = (INTN) (Size + Count - 1); Index >= (INTN) Pos; Index--) {
    BufferPtr[Index] = BufferPtr[Index - Count];
  }
  //
  // add the buffer
  //
  for (Index = (INTN) 0; Index < (INTN) Count; Index++) {
    BufferPtr[Index + Pos] = AddBuffer[Index];
  }

  Size += Count;

  HBufferImageFreeLines ();

  HBufferImageBufferToList (Buffer, Size);

  FreePool (Buffer);

  Link = HMainEditor.BufferImage->ListHead->ForwardLink;
  for (Index = 0; Index < (INTN) NewPos / 0x10; Index++) {
    Link = Link->ForwardLink;
  }

  Line                      = CR (Link, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);
  HBufferImage.CurrentLine  = Line;

  if (OldPos >= Pos) {
    NewPos = OldPos + Count;
  } else {
    NewPos = OldPos;
  }

  HBufferImageMovePosition (NewPos / 0x10 + 1, NewPos % 0x10 + 1, TRUE);

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageDoBackspace (
  VOID
  )
/*++

Routine Description: 

  delete the previous character

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{
  HEFI_EDITOR_LINE  *Line;

  UINTN             FileColumn;
  UINTN             FPos;
  BOOLEAN           LastLine;

  //
  // variable initialization
  //
  LastLine = FALSE;

  //
  // already the first character
  //
  if (HBufferImage.BufferPosition.Row == 1 && HBufferImage.BufferPosition.Column == 1) {
    return EFI_SUCCESS;
  }

  FPos        = (HBufferImage.BufferPosition.Row - 1) * 0x10 + HBufferImage.BufferPosition.Column - 1;

  FileColumn  = HBufferImage.BufferPosition.Column;

  Line        = HBufferImage.CurrentLine;
  LastLine    = FALSE;
  if (Line->Link.ForwardLink == HBufferImage.ListHead && FileColumn > 1) {
    LastLine = TRUE;
  }

  HBufferImageDeleteCharacterFromBuffer (FPos - 1, 1, NULL);

  //
  // if is the last line
  // then only this line need to be refreshed
  //
  if (LastLine) {
    HBufferImageNeedRefresh         = FALSE;
    HBufferImageOnlyLineNeedRefresh = TRUE;
  } else {
    HBufferImageNeedRefresh         = TRUE;
    HBufferImageOnlyLineNeedRefresh = FALSE;
  }

  if (!HBufferImage.Modified) {
    HBufferImage.Modified = TRUE;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageDoDelete (
  VOID
  )
/*++

Routine Description: 

  Delete current character from line

Arguments:  

  None

Returns:  

  EFI_SUCCESS

--*/
{

  HEFI_EDITOR_LINE  *Line;

  BOOLEAN           LastLine;
  UINTN             FileColumn;
  UINTN             FPos;

  FPos        = (HBufferImage.BufferPosition.Row - 1) * 0x10 + HBufferImage.BufferPosition.Column - 1;

  FileColumn  = HBufferImage.BufferPosition.Column;

  Line        = HBufferImage.CurrentLine;

  //
  // if beyond the last character
  //
  if (FileColumn > Line->Size) {
    return EFI_SUCCESS;
  }

  LastLine = FALSE;
  if (Line->Link.ForwardLink == HBufferImage.ListHead) {
    LastLine = TRUE;
  }

  HBufferImageDeleteCharacterFromBuffer (FPos, 1, NULL);

  //
  // if is the last line
  // then only this line need to be refreshed
  //
  if (LastLine) {
    HBufferImageNeedRefresh         = FALSE;
    HBufferImageOnlyLineNeedRefresh = TRUE;
  } else {
    HBufferImageNeedRefresh         = TRUE;
    HBufferImageOnlyLineNeedRefresh = FALSE;
  }

  if (!HBufferImage.Modified) {
    HBufferImage.Modified = TRUE;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageBufferToList (
  IN VOID   *Buffer,
  IN UINTN  Bytes
  )
{
  UINTN             i;
  UINTN             j;
  UINTN             Left;
  HEFI_EDITOR_LINE  *Line;
  UINT8             *BufferPtr;

  i         = 0;
  Left      = 0;
  BufferPtr = (UINT8 *) Buffer;

  //
  // parse file content line by line
  //
  while (i < Bytes) {
    if (Bytes - i >= 0x10) {
      Left = 0x10;
    } else {
      Left = Bytes - i;
    }

    //
    // allocate a new line
    //
    Line = HBufferImageCreateLine ();
    if (Line == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Line->Size = Left;

    for (j = 0; j < Left; j++) {
      Line->Buffer[j] = BufferPtr[i];
      i++;
    }

  }

  //
  // last line is a full line, SO create a new line
  //
  if (Left == 0x10 || Bytes == 0) {
    Line = HBufferImageCreateLine ();
    if (Line == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
HBufferImageListToBuffer (
  IN VOID   *Buffer,
  IN UINTN  Bytes
  )
{
  UINTN             Count;
  UINTN             Index;
  HEFI_EDITOR_LINE  *Line;
  LIST_ENTRY    *Link;
  UINT8             *BufferPtr;

  //
  // change the line list to a large buffer
  //
  if (HBufferImage.Lines == NULL) {
    return EFI_SUCCESS;
  }

  Link      = &HBufferImage.Lines->Link;
  Count     = 0;
  BufferPtr = (UINT8 *) Buffer;

  //
  // deal line by line
  //
  while (Link != HBufferImage.ListHead) {

    Line = CR (Link, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);

    if (Count + Line->Size > Bytes) {
      return EFI_SUCCESS;
    }

    for (Index = 0; Index < Line->Size; Index++) {
      BufferPtr[Index] = Line->Buffer[Index];
    }

    Count += Line->Size;
    BufferPtr += Line->Size;

    Link = Link->ForwardLink;
  }

  return EFI_SUCCESS;
}

VOID
HBufferImageAdjustMousePosition (
  IN INT32 TextX,
  IN INT32 TextY
  )
{
  UINTN X;
  UINTN Y;
  UINTN AbsX;
  UINTN AbsY;

  //
  // TextX and TextY is mouse movement data returned by mouse driver
  // This function will change it to MousePosition
  //
  //
  // get absolute X value
  //
  if (TextX >= 0) {
    AbsX = TextX;
  } else {
    AbsX = -TextX;
  }
  //
  // get absolute Y value
  //
  if (TextY >= 0) {
    AbsY = TextY;
  } else {
    AbsY = -TextY;
  }

  X = HBufferImage.MousePosition.Column;
  Y = HBufferImage.MousePosition.Row;

  if (TextX >= 0) {
    X += TextX;
  } else {
    if (X >= AbsX) {
      X -= AbsX;
    } else {
      X = 0;
    }
  }

  if (TextY >= 0) {
    Y += TextY;
  } else {
    if (Y >= AbsY) {
      Y -= AbsY;
    } else {
      Y = 0;
    }
  }
  //
  // check whether new mouse column position is beyond screen
  // if not, adjust it
  //
  if (X >= 10 && X <= (10 + 0x10 * 3 - 1)) {
    HBufferImage.MousePosition.Column = X;
  } else if (X < 10) {
    HBufferImage.MousePosition.Column = 10;
  } else if (X > (10 + 0x10 * 3 - 1)) {
    HBufferImage.MousePosition.Column = 10 + 0x10 * 3 - 1;
  }
  //
  // check whether new mouse row position is beyond screen
  // if not, adjust it
  //
  if (Y >= 2 && Y <= (HMainEditor.ScreenSize.Row - 4)) {
    HBufferImage.MousePosition.Row = Y;
  } else if (Y < 2) {
    HBufferImage.MousePosition.Row = 2;
  } else if (Y > (HMainEditor.ScreenSize.Row - 4)) {
    HBufferImage.MousePosition.Row = (HMainEditor.ScreenSize.Row - 4);
  }

}
