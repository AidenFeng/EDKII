
/** @file
  This file defines the encoding for the VFR (Visual Form Representation) language.
  Framework IFR is primarily consumed by the EFI presentation engine, and produced by EFI
  internal application and drivers as well as all add-in card option-ROM drivers

  Copyright (c) 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  These definitions are from Framework Specification HII 0.92.

**/

#ifndef __FRAMEWORK_INTERNAL_FORMREPRESENTATION_H__
#define __FRAMEWORK_INTERNAL_FORMREPRESENTATION_H__



//
// IFR Op codes
//
#define FRAMEWORK_EFI_IFR_FORM_OP                 0x01
#define FRAMEWORK_EFI_IFR_SUBTITLE_OP             0x02
#define FRAMEWORK_EFI_IFR_TEXT_OP                 0x03
#define FRAMEWORK_EFI_IFR_GRAPHIC_OP              0x04
#define FRAMEWORK_EFI_IFR_ONE_OF_OP               0x05
#define FRAMEWORK_EFI_IFR_CHECKBOX_OP             0x06
#define FRAMEWORK_EFI_IFR_NUMERIC_OP              0x07
#define FRAMEWORK_EFI_IFR_PASSWORD_OP             0x08
#define FRAMEWORK_EFI_IFR_ONE_OF_OPTION_OP        0x09  // ONEOF OPTION field
#define FRAMEWORK_EFI_IFR_SUPPRESS_IF_OP          0x0A
#define FRAMEWORK_EFI_IFR_END_FORM_OP             0x0B
#define FRAMEWORK_EFI_IFR_HIDDEN_OP               0x0C
#define FRAMEWORK_EFI_IFR_END_FORM_SET_OP         0x0D
#define FRAMEWORK_EFI_IFR_FORM_SET_OP             0x0E
#define FRAMEWORK_EFI_IFR_REF_OP                  0x0F
#define FRAMEWORK_EFI_IFR_END_ONE_OF_OP           0x10
#define FRAMEWORK_EFI_IFR_END_OP                  FRAMEWORK_EFI_IFR_END_ONE_OF_OP
#define FRAMEWORK_EFI_IFR_INCONSISTENT_IF_OP      0x11
#define FRAMEWORK_EFI_IFR_EQ_ID_VAL_OP            0x12
#define FRAMEWORK_EFI_IFR_EQ_ID_ID_OP             0x13
#define FRAMEWORK_EFI_IFR_EQ_ID_LIST_OP           0x14
#define FRAMEWORK_EFI_IFR_AND_OP                  0x15
#define FRAMEWORK_EFI_IFR_OR_OP                   0x16
#define FRAMEWORK_EFI_IFR_NOT_OP                  0x17
#define FRAMEWORK_EFI_IFR_END_IF_OP               0x18  // for endif of inconsistentif, suppressif, grayoutif
#define FRAMEWORK_EFI_IFR_GRAYOUT_IF_OP           0x19
#define FRAMEWORK_EFI_IFR_DATE_OP                 0x1A
#define FRAMEWORK_EFI_IFR_TIME_OP                 0x1B
#define FRAMEWORK_EFI_IFR_STRING_OP               0x1C
#define FRAMEWORK_EFI_IFR_LABEL_OP                0x1D
#define FRAMEWORK_EFI_IFR_SAVE_DEFAULTS_OP        0x1E
#define FRAMEWORK_EFI_IFR_RESTORE_DEFAULTS_OP     0x1F
#define FRAMEWORK_EFI_IFR_BANNER_OP               0x20
#define FRAMEWORK_EFI_IFR_INVENTORY_OP            0x21
#define FRAMEWORK_EFI_IFR_EQ_VAR_VAL_OP           0x22
#define FRAMEWORK_EFI_IFR_ORDERED_LIST_OP         0x23
#define FRAMEWORK_EFI_IFR_VARSTORE_OP             0x24
#define FRAMEWORK_EFI_IFR_VARSTORE_SELECT_OP      0x25
#define FRAMEWORK_EFI_IFR_VARSTORE_SELECT_PAIR_OP 0x26
#define FRAMEWORK_EFI_IFR_TRUE_OP                 0x27
#define FRAMEWORK_EFI_IFR_FALSE_OP                0x28
#define FRAMEWORK_EFI_IFR_GT_OP                   0x29
#define FRAMEWORK_EFI_IFR_GE_OP                   0x2A
#define FRAMEWORK_EFI_IFR_OEM_DEFINED_OP          0x2B
#define FRAMEWORK_EFI_IFR_LAST_OPCODE             FRAMEWORK_EFI_IFR_OEM_DEFINED_OP
#define FRAMEWORK_EFI_IFR_OEM_OP                  0xFE
#define FRAMEWORK_EFI_IFR_NV_ACCESS_COMMAND       0xFF

//
// Define values for the flags fields in some VFR opcodes. These are
// bitmasks.
//
#define FRAMEWORK_EFI_IFR_FLAG_DEFAULT              0x01
#define FRAMEWORK_EFI_IFR_FLAG_MANUFACTURING        0x02
#define FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE          0x04
#define FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS            0x08
#define FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED       0x10
#define FRAMEWORK_EFI_IFR_FLAG_LATE_CHECK           0x20

#define EFI_NON_DEVICE_CLASS              0x00  // Useful when you do not want something in the Device Manager
#define EFI_DISK_DEVICE_CLASS             0x01
#define EFI_VIDEO_DEVICE_CLASS            0x02
#define EFI_NETWORK_DEVICE_CLASS          0x04
#define EFI_INPUT_DEVICE_CLASS            0x08
#define EFI_ON_BOARD_DEVICE_CLASS         0x10
#define EFI_OTHER_DEVICE_CLASS            0x20

#define EFI_SETUP_APPLICATION_SUBCLASS    0x00
#define EFI_GENERAL_APPLICATION_SUBCLASS  0x01
#define EFI_FRONT_PAGE_SUBCLASS           0x02
#define EFI_SINGLE_USE_SUBCLASS           0x03  // Used to display a single entity and then exit

//
// Used to flag dynamically created op-codes. This is meaningful to the IFR Library set
// and the browser since we need to distinguish between compiled NV map data and created data.
// We do not allow new entries to be created in the NV map dynamically however we still need
// to display this information correctly.  To dynamically create op-codes and assume that their
// data will be saved, ensure that the NV starting location they refer to is pre-defined in the
// NV map.
//
#define FRAMEWORK_EFI_IFR_FLAG_CREATED  128


#pragma pack(1)
//
// IFR Structure definitions
//
typedef struct {
  UINT8 OpCode;
  UINT8 Length;
} FRAMEWORK_EFI_IFR_OP_HEADER;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER     Header;
  EFI_GUID              Guid;
  STRING_REF            FormSetTitle;
  STRING_REF            Help;
  EFI_PHYSICAL_ADDRESS  CallbackHandle;
  UINT16                Class;
  UINT16                SubClass;
  UINT16                NvDataSize; // set once, size of the NV data as defined in the script
} FRAMEWORK_EFI_IFR_FORM_SET;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            FormId;
  STRING_REF        FormTitle;
} FRAMEWORK_EFI_IFR_FORM;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            LabelId;
} FRAMEWORK_EFI_IFR_LABEL;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  STRING_REF        SubTitle;
} FRAMEWORK_EFI_IFR_SUBTITLE;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  STRING_REF        Help;
  STRING_REF        Text;
  STRING_REF        TextTwo;
  UINT8             Flags;          // This is included solely for purposes of interactive/dynamic support.
  UINT16            Key;            // Value to be passed to caller to identify this particular op-code
} FRAMEWORK_EFI_IFR_TEXT;

//
// goto
//
typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            FormId;
  STRING_REF        Prompt;
  STRING_REF        Help;   // The string Token for the context-help
  UINT8             Flags;  // This is included solely for purposes of interactive/dynamic support.
  UINT16            Key;    // Value to be passed to caller to identify this particular op-code
} FRAMEWORK_EFI_IFR_REF;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
} FRAMEWORK_EFI_IFR_END_FORM;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
} FRAMEWORK_EFI_IFR_END_FORM_SET;

//
// Also notice that the IFR_ONE_OF and IFR_CHECK_BOX are identical in structure......code assumes this to be true, if this ever
// changes we need to revisit the InitializeTagStructures code
//
typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId; // The ID designating what the question is about...sucked in from a #define, likely in the form of a variable name
  UINT8             Width;      // The Size of the Data being saved
  STRING_REF        Prompt;     // The String Token for the Prompt
  STRING_REF        Help;       // The string Token for the context-help
} FRAMEWORK_EFI_IFR_ONE_OF;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId; // The offset in NV for storage of the data
  UINT8             MaxEntries; // The maximum number of options in the ordered list (=size of NVStore)
  STRING_REF        Prompt;     // The string token for the prompt
  STRING_REF        Help;       // The string token for the context-help
} FRAMEWORK_EFI_IFR_ORDERED_LIST;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId; // The ID designating what the question is about...sucked in from a #define, likely in the form of a variable name
  UINT8             Width;      // The Size of the Data being saved
  STRING_REF        Prompt;     // The String Token for the Prompt
  STRING_REF        Help;       // The string Token for the context-help
  UINT8             Flags;      // For now, if non-zero, means that it is the default option, - further definition likely
  UINT16            Key;        // Value to be passed to caller to identify this particular op-code
} FRAMEWORK_EFI_IFR_CHECKBOX, FRAMEWORK_EFI_IFR_CHECK_BOX;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  STRING_REF        Option;     // The string token describing the option
  UINT16            Value;      // The value associated with this option that is stored in the NVRAM if chosen
  UINT8             Flags;      // For now, if non-zero, means that it is the default option, - further definition likely above
  UINT16            Key;        // Value to be passed to caller to identify this particular op-code
} FRAMEWORK_EFI_IFR_ONE_OF_OPTION;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId; // The ID designating what the question is about...sucked in from a #define, likely in the form of a variable name
  UINT8             Width;      // The Size of the Data being saved
  STRING_REF        Prompt;     // The String Token for the Prompt
  STRING_REF        Help;       // The string Token for the context-help
  UINT8             Flags;      // This is included solely for purposes of interactive/dynamic support.
  UINT16            Key;        // Value to be passed to caller to identify this particular op-code
  UINT16            Minimum;
  UINT16            Maximum;
  UINT16            Step;       // If step is 0, then manual input is specified, otherwise, left/right arrow selection is called for
  UINT16            Default;
} FRAMEWORK_EFI_IFR_NUMERIC;

//
// There is an interesting twist with regards to Time and Date.  This is one of the few items which can accept input from
// a user, however may or may not need to use storage in the NVRAM space.  The decided method for determining if NVRAM space
// will be used (only for a TimeOp or DateOp) is:  If .QuestionId == 0 && .Width == 0 (normally an impossibility) then use system
// resources to store the data away and not NV resources.  In other words, the setup engine will call gRT->SetTime, and gRT->SetDate
// for the saving of data, and the values displayed will be from the gRT->GetXXXX series of calls.
//
typedef struct {
  FRAMEWORK_EFI_IFR_NUMERIC Hour;
  FRAMEWORK_EFI_IFR_NUMERIC Minute;
  FRAMEWORK_EFI_IFR_NUMERIC Second;
} FRAMEWORK_EFI_IFR_TIME;

typedef struct {
  FRAMEWORK_EFI_IFR_NUMERIC Year;
  FRAMEWORK_EFI_IFR_NUMERIC Month;
  FRAMEWORK_EFI_IFR_NUMERIC Day;
} FRAMEWORK_EFI_IFR_DATE;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId;   // The ID designating what the question is about...sucked in from a #define, likely in the form of a variable name
  UINT8             Width;        // The Size of the Data being saved -- BUGBUG -- remove someday
  STRING_REF        Prompt;       // The String Token for the Prompt
  STRING_REF        Help;         // The string Token for the context-help
  UINT8             Flags;        // This is included solely for purposes of interactive/dynamic support.
  UINT16            Key;          // Value to be passed to caller to identify this particular op-code
  UINT8             MinSize;      // Minimum allowable sized password
  UINT8             MaxSize;      // Maximum allowable sized password
  UINT16            Encoding;
} FRAMEWORK_EFI_IFR_PASSWORD;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId;   // The ID designating what the question is about...sucked in from a #define, likely in the form of a variable name
  UINT8             Width;        // The Size of the Data being saved -- BUGBUG -- remove someday
  STRING_REF        Prompt;       // The String Token for the Prompt
  STRING_REF        Help;         // The string Token for the context-help
  UINT8             Flags;        // This is included solely for purposes of interactive/dynamic support.
  UINT16            Key;          // Value to be passed to caller to identify this particular op-code
  UINT8             MinSize;      // Minimum allowable sized password
  UINT8             MaxSize;      // Maximum allowable sized password
} FRAMEWORK_EFI_IFR_STRING;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
} FRAMEWORK_EFI_IFR_END_ONE_OF;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            Value;
  UINT16            Key;
} FRAMEWORK_EFI_IFR_HIDDEN;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT8             Flags;
} FRAMEWORK_EFI_IFR_SUPPRESS;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT8             Flags;
} FRAMEWORK_EFI_IFR_GRAY_OUT;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  STRING_REF        Popup;
  UINT8             Flags;
} FRAMEWORK_EFI_IFR_INCONSISTENT;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId;   // offset into variable storage
  UINT8             Width;        // size of variable storage
  UINT16            Value;        // value to compare against
} FRAMEWORK_EFI_IFR_EQ_ID_VAL;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId;   // offset into variable storage
  UINT8             Width;        // size of variable storage
  UINT16            ListLength;
  UINT16            ValueList[1];
} FRAMEWORK_EFI_IFR_EQ_ID_LIST;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId1;  // offset into variable storage for first value to compare
  UINT8             Width;        // size of variable storage (must be same for both)
  UINT16            QuestionId2;  // offset into variable storage for second value to compare
} FRAMEWORK_EFI_IFR_EQ_ID_ID;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            VariableId;   // offset into variable storage
  UINT16            Value;        // value to compare against
} FRAMEWORK_EFI_IFR_EQ_VAR_VAL;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
} FRAMEWORK_EFI_IFR_AND;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
} FRAMEWORK_EFI_IFR_OR;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
} FRAMEWORK_EFI_IFR_NOT;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
} FRAMEWORK_EFI_IFR_END_EXPR, FRAMEWORK_EFI_IFR_END_IF;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            FormId;
  STRING_REF        Prompt;
  STRING_REF        Help;
  UINT8             Flags;
  UINT16            Key;
} FRAMEWORK_EFI_IFR_SAVE_DEFAULTS;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  STRING_REF        Help;
  STRING_REF        Text;
  STRING_REF        TextTwo;      // optional text
} FRAMEWORK_EFI_IFR_INVENTORY;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  EFI_GUID          Guid;         // GUID for the variable
  UINT16            VarId;        // variable store ID, as referenced elsewhere in the form
  UINT16            Size;         // size of the variable storage
} FRAMEWORK_EFI_IFR_VARSTORE;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            VarId;        // variable store ID, as referenced elsewhere in the form
} FRAMEWORK_EFI_IFR_VARSTORE_SELECT;

//
// Used for the ideqid VFR statement where two variable stores may be referenced in the
// same VFR statement.
// A browser should treat this as an FRAMEWORK_EFI_IFR_VARSTORE_SELECT statement and assume that all following
// IFR opcodes use the VarId as defined here.
//
typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            VarId;          // variable store ID, as referenced elsewhere in the form
  UINT16            SecondaryVarId; // variable store ID, as referenced elsewhere in the form
} FRAMEWORK_EFI_IFR_VARSTORE_SELECT_PAIR;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
} FRAMEWORK_EFI_IFR_TRUE;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
} FRAMEWORK_EFI_IFR_FALSE;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
} FRAMEWORK_EFI_IFR_GT;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
} FRAMEWORK_EFI_IFR_GE;

//
// Save defaults and restore defaults have same structure
//
#define FRAMEWORK_EFI_IFR_RESTORE_DEFAULTS  FRAMEWORK_EFI_IFR_SAVE_DEFAULTS

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  STRING_REF        Title;        // The string token for the banner title
  UINT16            LineNumber;   // 1-based line number
  UINT8             Alignment;    // left, center, or right-aligned
} FRAMEWORK_EFI_IFR_BANNER;

#define FRAMEWORK_EFI_IFR_BANNER_ALIGN_LEFT   0
#define FRAMEWORK_EFI_IFR_BANNER_ALIGN_CENTER 1
#define FRAMEWORK_EFI_IFR_BANNER_ALIGN_RIGHT  2
#define FRAMEWORK_EFI_IFR_BANNER_TIMEOUT      0xFF

#pragma pack()

#endif
