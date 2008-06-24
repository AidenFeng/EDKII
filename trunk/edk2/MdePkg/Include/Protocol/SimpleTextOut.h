/** @file
  Simple Text Out protocol from the UEFI 2.0 specification.

  Abstraction of a very simple text based output device like VGA text mode or
  a serial terminal. The Simple Text Out protocol instance can represent
  a single hardware device or a virtual device that is an agregation
  of multiple physical devices.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __SIMPLE_TEXT_OUT_H__
#define __SIMPLE_TEXT_OUT_H__

#include <PiDxe.h>

#define EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID \
  { \
    0x387477c2, 0x69c7, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

//
// Protocol GUID defined in EFI1.1.
// 
#define SIMPLE_TEXT_OUTPUT_PROTOCOL   EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

//
// Backward-compatible with EFI1.1.
// 
typedef EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   SIMPLE_TEXT_OUTPUT_INTERFACE;

//
// Define's for required EFI Unicode Box Draw characters
//
#define BOXDRAW_HORIZONTAL                  0x2500
#define BOXDRAW_VERTICAL                    0x2502
#define BOXDRAW_DOWN_RIGHT                  0x250c
#define BOXDRAW_DOWN_LEFT                   0x2510
#define BOXDRAW_UP_RIGHT                    0x2514
#define BOXDRAW_UP_LEFT                     0x2518
#define BOXDRAW_VERTICAL_RIGHT              0x251c
#define BOXDRAW_VERTICAL_LEFT               0x2524
#define BOXDRAW_DOWN_HORIZONTAL             0x252c
#define BOXDRAW_UP_HORIZONTAL               0x2534
#define BOXDRAW_VERTICAL_HORIZONTAL         0x253c
#define BOXDRAW_DOUBLE_HORIZONTAL           0x2550
#define BOXDRAW_DOUBLE_VERTICAL             0x2551
#define BOXDRAW_DOWN_RIGHT_DOUBLE           0x2552
#define BOXDRAW_DOWN_DOUBLE_RIGHT           0x2553
#define BOXDRAW_DOUBLE_DOWN_RIGHT           0x2554
#define BOXDRAW_DOWN_LEFT_DOUBLE            0x2555
#define BOXDRAW_DOWN_DOUBLE_LEFT            0x2556
#define BOXDRAW_DOUBLE_DOWN_LEFT            0x2557
#define BOXDRAW_UP_RIGHT_DOUBLE             0x2558
#define BOXDRAW_UP_DOUBLE_RIGHT             0x2559
#define BOXDRAW_DOUBLE_UP_RIGHT             0x255a
#define BOXDRAW_UP_LEFT_DOUBLE              0x255b
#define BOXDRAW_UP_DOUBLE_LEFT              0x255c
#define BOXDRAW_DOUBLE_UP_LEFT              0x255d
#define BOXDRAW_VERTICAL_RIGHT_DOUBLE       0x255e
#define BOXDRAW_VERTICAL_DOUBLE_RIGHT       0x255f
#define BOXDRAW_DOUBLE_VERTICAL_RIGHT       0x2560
#define BOXDRAW_VERTICAL_LEFT_DOUBLE        0x2561
#define BOXDRAW_VERTICAL_DOUBLE_LEFT        0x2562
#define BOXDRAW_DOUBLE_VERTICAL_LEFT        0x2563
#define BOXDRAW_DOWN_HORIZONTAL_DOUBLE      0x2564
#define BOXDRAW_DOWN_DOUBLE_HORIZONTAL      0x2565
#define BOXDRAW_DOUBLE_DOWN_HORIZONTAL      0x2566
#define BOXDRAW_UP_HORIZONTAL_DOUBLE        0x2567
#define BOXDRAW_UP_DOUBLE_HORIZONTAL        0x2568
#define BOXDRAW_DOUBLE_UP_HORIZONTAL        0x2569
#define BOXDRAW_VERTICAL_HORIZONTAL_DOUBLE  0x256a
#define BOXDRAW_VERTICAL_DOUBLE_HORIZONTAL  0x256b
#define BOXDRAW_DOUBLE_VERTICAL_HORIZONTAL  0x256c

//
// EFI Required Block Elements Code Chart
//
#define BLOCKELEMENT_FULL_BLOCK   0x2588
#define BLOCKELEMENT_LIGHT_SHADE  0x2591

//
// EFI Required Geometric Shapes Code Chart
//
#define GEOMETRICSHAPE_UP_TRIANGLE    0x25b2
#define GEOMETRICSHAPE_RIGHT_TRIANGLE 0x25ba
#define GEOMETRICSHAPE_DOWN_TRIANGLE  0x25bc
#define GEOMETRICSHAPE_LEFT_TRIANGLE  0x25c4

//
// EFI Required Arrow shapes
//
#define ARROW_LEFT  0x2190
#define ARROW_UP    0x2191
#define ARROW_RIGHT 0x2192
#define ARROW_DOWN  0x2193

//
// EFI Console Colours
//
#define EFI_BLACK                 0x00
#define EFI_BLUE                  0x01
#define EFI_GREEN                 0x02
#define EFI_CYAN                  (EFI_BLUE | EFI_GREEN)
#define EFI_RED                   0x04
#define EFI_MAGENTA               (EFI_BLUE | EFI_RED)
#define EFI_BROWN                 (EFI_GREEN | EFI_RED)
#define EFI_LIGHTGRAY             (EFI_BLUE | EFI_GREEN | EFI_RED)
#define EFI_BRIGHT                0x08
#define EFI_DARKGRAY              (EFI_BRIGHT)
#define EFI_LIGHTBLUE             (EFI_BLUE | EFI_BRIGHT)
#define EFI_LIGHTGREEN            (EFI_GREEN | EFI_BRIGHT)
#define EFI_LIGHTCYAN             (EFI_CYAN | EFI_BRIGHT)
#define EFI_LIGHTRED              (EFI_RED | EFI_BRIGHT)
#define EFI_LIGHTMAGENTA          (EFI_MAGENTA | EFI_BRIGHT)
#define EFI_YELLOW                (EFI_BROWN | EFI_BRIGHT)
#define EFI_WHITE                 (EFI_BLUE | EFI_GREEN | EFI_RED | EFI_BRIGHT)

#define EFI_TEXT_ATTR(f, b)       ((f) | ((b) << 4))

#define EFI_BACKGROUND_BLACK      0x00
#define EFI_BACKGROUND_BLUE       0x10
#define EFI_BACKGROUND_GREEN      0x20
#define EFI_BACKGROUND_CYAN       (EFI_BACKGROUND_BLUE | EFI_BACKGROUND_GREEN)
#define EFI_BACKGROUND_RED        0x40
#define EFI_BACKGROUND_MAGENTA    (EFI_BACKGROUND_BLUE | EFI_BACKGROUND_RED)
#define EFI_BACKGROUND_BROWN      (EFI_BACKGROUND_GREEN | EFI_BACKGROUND_RED)
#define EFI_BACKGROUND_LIGHTGRAY  (EFI_BACKGROUND_BLUE | EFI_BACKGROUND_GREEN | EFI_BACKGROUND_RED)

//
// We currently define attributes from 0 - 7F for color manipulations
// To internally handle the local display characteristics for a particular character, we are defining
// Bit 7 to signify the local glyph representation for a character.  If turned on, glyphs will be
// pulled from the wide glyph database and will display locally as a wide character (16 X 19 versus 8 X 19)
// If bit 7 is off, the narrow glyph database will be used.  This does NOT affect information that is sent to
// non-local displays (e.g. serial or LAN consoles).
//
#define EFI_WIDE_ATTRIBUTE  0x80

/**
  Reset the text output device hardware and optionaly run diagnostics

  @param  This                 Protocol instance pointer.
  @param  ExtendedVerification Driver may perform more exhaustive verfication
                               operation of the device during reset.

  @retval EFI_SUCCESS          The text output device was reset.
  @retval EFI_DEVICE_ERROR     The text output device is not functioning correctly and
                               could not be reset.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_RESET)(
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL           *This,
  IN BOOLEAN                                ExtendedVerification
  )
;

/**
  Write a Unicode string to the output device.

  @param  This   Protocol instance pointer.
  @param  String The NULL-terminated Unicode string to be displayed on the output
                 device(s). All output devices must also support the Unicode
                 drawing defined in this file.

  @retval EFI_SUCCESS             The string was output to the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting to output
                                  the text.
  @retval EFI_UNSUPPORTED         The output device's mode is not currently in a
                                  defined text mode.
  @retval EFI_WARN_UNKNOWN_GLYPH  This warning code indicates that some of the
                                  characters in the Unicode string could not be
                                  rendered and were skipped.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_STRING)(
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL        *This,
  IN CHAR16                                 *String
  )
;

/**
  Verifies that all characters in a Unicode string can be output to the 
  target device.

  @param  This   Protocol instance pointer.
  @param  String The NULL-terminated Unicode string to be examined for the output
                 device(s).

  @retval EFI_SUCCESS      The device(s) are capable of rendering the output string.
  @retval EFI_UNSUPPORTED  Some of the characters in the Unicode string cannot be
                           rendered by one or more of the output devices mapped
                           by the EFI handle.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_TEST_STRING)(
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL        *This,
  IN CHAR16                                 *String
  )
;

/**
  Returns information for an available text mode that the output device(s)
  supports.

  @param  This       Protocol instance pointer.
  @param  ModeNumber The mode number to return information on.
  @param  Columns    Returns the geometry of the text output device for the
                     requested ModeNumber.
  @param  Rows       Returns the geometry of the text output device for the
                     requested ModeNumber.
                                          
  @retval EFI_SUCCESS      The requested mode information was returned.
  @retval EFI_DEVICE_ERROR The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED  The mode number was not valid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_QUERY_MODE)(
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL        *This,
  IN UINTN                                  ModeNumber,
  OUT UINTN                                 *Columns,
  OUT UINTN                                 *Rows
  )
;

/**
  Sets the output device(s) to a specified mode.

  @param  This       Protocol instance pointer.
  @param  ModeNumber The mode number to set.

  @retval EFI_SUCCESS      The requested text mode was set.
  @retval EFI_DEVICE_ERROR The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED  The mode number was not valid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_SET_MODE)(
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL        *This,
  IN UINTN                                  ModeNumber
  )
;

/**
  Sets the background and foreground colors for the OutputString () and
  ClearScreen () functions.

  @param  This      Protocol instance pointer.
  @param  Attribute The attribute to set. Bits 0..3 are the foreground color, and
                    bits 4..6 are the background color. All other bits are undefined
                    and must be zero. The valid Attributes are defined in this file.

  @retval EFI_SUCCESS     The attribute was set.
  @retval EFI_DEVICE_     ERROR The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED The attribute requested is not defined.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL        *This,
  IN UINTN                                  Attribute
  )
;

/**
  Clears the output device(s) display to the currently selected background 
  color.

  @param  This              Protocol instance pointer.
                           
  @retval  EFI_SUCCESS      The operation completed successfully.
  @retval  EFI_DEVICE_ERROR The device had an error and could not complete the request.
  @retval  EFI_UNSUPPORTED  The output device is not in a valid text mode.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_CLEAR_SCREEN)(
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *This
  )
;

/**
  Sets the current coordinates of the cursor position

  @param  This        Protocol instance pointer.
  @param  Column      The position to set the cursor to. Must be greater than or
                      equal to zero and less than the number of columns and rows
                      by QueryMode ().
  @param  Row         The position to set the cursor to. Must be greater than or
                      equal to zero and less than the number of columns and rows
                      by QueryMode ().

  @retval EFI_SUCCESS      The operation completed successfully.
  @retval EFI_DEVICE_ERROR The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED  The output device is not in a valid text mode, or the
                           cursor position is invalid for the current mode.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_SET_CURSOR_POSITION)(
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL        *This,
  IN UINTN                                  Column,
  IN UINTN                                  Row
  )
;

/**
  Makes the cursor visible or invisible

  @param  This    Protocol instance pointer.
  @param  Visible If TRUE, the cursor is set to be visible. If FALSE, the cursor is
                  set to be invisible.

  @retval EFI_SUCCESS      The operation completed successfully.
  @retval EFI_DEVICE_ERROR The device had an error and could not complete the
                           request, or the device does not support changing
                           the cursor mode.
  @retval EFI_UNSUPPORTED  The output device is not in a valid text mode.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_ENABLE_CURSOR)(
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL        *This,
  IN BOOLEAN                                Visible
  )
;

/**
  Mode Structure pointed to by Simple Text Out protocol.

  MaxMode   - The number of modes supported by QueryMode () and SetMode ().
  Mode      - The text mode of the output device(s).
  Attribute - The current character output attribute
  CursorColumn  - The cursor's column.
  CursorRow     - The cursor's row.
  CursorVisible - The cursor is currently visbile or not.

**/
typedef struct {
  INT32   MaxMode;

  //
  // current settings
  //
  INT32   Mode;
  INT32   Attribute;
  INT32   CursorColumn;
  INT32   CursorRow;
  BOOLEAN CursorVisible;
} EFI_SIMPLE_TEXT_OUTPUT_MODE;

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
  EFI_TEXT_RESET                Reset;

  EFI_TEXT_STRING               OutputString;
  EFI_TEXT_TEST_STRING          TestString;

  EFI_TEXT_QUERY_MODE           QueryMode;
  EFI_TEXT_SET_MODE             SetMode;
  EFI_TEXT_SET_ATTRIBUTE        SetAttribute;

  EFI_TEXT_CLEAR_SCREEN         ClearScreen;
  EFI_TEXT_SET_CURSOR_POSITION  SetCursorPosition;
  EFI_TEXT_ENABLE_CURSOR        EnableCursor;

  //
  // Current mode
  //
  EFI_SIMPLE_TEXT_OUTPUT_MODE   *Mode;
};

extern EFI_GUID gEfiSimpleTextOutProtocolGuid;

#endif
