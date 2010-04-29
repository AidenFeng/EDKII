/** @file
  Root include file for Mde Package Base type modules

  This is the include file for any module of type base. Base modules only use 
  types defined via this include file and can be ported easily to any 
  environment. There are a set of base libraries in the Mde Package that can
  be used to implement base modules.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef __BASE_H__
#define __BASE_H__

//
// Include processor specific binding
//
#include <ProcessorBind.h>


/**
  Verifies the storage size of a given data type.
  
  This macro generates a divide by zero error or a zero size array declaration in
  the preprocessor if the size is incorrect.  These are declared as "extern" so
  the space for these arrays will not be in the modules.

  @param  TYPE  The date type to determine the size of.
  @param  Size  The expected size for the TYPE.

**/
#define VERIFY_SIZE_OF(TYPE, Size) extern UINT8 _VerifySizeof##TYPE[(sizeof(TYPE) == (Size)) / (sizeof(TYPE) == (Size))]

//
// Verify that ProcessorBind.h produced UEFI Data Types that are compliant with 
// Section 2.3.1 of the UEFI 2.3 Specification.  
//
VERIFY_SIZE_OF (BOOLEAN, 1);
VERIFY_SIZE_OF (INT8, 1);
VERIFY_SIZE_OF (UINT8, 1);
VERIFY_SIZE_OF (INT16, 2);
VERIFY_SIZE_OF (UINT16, 2);
VERIFY_SIZE_OF (INT32, 4);
VERIFY_SIZE_OF (UINT32, 4);
VERIFY_SIZE_OF (INT64, 8);
VERIFY_SIZE_OF (UINT64, 8);
VERIFY_SIZE_OF (CHAR8, 1);
VERIFY_SIZE_OF (CHAR16, 2);

//
// The Microsoft* C compiler can removed references to unreferenced data items
//  if the /OPT:REF linker option is used. We defined a macro as this is a 
//  a non standard extension
//
#if defined(_MSC_EXTENSIONS) && !defined (MDE_CPU_EBC)
  ///
  /// Remove global variable from the linked image if there are no references to 
  /// it after all compiler and linker optimizations have been performed.
  ///
  ///
  #define GLOBAL_REMOVE_IF_UNREFERENCED __declspec(selectany)
#else
  ///
  /// Remove the global variable from the linked image if there are no references 
  ///  to it after all compiler and linker optimizations have been performed.
  ///
  ///
  #define GLOBAL_REMOVE_IF_UNREFERENCED
#endif

//
// For symbol name in GNU assembly code, an extra "_" is necessary
//
#if defined(__GNUC__)
  ///
  /// Private worker functions for ASM_PFX()
  ///
  #define _CONCATENATE(a, b)  __CONCATENATE(a, b)
  #define __CONCATENATE(a, b) a ## b

  ///
  /// The __USER_LABEL_PREFIX__ macro predefined by GNUC represents the prefix
  /// on symbols in assembly language.
  ///
  #define ASM_PFX(name) _CONCATENATE (__USER_LABEL_PREFIX__, name)
#endif

#if __APPLE__
  //
  // Apple extension that is used by the linker to optimize code size 
  // with assembly functions. Put at the end of your .S files
  //
  #define ASM_FUNCTION_REMOVE_IF_UNREFERENCED  .subsections_via_symbols
#else
  #define ASM_FUNCTION_REMOVE_IF_UNREFERENCED
#endif

#ifdef __CC_ARM
  //
  // Older RVCT ARM compilers don't fully support #pragma pack and require __packed 
  // as a prefix for the structure.
  //
  #define PACKED  __packed
#else
  #define PACKED
#endif

///
/// 128 bit buffer containing a unique identifier value.  
/// Unless otherwise specified, aligned on a 64 bit boundary.
///
typedef struct {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} GUID;

//
// 8-bytes unsigned value that represents a physical system address.
//
typedef UINT64 PHYSICAL_ADDRESS;

///
/// LIST_ENTRY structure definition.
///
typedef struct _LIST_ENTRY LIST_ENTRY;

///
/// _LIST_ENTRY structure definition.
///
struct _LIST_ENTRY {
  LIST_ENTRY  *ForwardLink;
  LIST_ENTRY  *BackLink;
};

//
// Modifiers to abstract standard types to aid in debug of problems
//

///
/// Datum is read-only.
///
#define CONST     const

///
/// Datum is scoped to the current file or function.
///
#define STATIC    static

///
/// Undeclared type.
///
#define VOID      void

//
// Modifiers for Data Types used to self document code.
// This concept is borrowed for UEFI specification.
//

///
/// Datum is passed to the function.
///
#define IN

///
/// Datum is returned from the function.
///
#define OUT

///
/// Passing the datum to the function is optional, and a NULL
/// is passed if the value is not supplied.
///
#define OPTIONAL

//
//  UEFI specification claims 1 and 0. We are concerned about the 
//  complier portability so we did it this way.
//

///
/// Boolean true value.  UEFI Specification defines this value to be 1,
/// but this form is more portable.
///
#define TRUE  ((BOOLEAN)(1==1))

///
/// Boolean false value.  UEFI Specification defines this value to be 0,
/// but this form is more portable.
///
#define FALSE ((BOOLEAN)(0==1))

///
/// NULL pointer (VOID *)
///
#define NULL  ((VOID *) 0)


#define  BIT0     0x00000001
#define  BIT1     0x00000002
#define  BIT2     0x00000004
#define  BIT3     0x00000008
#define  BIT4     0x00000010
#define  BIT5     0x00000020
#define  BIT6     0x00000040
#define  BIT7     0x00000080
#define  BIT8     0x00000100
#define  BIT9     0x00000200
#define  BIT10    0x00000400
#define  BIT11    0x00000800
#define  BIT12    0x00001000
#define  BIT13    0x00002000
#define  BIT14    0x00004000
#define  BIT15    0x00008000
#define  BIT16    0x00010000
#define  BIT17    0x00020000
#define  BIT18    0x00040000
#define  BIT19    0x00080000
#define  BIT20    0x00100000
#define  BIT21    0x00200000
#define  BIT22    0x00400000
#define  BIT23    0x00800000
#define  BIT24    0x01000000
#define  BIT25    0x02000000
#define  BIT26    0x04000000
#define  BIT27    0x08000000
#define  BIT28    0x10000000
#define  BIT29    0x20000000
#define  BIT30    0x40000000
#define  BIT31    0x80000000
#define  BIT32    0x0000000100000000ULL
#define  BIT33    0x0000000200000000ULL
#define  BIT34    0x0000000400000000ULL
#define  BIT35    0x0000000800000000ULL
#define  BIT36    0x0000001000000000ULL
#define  BIT37    0x0000002000000000ULL
#define  BIT38    0x0000004000000000ULL
#define  BIT39    0x0000008000000000ULL
#define  BIT40    0x0000010000000000ULL
#define  BIT41    0x0000020000000000ULL
#define  BIT42    0x0000040000000000ULL
#define  BIT43    0x0000080000000000ULL
#define  BIT44    0x0000100000000000ULL
#define  BIT45    0x0000200000000000ULL
#define  BIT46    0x0000400000000000ULL
#define  BIT47    0x0000800000000000ULL
#define  BIT48    0x0001000000000000ULL
#define  BIT49    0x0002000000000000ULL
#define  BIT50    0x0004000000000000ULL
#define  BIT51    0x0008000000000000ULL
#define  BIT52    0x0010000000000000ULL
#define  BIT53    0x0020000000000000ULL
#define  BIT54    0x0040000000000000ULL
#define  BIT55    0x0080000000000000ULL
#define  BIT56    0x0100000000000000ULL
#define  BIT57    0x0200000000000000ULL
#define  BIT58    0x0400000000000000ULL
#define  BIT59    0x0800000000000000ULL
#define  BIT60    0x1000000000000000ULL
#define  BIT61    0x2000000000000000ULL
#define  BIT62    0x4000000000000000ULL
#define  BIT63    0x8000000000000000ULL

#define  SIZE_1KB    0x00000400
#define  SIZE_2KB    0x00000800
#define  SIZE_4KB    0x00001000
#define  SIZE_8KB    0x00002000
#define  SIZE_16KB   0x00004000
#define  SIZE_32KB   0x00008000
#define  SIZE_64KB   0x00010000
#define  SIZE_128KB  0x00020000
#define  SIZE_256KB  0x00040000
#define  SIZE_512KB  0x00080000
#define  SIZE_1MB    0x00100000
#define  SIZE_2MB    0x00200000
#define  SIZE_4MB    0x00400000
#define  SIZE_8MB    0x00800000
#define  SIZE_16MB   0x01000000
#define  SIZE_32MB   0x02000000
#define  SIZE_64MB   0x04000000
#define  SIZE_128MB  0x08000000
#define  SIZE_256MB  0x10000000
#define  SIZE_512MB  0x20000000
#define  SIZE_1GB    0x40000000
#define  SIZE_2GB    0x80000000
#define  SIZE_4GB    0x0000000100000000ULL
#define  SIZE_8GB    0x0000000200000000ULL
#define  SIZE_16GB   0x0000000400000000ULL
#define  SIZE_32GB   0x0000000800000000ULL
#define  SIZE_64GB   0x0000001000000000ULL
#define  SIZE_128GB  0x0000002000000000ULL
#define  SIZE_256GB  0x0000004000000000ULL
#define  SIZE_512GB  0x0000008000000000ULL
#define  SIZE_1TB    0x0000010000000000ULL
#define  SIZE_2TB    0x0000020000000000ULL
#define  SIZE_4TB    0x0000040000000000ULL
#define  SIZE_8TB    0x0000080000000000ULL
#define  SIZE_16TB   0x0000100000000000ULL
#define  SIZE_32TB   0x0000200000000000ULL
#define  SIZE_64TB   0x0000400000000000ULL
#define  SIZE_128TB  0x0000800000000000ULL
#define  SIZE_256TB  0x0001000000000000ULL
#define  SIZE_512TB  0x0002000000000000ULL
#define  SIZE_1PB    0x0004000000000000ULL
#define  SIZE_2PB    0x0008000000000000ULL
#define  SIZE_4PB    0x0010000000000000ULL
#define  SIZE_8PB    0x0020000000000000ULL
#define  SIZE_16PB   0x0040000000000000ULL
#define  SIZE_32PB   0x0080000000000000ULL
#define  SIZE_64PB   0x0100000000000000ULL
#define  SIZE_128PB  0x0200000000000000ULL
#define  SIZE_256PB  0x0400000000000000ULL
#define  SIZE_512PB  0x0800000000000000ULL
#define  SIZE_1EB    0x1000000000000000ULL
#define  SIZE_2EB    0x2000000000000000ULL
#define  SIZE_4EB    0x4000000000000000ULL
#define  SIZE_8EB    0x8000000000000000ULL
 
#define  BASE_1KB    0x00000400
#define  BASE_2KB    0x00000800
#define  BASE_4KB    0x00001000
#define  BASE_8KB    0x00002000
#define  BASE_16KB   0x00004000
#define  BASE_32KB   0x00008000
#define  BASE_64KB   0x00010000
#define  BASE_128KB  0x00020000
#define  BASE_256KB  0x00040000
#define  BASE_512KB  0x00080000
#define  BASE_1MB    0x00100000
#define  BASE_2MB    0x00200000
#define  BASE_4MB    0x00400000
#define  BASE_8MB    0x00800000
#define  BASE_16MB   0x01000000
#define  BASE_32MB   0x02000000
#define  BASE_64MB   0x04000000
#define  BASE_128MB  0x08000000
#define  BASE_256MB  0x10000000
#define  BASE_512MB  0x20000000
#define  BASE_1GB    0x40000000
#define  BASE_2GB    0x80000000
#define  BASE_4GB    0x0000000100000000ULL
#define  BASE_8GB    0x0000000200000000ULL
#define  BASE_16GB   0x0000000400000000ULL
#define  BASE_32GB   0x0000000800000000ULL
#define  BASE_64GB   0x0000001000000000ULL
#define  BASE_128GB  0x0000002000000000ULL
#define  BASE_256GB  0x0000004000000000ULL
#define  BASE_512GB  0x0000008000000000ULL
#define  BASE_1TB    0x0000010000000000ULL
#define  BASE_2TB    0x0000020000000000ULL
#define  BASE_4TB    0x0000040000000000ULL
#define  BASE_8TB    0x0000080000000000ULL
#define  BASE_16TB   0x0000100000000000ULL
#define  BASE_32TB   0x0000200000000000ULL
#define  BASE_64TB   0x0000400000000000ULL
#define  BASE_128TB  0x0000800000000000ULL
#define  BASE_256TB  0x0001000000000000ULL
#define  BASE_512TB  0x0002000000000000ULL
#define  BASE_1PB    0x0004000000000000ULL
#define  BASE_2PB    0x0008000000000000ULL
#define  BASE_4PB    0x0010000000000000ULL
#define  BASE_8PB    0x0020000000000000ULL
#define  BASE_16PB   0x0040000000000000ULL
#define  BASE_32PB   0x0080000000000000ULL
#define  BASE_64PB   0x0100000000000000ULL
#define  BASE_128PB  0x0200000000000000ULL
#define  BASE_256PB  0x0400000000000000ULL
#define  BASE_512PB  0x0800000000000000ULL
#define  BASE_1EB    0x1000000000000000ULL
#define  BASE_2EB    0x2000000000000000ULL
#define  BASE_4EB    0x4000000000000000ULL
#define  BASE_8EB    0x8000000000000000ULL

//
//  Support for variable length argument lists using the ANSI standard.
//  
//  Since we are using the ANSI standard we used the standard naming and
//  did not follow the coding convention
//
//  VA_LIST  - typedef for argument list.
//  VA_START (VA_LIST Marker, argument before the ...) - Init Marker for use.
//  VA_END (VA_LIST Marker) - Clear Marker
//  VA_ARG (VA_LIST Marker, var arg size) - Use Marker to get an argument from
//    the ... list. You must know the size and pass it in this macro.
//
//  example:
//
//  UINTN
//  ExampleVarArg (
//    IN UINTN  NumberOfArgs,
//    ...
//    )
//  {
//    VA_LIST Marker;
//    UINTN   Index;
//    UINTN   Result;
//
//    //
//    // Initialize the Marker
//    //
//    VA_START (Marker, NumberOfArgs);
//    for (Index = 0, Result = 0; Index < NumberOfArgs; Index++) {
//      //
//      // The ... list is a series of UINTN values, so average them up.
//      //
//      Result += VA_ARG (Marker, UINTN);
//    }
//
//    VA_END (Marker);
//    return Result
//  }
//

/**
  Return the size of argument that has been aligned to sizeof (UINTN).

  @param  n    The parameter size to be aligned.

  @return The aligned size.
**/
#define _INT_SIZE_OF(n) ((sizeof (n) + sizeof (UINTN) - 1) &~(sizeof (UINTN) - 1))

#if defined(__CC_ARM)
//
// RVCT ARM variable argument list support.
//

///
/// Variable used to traverse the list of arguments. This type can vary by 
/// implementation and could be an array or structure. 
///
#ifdef __APCS_ADSABI
  typedef int         *va_list[1];
  #define VA_LIST     va_list
#else
  typedef struct __va_list { void *__ap; } va_list;
  #define VA_LIST                          va_list
#endif

#define VA_START(Marker, Parameter)   __va_start(Marker, Parameter)

#define VA_ARG(Marker, TYPE)          __va_arg(Marker, TYPE)

#define VA_END(Marker)                ((void)0)

#elif defined(__GNUC__) && !defined(NO_BUILTIN_VA_FUNCS)
//
// Use GCC built-in macros for variable argument lists.
//

///
/// Variable used to traverse the list of arguments. This type can vary by 
/// implementation and could be an array or structure. 
///
typedef __builtin_va_list VA_LIST;

#define VA_START(Marker, Parameter)  __builtin_va_start (Marker, Parameter)

#define VA_ARG(Marker, TYPE)         ((sizeof (TYPE) < sizeof (UINTN)) ? (TYPE)(__builtin_va_arg (Marker, UINTN)) : (TYPE)(__builtin_va_arg (Marker, TYPE)))

#define VA_END(Marker)               __builtin_va_end (Marker)

#else
///
/// Variable used to traverse the list of arguments. This type can vary by 
/// implementation and could be an array or structure. 
///
typedef CHAR8 *VA_LIST;

/**
  Retrieves a pointer to the beginning of a variable argument list, based on 
  the name of the parameter that immediately precedes the variable argument list. 

  This function initializes Marker to point to the beginning of the variable  
  argument list that immediately follows Parameter.  The method for computing the 
  pointer to the next argument in the argument list is CPU-specific following the 
  EFIAPI ABI.

  @param   Marker       The VA_LIST used to traverse the list of arguments.
  @param   Parameter    The name of the parameter that immediately precedes 
                        the variable argument list.
  
  @return  A pointer to the beginning of a variable argument list.

**/
#define VA_START(Marker, Parameter) (Marker = (VA_LIST) & (Parameter) + _INT_SIZE_OF (Parameter))

/**
  Returns an argument of a specified type from a variable argument list and updates 
  the pointer to the variable argument list to point to the next argument. 

  This function returns an argument of the type specified by TYPE from the beginning 
  of the variable argument list specified by Marker.  Marker is then updated to point 
  to the next argument in the variable argument list.  The method for computing the 
  pointer to the next argument in the argument list is CPU-specific following the EFIAPI ABI.

  @param   Marker   VA_LIST used to traverse the list of arguments.
  @param   TYPE     The type of argument to retrieve from the beginning 
                    of the variable argument list.
  
  @return  An argument of the type specified by TYPE.

**/
#define VA_ARG(Marker, TYPE)   (*(TYPE *) ((Marker += _INT_SIZE_OF (TYPE)) - _INT_SIZE_OF (TYPE)))

/**
  Terminates the use of a variable argument list.

  This function initializes Marker so it can no longer be used with VA_ARG().  
  After this macro is used, the only way to access the variable argument list is 
  by using VA_START() again.

  @param   Marker   VA_LIST used to traverse the list of arguments.
  
**/
#define VA_END(Marker)      (Marker = (VA_LIST) 0)

#endif

///
/// Pointer to the start of a variable argument list stored in a memory buffer. Same as UINT8 *.
///
typedef UINTN  *BASE_LIST;

/**
  Returns the size of a data type in sizeof(UINTN) units rounded up to the nearest UINTN boundary.

  @param  TYPE  The date type to determine the size of.

  @return The size of TYPE in sizeof (UINTN) units rounded up to the nearest UINTN boundary.
**/
#define _BASE_INT_SIZE_OF(TYPE) ((sizeof (TYPE) + sizeof (UINTN) - 1) / sizeof (UINTN))

/**
  Returns an argument of a specified type from a variable argument list and updates 
  the pointer to the variable argument list to point to the next argument. 

  This function returns an argument of the type specified by TYPE from the beginning 
  of the variable argument list specified by Marker.  Marker is then updated to point 
  to the next argument in the variable argument list.  The method for computing the 
  pointer to the next argument in the argument list is CPU specific following the EFIAPI ABI.

  @param   Marker   The pointer to the beginning of a variable argument list.
  @param   TYPE     The type of argument to retrieve from the beginning 
                    of the variable argument list.
  
  @return  An argument of the type specified by TYPE.

**/
#define BASE_ARG(Marker, TYPE)   (*(TYPE *) ((Marker += _BASE_INT_SIZE_OF (TYPE)) - _BASE_INT_SIZE_OF (TYPE)))

/**
  The macro that returns the byte offset of a field in a data structure. 

  This function returns the offset, in bytes, of field specified by Field from the 
  beginning of the  data structure specified by TYPE. If TYPE does not contain Field, 
  the module will not compile.  

  @param   TYPE     The name of the data structure that contains the field specified by Field. 
  @param   Field    The name of the field in the data structure.
  
  @return  Offset, in bytes, of field.
  
**/
#define OFFSET_OF(TYPE, Field) ((UINTN) &(((TYPE *)0)->Field))

/**
  Macro that returns a pointer to the data structure that contains a specified field of 
  that data structure.  This is a lightweight method to hide information by placing a 
  public data structure inside a larger private data structure and using a pointer to 
  the public data structure to retrieve a pointer to the private data structure.

  This function computes the offset, in bytes, of field specified by Field from the beginning 
  of the  data structure specified by TYPE.  This offset is subtracted from Record, and is 
  used to return a pointer to a data structure of the type specified by TYPE. If the data type 
  specified by TYPE does not contain the field specified by Field, then the module will not compile. 
   
  @param   Record   Pointer to the field specified by Field within a data structure of type TYPE. 
  @param   TYPE     The name of the data structure type to return.  This data structure must 
                    contain the field specified by Field. 
  @param   Field    The name of the field in the data structure specified by TYPE to which Record points.
  
  @return  A pointer to the structure from one of it's elements.
  
**/
#define BASE_CR(Record, TYPE, Field)  ((TYPE *) ((CHAR8 *) (Record) - (CHAR8 *) &(((TYPE *) 0)->Field)))

/**
  Rounds a value up to the next boundary using a specified alignment.  

  This function rounds Value up to the next boundary using the specified Alignment.  
  This aligned value is returned.  

  @param   Value      The value to round up.
  @param   Alignment  The alignment boundary used to return the aligned value.
  
  @return  A value up to the next boundary.
  
**/
#define ALIGN_VALUE(Value, Alignment) ((Value) + (((Alignment) - (Value)) & ((Alignment) - 1)))

/**
  Adjust a pointer by adding the minimum offset required for it to be aligned on 
  a specified alignment boundary.  

  This function rounds the pointer specified by Pointer to the next alignment boundary 
  specified by Alignment. The pointer to the aligned address is returned.  

  @param   Pointer    The pointer to round up.
  @param   Alignment  The alignment boundary to use to return an aligned pointer.
  
  @return  Pointer to the aligned address.
  
**/
#define ALIGN_POINTER(Pointer, Alignment) ((VOID *) (ALIGN_VALUE ((UINTN)(Pointer), (Alignment))))

/**
  Rounds a value up to the next natural boundary for the current CPU.  
  This is 4-bytes for 32-bit CPUs and 8-bytes for 64-bit CPUs.   

  This function rounds the value specified by Value up to the next natural boundary for the 
  current CPU. This rounded value is returned.  

  @param   Value      The value to round up.

  @return  Rounded value specified by Value.
  
**/
#define ALIGN_VARIABLE(Value)  ALIGN_VALUE ((Value), sizeof (UINTN))
  

/**
  Return the maximum of two operands. 

  This macro returns the maximum of two operand specified by a and b.  
  Both a and b must be the same numerical types, signed or unsigned.

  @param   a        The first operand with any numerical type.
  @param   b        The second operand. Can be any numerical type as long as is 
                    the same type as a.
  
  @return  Maximum of two operands.
  
**/
#define MAX(a, b)                       \
  (((a) > (b)) ? (a) : (b))

/**
  Return the minimum of two operands. 

  This macro returns the minimal of two operand specified by a and b.  
  Both a and b must be the same numerical types, signed or unsigned.

  @param   a        The first operand with any numerical type.
  @param   b        The second operand. It should be the same any numerical type with a.
  
  @return  Minimum of two operands.
  
**/

#define MIN(a, b)                       \
  (((a) < (b)) ? (a) : (b))

//
// Status codes common to all execution phases
//
typedef UINTN RETURN_STATUS;

/**
  Produces a RETURN_STATUS code with the highest bit set. 

  @param  StatusCode    The status code value to convert into a warning code.  
                        StatusCode must be in the range 0x00000000..0x7FFFFFFF.

  @return The value specified by StatusCode with the highest bit set.

**/
#define ENCODE_ERROR(StatusCode)     ((RETURN_STATUS)(MAX_BIT | (StatusCode)))

/**
  Produces a RETURN_STATUS code with the highest bit clear. 

  @param  StatusCode    The status code value to convert into a warning code.  
                        StatusCode must be in the range 0x00000000..0x7FFFFFFF.

  @return The value specified by StatusCode with the highest bit clear.

**/
#define ENCODE_WARNING(StatusCode)   ((RETURN_STATUS)(StatusCode))

/**
  Returns TRUE if a specified RETURN_STATUS code is an error code. 

  This function returns TRUE if StatusCode has the high bit set.  Otherwise, FALSE is returned.  
  
  @param  StatusCode    The status code value to evaluate.

  @retval TRUE          The high bit of StatusCode is set.
  @retval FALSE         The high bit of StatusCode is clear.

**/
#define RETURN_ERROR(StatusCode)     (((INTN)(RETURN_STATUS)(StatusCode)) < 0)

///
/// The operation completed successfully.
///
#define RETURN_SUCCESS               0

///
/// The image failed to load.
///
#define RETURN_LOAD_ERROR            ENCODE_ERROR (1)

///
/// The parameter was incorrect.
///
#define RETURN_INVALID_PARAMETER     ENCODE_ERROR (2)

///
/// The operation is not supported.
///
#define RETURN_UNSUPPORTED           ENCODE_ERROR (3)

///
/// The buffer was not the proper size for the request.
///
#define RETURN_BAD_BUFFER_SIZE       ENCODE_ERROR (4)

///
/// The buffer was not large enough to hold the requested data.
/// The required buffer size is returned in the appropriate
/// parameter when this error occurs.
///
#define RETURN_BUFFER_TOO_SMALL      ENCODE_ERROR (5)

///
/// There is no data pending upon return.
///
#define RETURN_NOT_READY             ENCODE_ERROR (6)

///
/// The physical device reported an error while attempting the
/// operation.
///
#define RETURN_DEVICE_ERROR          ENCODE_ERROR (7)

///
/// The device can not be written to.
///
#define RETURN_WRITE_PROTECTED       ENCODE_ERROR (8)

///
/// The resource has run out.
///
#define RETURN_OUT_OF_RESOURCES      ENCODE_ERROR (9)

///
/// An inconsistency was detected on the file system causing the 
/// operation to fail.
///
#define RETURN_VOLUME_CORRUPTED      ENCODE_ERROR (10)

///
/// There is no more space on the file system.
///
#define RETURN_VOLUME_FULL           ENCODE_ERROR (11)

///
/// The device does not contain any medium to perform the 
/// operation.
///
#define RETURN_NO_MEDIA              ENCODE_ERROR (12)

///
/// The medium in the device has changed since the last
/// access.
///
#define RETURN_MEDIA_CHANGED         ENCODE_ERROR (13)

///
/// The item was not found.
///
#define RETURN_NOT_FOUND             ENCODE_ERROR (14)

///
/// Access was denied.
///
#define RETURN_ACCESS_DENIED         ENCODE_ERROR (15)

///
/// The server was not found or did not respond to the request.
///
#define RETURN_NO_RESPONSE           ENCODE_ERROR (16)

///
/// A mapping to the device does not exist.
///
#define RETURN_NO_MAPPING            ENCODE_ERROR (17)

///
/// A timeout time expired.
///
#define RETURN_TIMEOUT               ENCODE_ERROR (18)

///
/// The protocol has not been started.
///
#define RETURN_NOT_STARTED           ENCODE_ERROR (19)

///
/// The protocol has already been started.
///
#define RETURN_ALREADY_STARTED       ENCODE_ERROR (20)

///
/// The operation was aborted.
///
#define RETURN_ABORTED               ENCODE_ERROR (21)

///
/// An ICMP error occurred during the network operation.
///
#define RETURN_ICMP_ERROR            ENCODE_ERROR (22)

///
/// A TFTP error occurred during the network operation.
///
#define RETURN_TFTP_ERROR            ENCODE_ERROR (23)

///
/// A protocol error occurred during the network operation.
///
#define RETURN_PROTOCOL_ERROR        ENCODE_ERROR (24)

///
/// A function encountered an internal version that was
/// incompatible with a version requested by the caller.
///
#define RETURN_INCOMPATIBLE_VERSION  ENCODE_ERROR (25)

///
/// The function was not performed due to a security violation.
///
#define RETURN_SECURITY_VIOLATION    ENCODE_ERROR (26)

///
/// A CRC error was detected.
///
#define RETURN_CRC_ERROR             ENCODE_ERROR (27)

///
/// The beginning or end of media was reached.
///
#define RETURN_END_OF_MEDIA          ENCODE_ERROR (28)

///
/// The end of the file was reached.
///
#define RETURN_END_OF_FILE           ENCODE_ERROR (31)

///
/// The language specified was invalid.
///
#define RETURN_INVALID_LANGUAGE      ENCODE_ERROR (32)


///
/// The string contained one or more characters that
/// the device could not render and were skipped.
///
#define RETURN_WARN_UNKNOWN_GLYPH    ENCODE_WARNING (1)

///
/// The handle was closed, but the file was not deleted.
///
#define RETURN_WARN_DELETE_FAILURE   ENCODE_WARNING (2)

///
/// The handle was closed, but the data to the file was not
/// flushed properly.
///
#define RETURN_WARN_WRITE_FAILURE    ENCODE_WARNING (3)

///
/// The resulting buffer was too small, and the data was 
/// truncated to the buffer size.
///
#define RETURN_WARN_BUFFER_TOO_SMALL ENCODE_WARNING (4)

/**
  Returns a 16-bit signature built from 2 ASCII characters.
  
  This macro returns a 16-bit value built from the two ASCII characters specified 
  by A and B.
  
  @param  A    The first ASCII character.
  @param  B    The second ASCII character.

  @return A 16-bit value built from the two ASCII characters specified by A and B.

**/
#define SIGNATURE_16(A, B)        ((A) | (B << 8))

/**
  Returns a 32-bit signature built from 4 ASCII characters.
  
  This macro returns a 32-bit value built from the four ASCII characters specified 
  by A, B, C, and D.
  
  @param  A    The first ASCII character.
  @param  B    The second ASCII character.
  @param  C    The third ASCII character.
  @param  D    The fourth ASCII character.

  @return A 32-bit value built from the two ASCII characters specified by A, B,
          C and D.

**/
#define SIGNATURE_32(A, B, C, D)  (SIGNATURE_16 (A, B) | (SIGNATURE_16 (C, D) << 16))

/**
  Returns a 64-bit signature built from 8 ASCII characters.
  
  This macro returns a 64-bit value built from the eight ASCII characters specified 
  by A, B, C, D, E, F, G,and H.
  
  @param  A    The first ASCII character.
  @param  B    The second ASCII character.
  @param  C    The third ASCII character.
  @param  D    The fourth ASCII character.
  @param  E    The fifth ASCII character.
  @param  F    The sixth ASCII character.
  @param  G    The seventh ASCII character.
  @param  H    The eighth ASCII character.

  @return A 64-bit value built from the two ASCII characters specified by A, B,
          C, D, E, F, G and H.

**/
#define SIGNATURE_64(A, B, C, D, E, F, G, H) \
    (SIGNATURE_32 (A, B, C, D) | ((UINT64) (SIGNATURE_32 (E, F, G, H)) << 32))

#endif

