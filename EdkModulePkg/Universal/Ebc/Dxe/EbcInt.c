/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:  

  EbcInt.c
  
Abstract:

  Top level module for the EBC virtual machine implementation.
  Provides auxilliary support routines for the VM. That is, routines
  that are not particularly related to VM execution of EBC instructions.
  
--*/

#include "EbcInt.h"
#include "EbcExecute.h"

//
// We'll keep track of all thunks we create in a linked list. Each
// thunk is tied to an image handle, so we have a linked list of
// image handles, with each having a linked list of thunks allocated
// to that image handle.
//
typedef struct _EBC_THUNK_LIST {
  VOID                    *ThunkBuffer;
  struct _EBC_THUNK_LIST  *Next;
} EBC_THUNK_LIST;

typedef struct _EBC_IMAGE_LIST {
  struct _EBC_IMAGE_LIST  *Next;
  EFI_HANDLE              ImageHandle;
  EBC_THUNK_LIST          *ThunkList;
} EBC_IMAGE_LIST;

//
// Function prototypes
//
EFI_STATUS
EFIAPI
InitializeEbcDriver (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

STATIC
EFI_STATUS
EFIAPI
EbcUnloadImage (
  IN EFI_EBC_PROTOCOL     *This,
  IN EFI_HANDLE           ImageHandle
  );

STATIC
EFI_STATUS
EFIAPI
EbcCreateThunk (
  IN EFI_EBC_PROTOCOL     *This,
  IN EFI_HANDLE           ImageHandle,
  IN VOID                 *EbcEntryPoint,
  OUT VOID                **Thunk
  );

STATIC
EFI_STATUS
EFIAPI
EbcGetVersion (
  IN EFI_EBC_PROTOCOL     *This,
  IN OUT UINT64           *Version
  );

//
// These two functions and the  GUID are used to produce an EBC test protocol.
// This functionality is definitely not required for execution.
//
STATIC
EFI_STATUS
InitEbcVmTestProtocol (
  IN EFI_HANDLE     *Handle
  );

STATIC
EFI_STATUS
EbcVmTestUnsupported (
  VOID
  );

STATIC
EFI_STATUS
EFIAPI
EbcRegisterICacheFlush (
  IN EFI_EBC_PROTOCOL               *This,
  IN EBC_ICACHE_FLUSH               Flush
  );

STATIC
EFI_STATUS
EFIAPI
EbcDebugGetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL     *This,
  OUT UINTN                         *MaxProcessorIndex
  );

STATIC
EFI_STATUS
EFIAPI
EbcDebugRegisterPeriodicCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL     *This,
  IN UINTN                          ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK          PeriodicCallback
  );

STATIC
EFI_STATUS
EFIAPI
EbcDebugRegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL     *This,
  IN UINTN                          ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK         ExceptionCallback,
  IN EFI_EXCEPTION_TYPE             ExceptionType
  );

STATIC
EFI_STATUS
EFIAPI
EbcDebugInvalidateInstructionCache (
  IN EFI_DEBUG_SUPPORT_PROTOCOL     *This,
  IN UINTN                          ProcessorIndex,
  IN VOID                           *Start,
  IN UINT64                         Length
  );

//
// We have one linked list of image handles for the whole world. Since
// there should only be one interpreter, make them global. They must
// also be global since the execution of an EBC image does not provide
// a This pointer.
//
static EBC_IMAGE_LIST         *mEbcImageList = NULL;

//
// Callback function to flush the icache after thunk creation
//
static EBC_ICACHE_FLUSH       mEbcICacheFlush;

//
// These get set via calls by the debug agent
//
static EFI_PERIODIC_CALLBACK  mDebugPeriodicCallback                            = NULL;
static EFI_EXCEPTION_CALLBACK mDebugExceptionCallback[MAX_EBC_EXCEPTION + 1] = {NULL};
static EFI_GUID               mEfiEbcVmTestProtocolGuid = EFI_EBC_VM_TEST_PROTOCOL_GUID;

EFI_STATUS
EFIAPI
InitializeEbcDriver (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description: 

  Initializes the VM EFI interface.  Allocates memory for the VM interface 
  and registers the VM protocol.

Arguments:  

  ImageHandle - EFI image handle.
  SystemTable - Pointer to the EFI system table.

Returns:  
  Standard EFI status code.

--*/
{
  EFI_EBC_PROTOCOL            *EbcProtocol;
  EFI_EBC_PROTOCOL            *OldEbcProtocol;
  EFI_STATUS                  Status;
  EFI_DEBUG_SUPPORT_PROTOCOL  *EbcDebugProtocol;
  EFI_HANDLE                  *HandleBuffer;
  UINTN                       NumHandles;
  UINTN                       Index;
  BOOLEAN                     Installed;

  //
  // Allocate memory for our protocol. Then fill in the blanks.
  //
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EFI_EBC_PROTOCOL),
                  (VOID **) &EbcProtocol
                  );
  if (Status != EFI_SUCCESS) {
    return EFI_OUT_OF_RESOURCES;
  }

  EbcProtocol->CreateThunk          = EbcCreateThunk;
  EbcProtocol->UnloadImage          = EbcUnloadImage;
  EbcProtocol->RegisterICacheFlush  = EbcRegisterICacheFlush;
  EbcProtocol->GetVersion           = EbcGetVersion;
  mEbcICacheFlush                   = NULL;

  //
  // Find any already-installed EBC protocols and uninstall them
  //
  Installed     = FALSE;
  HandleBuffer  = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiEbcProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );
  if (Status == EFI_SUCCESS) {
    //
    // Loop through the handles
    //
    for (Index = 0; Index < NumHandles; Index++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiEbcProtocolGuid,
                      (VOID **) &OldEbcProtocol
                      );
      if (Status == EFI_SUCCESS) {
        if (gBS->ReinstallProtocolInterface (
                  HandleBuffer[Index],
                  &gEfiEbcProtocolGuid,
                  OldEbcProtocol,
                  EbcProtocol
                  ) == EFI_SUCCESS) {
          Installed = TRUE;
        }
      }
    }
  }

  if (HandleBuffer != NULL) {
    gBS->FreePool (HandleBuffer);
    HandleBuffer = NULL;
  }
  //
  // Add the protocol so someone can locate us if we haven't already.
  //
  if (!Installed) {
    Status = gBS->InstallProtocolInterface (
                    &ImageHandle,
                    &gEfiEbcProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    EbcProtocol
                    );
    if (EFI_ERROR (Status)) {
      gBS->FreePool (EbcProtocol);
      return Status;
    }
  }
  //
  // Allocate memory for our debug protocol. Then fill in the blanks.
  //
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EFI_DEBUG_SUPPORT_PROTOCOL),
                  (VOID **) &EbcDebugProtocol
                  );
  if (Status != EFI_SUCCESS) {
    return EFI_OUT_OF_RESOURCES;
  }

  EbcDebugProtocol->Isa                         = IsaEbc;
  EbcDebugProtocol->GetMaximumProcessorIndex    = EbcDebugGetMaximumProcessorIndex;
  EbcDebugProtocol->RegisterPeriodicCallback    = EbcDebugRegisterPeriodicCallback;
  EbcDebugProtocol->RegisterExceptionCallback   = EbcDebugRegisterExceptionCallback;
  EbcDebugProtocol->InvalidateInstructionCache  = EbcDebugInvalidateInstructionCache;

  //
  // Add the protocol so the debug agent can find us
  //
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiDebugSupportProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  EbcDebugProtocol
                  );
  //
  // This is recoverable, so free the memory and continue.
  //
  if (EFI_ERROR (Status)) {
    gBS->FreePool (EbcDebugProtocol);
  }
  //
  // Produce a VM test interface protocol. Not required for execution.
  //
  DEBUG_CODE_BEGIN ();
    InitEbcVmTestProtocol (&ImageHandle);
  DEBUG_CODE_END ();

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
EbcCreateThunk (
  IN EFI_EBC_PROTOCOL   *This,
  IN EFI_HANDLE         ImageHandle,
  IN VOID               *EbcEntryPoint,
  OUT VOID              **Thunk
  )
/*++

Routine Description:
  
  This is the top-level routine plugged into the EBC protocol. Since thunks
  are very processor-specific, from here we dispatch directly to the very 
  processor-specific routine EbcCreateThunks().

Arguments:

  This          - protocol instance pointer
  ImageHandle   - handle to the image. The EBC interpreter may use this to keep
                  track of any resource allocations performed in loading and
                  executing the image.
  EbcEntryPoint - the entry point for the image (as defined in the file header)
  Thunk         - pointer to thunk pointer where the address of the created
                  thunk is returned.

Returns:

  EFI_STATUS

--*/
{
  EFI_STATUS  Status;

  Status = EbcCreateThunks (
            ImageHandle,
            EbcEntryPoint,
            Thunk,
            FLAG_THUNK_ENTRY_POINT
            );
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
EbcDebugGetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL          *This,
  OUT UINTN                              *MaxProcessorIndex
  )
/*++

Routine Description:
  
  This EBC debugger protocol service is called by the debug agent

Arguments:

  This              - pointer to the caller's debug support protocol interface
  MaxProcessorIndex - pointer to a caller allocated UINTN in which the maximum
                      processor index is returned.
                                               
Returns:

  Standard EFI_STATUS

--*/
{
  *MaxProcessorIndex = 0;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
EbcDebugRegisterPeriodicCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN UINTN                       ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK       PeriodicCallback
  )
/*++

Routine Description:
  
  This protocol service is called by the debug agent to register a function
  for us to call on a periodic basis.
  

Arguments:

  This              - pointer to the caller's debug support protocol interface
  PeriodicCallback  - pointer to the function to call periodically

Returns:

  Always EFI_SUCCESS

--*/
{
  if ((mDebugPeriodicCallback == NULL) && (PeriodicCallback == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  if ((mDebugPeriodicCallback != NULL) && (PeriodicCallback != NULL)) {
    return EFI_ALREADY_STARTED;
  }
	
  mDebugPeriodicCallback = PeriodicCallback;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
EbcDebugRegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN UINTN                       ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK      ExceptionCallback,
  IN EFI_EXCEPTION_TYPE          ExceptionType
  )
/*++

Routine Description:
  
  This protocol service is called by the debug agent to register a function
  for us to call when we detect an exception.
  

Arguments:

  This              - pointer to the caller's debug support protocol interface
  PeriodicCallback  - pointer to the function to call periodically

Returns:

  Always EFI_SUCCESS

--*/
{
  if ((ExceptionType < 0) || (ExceptionType > MAX_EBC_EXCEPTION)) {
    return EFI_INVALID_PARAMETER;
  }
  if ((mDebugExceptionCallback[ExceptionType] == NULL) && (ExceptionCallback == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  if ((mDebugExceptionCallback[ExceptionType] != NULL) && (ExceptionCallback != NULL)) {
    return EFI_ALREADY_STARTED;
  }
  mDebugExceptionCallback[ExceptionType] = ExceptionCallback;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
EbcDebugInvalidateInstructionCache (
  IN EFI_DEBUG_SUPPORT_PROTOCOL          *This,
  IN UINTN                               ProcessorIndex,
  IN VOID                                *Start,
  IN UINT64                              Length
  )
/*++

Routine Description:
  
  This EBC debugger protocol service is called by the debug agent.  Required
  for DebugSupport compliance but is only stubbed out for EBC.

Arguments:
                                               
Returns:

  EFI_SUCCESS

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EbcDebugSignalException (
  IN EFI_EXCEPTION_TYPE                   ExceptionType,
  IN EXCEPTION_FLAGS                      ExceptionFlags,
  IN VM_CONTEXT                           *VmPtr
  )
/*++

Routine Description:

  The VM interpreter calls this function when an exception is detected.
  
Arguments:

  VmPtr - pointer to a VM context for passing info to the EFI debugger.

Returns:

  EFI_SUCCESS if it returns at all
  
--*/
{
  EFI_SYSTEM_CONTEXT_EBC  EbcContext;
  EFI_SYSTEM_CONTEXT      SystemContext;
  EFI_STATUS_CODE_VALUE   StatusCodeValue;
  BOOLEAN                 Report;
  //
  // Save the exception in the context passed in
  //
  VmPtr->ExceptionFlags |= ExceptionFlags;
  VmPtr->LastException = ExceptionType;
  //
  // If it's a fatal exception, then flag it in the VM context in case an
  // attached debugger tries to return from it.
  //
  if (ExceptionFlags & EXCEPTION_FLAG_FATAL) {
    VmPtr->StopFlags |= STOPFLAG_APP_DONE;
  }
  //
  // Initialize the context structure
  //
  EbcContext.R0                   = VmPtr->R[0];
  EbcContext.R1                   = VmPtr->R[1];
  EbcContext.R2                   = VmPtr->R[2];
  EbcContext.R3                   = VmPtr->R[3];
  EbcContext.R4                   = VmPtr->R[4];
  EbcContext.R5                   = VmPtr->R[5];
  EbcContext.R6                   = VmPtr->R[6];
  EbcContext.R7                   = VmPtr->R[7];
  EbcContext.Ip                   = (UINT64) (UINTN) VmPtr->Ip;
  EbcContext.Flags                = VmPtr->Flags;
  EbcContext.ControlFlags         = 0;
  SystemContext.SystemContextEbc  = &EbcContext;
  //
  // If someone's registered for exception callbacks, then call them.
  // Otherwise report the status code via the status code API
  //
  if ((ExceptionType >= 0) && (ExceptionType <= MAX_EBC_EXCEPTION) &&
      (mDebugExceptionCallback[ExceptionType] != NULL)) {
    mDebugExceptionCallback[ExceptionType] (ExceptionType, SystemContext);
  }
  //
  // Determine if we should report the exception. We report all of them by default,
  // but if a debugger is attached don't report the breakpoint, debug, and step exceptions.
  // Note that EXCEPT_EBC_OVERFLOW is never reported by this VM implementation, so is
  // not included in the switch statement.
  //
  Report = TRUE;
  switch (ExceptionType) {
  case EXCEPT_EBC_UNDEFINED:
    StatusCodeValue = EFI_SOFTWARE_EBC_EXCEPTION | EFI_SW_EC_EBC_UNDEFINED;
    break;

  case EXCEPT_EBC_DIVIDE_ERROR:
    StatusCodeValue = EFI_SOFTWARE_EBC_EXCEPTION | EFI_SW_EC_EBC_DIVIDE_ERROR;
    break;

  case EXCEPT_EBC_DEBUG:
    StatusCodeValue = EFI_SOFTWARE_EBC_EXCEPTION | EFI_SW_EC_EBC_DEBUG;
    Report          = (BOOLEAN) ((mDebugExceptionCallback[ExceptionType] == NULL) ? TRUE : FALSE);
    break;

  case EXCEPT_EBC_BREAKPOINT:
    StatusCodeValue = EFI_SOFTWARE_EBC_EXCEPTION | EFI_SW_EC_EBC_BREAKPOINT;
    Report          = (BOOLEAN) ((mDebugExceptionCallback[ExceptionType] == NULL) ? TRUE : FALSE);
    break;

  case EXCEPT_EBC_INVALID_OPCODE:
    StatusCodeValue = EFI_SOFTWARE_EBC_EXCEPTION | EFI_SW_EC_EBC_INVALID_OPCODE;
    break;

  case EXCEPT_EBC_STACK_FAULT:
    StatusCodeValue = EFI_SOFTWARE_EBC_EXCEPTION | EFI_SW_EC_EBC_STACK_FAULT;
    break;

  case EXCEPT_EBC_ALIGNMENT_CHECK:
    StatusCodeValue = EFI_SOFTWARE_EBC_EXCEPTION | EFI_SW_EC_EBC_ALIGNMENT_CHECK;
    break;

  case EXCEPT_EBC_INSTRUCTION_ENCODING:
    StatusCodeValue = EFI_SOFTWARE_EBC_EXCEPTION | EFI_SW_EC_EBC_INSTRUCTION_ENCODING;
    break;

  case EXCEPT_EBC_BAD_BREAK:
    StatusCodeValue = EFI_SOFTWARE_EBC_EXCEPTION | EFI_SW_EC_EBC_BAD_BREAK;
    break;

  case EXCEPT_EBC_STEP:
    StatusCodeValue = EFI_SOFTWARE_EBC_EXCEPTION | EFI_SW_EC_EBC_STEP;
    Report          = (BOOLEAN) ((mDebugExceptionCallback[ExceptionType] == NULL) ? TRUE : FALSE);
    break;

  default:
    StatusCodeValue = EFI_SOFTWARE_EBC_EXCEPTION | EFI_SW_EC_NON_SPECIFIC;
    break;
  }
  //
  // If we determined that we should report the condition, then do so now.
  //
  if (Report) {
    REPORT_STATUS_CODE (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED, StatusCodeValue);
  }

  switch (ExceptionType) {
  //
  // If ReportStatusCode returned, then for most exceptions we do an assert. The
  // ExceptionType++ is done simply to force the ASSERT() condition to be met.
  // For breakpoints, assume a debugger did not insert a software breakpoint
  // and skip the instruction.
  //
  case EXCEPT_EBC_BREAKPOINT:
    VmPtr->Ip += 2;
    break;

  case EXCEPT_EBC_STEP:
    break;

  case EXCEPT_EBC_UNDEFINED:
    ExceptionType++;
    ASSERT (ExceptionType == EXCEPT_EBC_UNDEFINED);
    break;

  case EXCEPT_EBC_DIVIDE_ERROR:
    ExceptionType++;
    ASSERT (ExceptionType == EXCEPT_EBC_DIVIDE_ERROR);
    break;

  case EXCEPT_EBC_DEBUG:
    ExceptionType++;
    ASSERT (ExceptionType == EXCEPT_EBC_DEBUG);
    break;

  case EXCEPT_EBC_INVALID_OPCODE:
    ExceptionType++;
    ASSERT (ExceptionType == EXCEPT_EBC_INVALID_OPCODE);
    break;

  case EXCEPT_EBC_STACK_FAULT:
    ExceptionType++;
    ASSERT (ExceptionType == EXCEPT_EBC_STACK_FAULT);
    break;

  case EXCEPT_EBC_ALIGNMENT_CHECK:
    ExceptionType++;
    ASSERT (ExceptionType == EXCEPT_EBC_ALIGNMENT_CHECK);
    break;

  case EXCEPT_EBC_INSTRUCTION_ENCODING:
    ExceptionType++;
    ASSERT (ExceptionType == EXCEPT_EBC_INSTRUCTION_ENCODING);
    break;

  case EXCEPT_EBC_BAD_BREAK:
    ExceptionType++;
    ASSERT (ExceptionType == EXCEPT_EBC_BAD_BREAK);
    break;

  default:
    //
    // Unknown
    //
    ASSERT (0);
    break;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EbcDebugPeriodic (
  IN VM_CONTEXT *VmPtr
  )
/*++

Routine Description:

  The VM interpreter calls this function on a periodic basis to support
  the EFI debug support protocol.
  
Arguments:

  VmPtr - pointer to a VM context for passing info to the debugger.

Returns:

  Standard EFI status.
  
--*/
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
EbcUnloadImage (
  IN EFI_EBC_PROTOCOL   *This,
  IN EFI_HANDLE         ImageHandle
  )
/*++

Routine Description:
  
  This routine is called by the core when an image is being unloaded from 
  memory. Basically we now have the opportunity to do any necessary cleanup.
  Typically this will include freeing any memory allocated for thunk-creation.

Arguments:

  This          - protocol instance pointer
  ImageHandle   - handle to the image being unloaded.

Returns:

  EFI_INVALID_PARAMETER  - the ImageHandle passed in was not found in
                           the internal list of EBC image handles.
  EFI_STATUS             - completed successfully

--*/
{
  EBC_THUNK_LIST  *ThunkList;
  EBC_THUNK_LIST  *NextThunkList;
  EBC_IMAGE_LIST  *ImageList;
  EBC_IMAGE_LIST  *PrevImageList;
  //
  // First go through our list of known image handles and see if we've already
  // created an image list element for this image handle.
  //
  PrevImageList = NULL;
  for (ImageList = mEbcImageList; ImageList != NULL; ImageList = ImageList->Next) {
    if (ImageList->ImageHandle == ImageHandle) {
      break;
    }
    //
    // Save the previous so we can connect the lists when we remove this one
    //
    PrevImageList = ImageList;
  }

  if (ImageList == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Free up all the thunk buffers and thunks list elements for this image
  // handle.
  //
  ThunkList = ImageList->ThunkList;
  while (ThunkList != NULL) {
    NextThunkList = ThunkList->Next;
    gBS->FreePool (ThunkList->ThunkBuffer);
    gBS->FreePool (ThunkList);
    ThunkList = NextThunkList;
  }
  //
  // Now remove this image list element from the chain
  //
  if (PrevImageList == NULL) {
    //
    // Remove from head
    //
    mEbcImageList = ImageList->Next;
  } else {
    PrevImageList->Next = ImageList->Next;
  }
  //
  // Now free up the image list element
  //
  gBS->FreePool (ImageList);
  return EFI_SUCCESS;
}

EFI_STATUS
EbcAddImageThunk (
  IN EFI_HANDLE      ImageHandle,
  IN VOID            *ThunkBuffer,
  IN UINT32          ThunkSize
  )
/*++

Routine Description:
  
  Add a thunk to our list of thunks for a given image handle. 
  Also flush the instruction cache since we've written thunk code
  to memory that will be executed eventually.

Arguments:

  ImageHandle - the image handle to which the thunk is tied
  ThunkBuffer - the buffer we've created/allocated
  ThunkSize    - the size of the thunk memory allocated

Returns:
 
  EFI_OUT_OF_RESOURCES    - memory allocation failed
  EFI_SUCCESS             - successful completion

--*/
{
  EBC_THUNK_LIST  *ThunkList;
  EBC_IMAGE_LIST  *ImageList;
  EFI_STATUS      Status;

  //
  // It so far so good, then flush the instruction cache
  //
  if (mEbcICacheFlush != NULL) {
    Status = mEbcICacheFlush ((EFI_PHYSICAL_ADDRESS) (UINTN) ThunkBuffer, ThunkSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  //
  // Go through our list of known image handles and see if we've already
  // created a image list element for this image handle.
  //
  for (ImageList = mEbcImageList; ImageList != NULL; ImageList = ImageList->Next) {
    if (ImageList->ImageHandle == ImageHandle) {
      break;
    }
  }

  if (ImageList == NULL) {
    //
    // Allocate a new one
    //
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    sizeof (EBC_IMAGE_LIST),
                    (VOID **) &ImageList
                    );
    if (Status != EFI_SUCCESS) {
      return EFI_OUT_OF_RESOURCES;
    }

    ImageList->ThunkList    = NULL;
    ImageList->ImageHandle  = ImageHandle;
    ImageList->Next         = mEbcImageList;
    mEbcImageList           = ImageList;
  }
  //
  // Ok, now create a new thunk element to add to the list
  //
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EBC_THUNK_LIST),
                  (VOID **) &ThunkList
                  );
  if (Status != EFI_SUCCESS) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Add it to the head of the list
  //
  ThunkList->Next         = ImageList->ThunkList;
  ThunkList->ThunkBuffer  = ThunkBuffer;
  ImageList->ThunkList    = ThunkList;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
EbcRegisterICacheFlush (
  IN EFI_EBC_PROTOCOL   *This,
  IN EBC_ICACHE_FLUSH   Flush
  )
{
  mEbcICacheFlush = Flush;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
EbcGetVersion (
  IN EFI_EBC_PROTOCOL   *This,
  IN OUT UINT64         *Version
  )
{
  if (Version == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Version = GetVmVersion ();
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InitEbcVmTestProtocol (
  IN EFI_HANDLE     *IHandle
  )
/*++

Routine Description:
  
  Produce an EBC VM test protocol that can be used for regression tests.

Arguments:

  IHandle - handle on which to install the protocol.

Returns:

  EFI_OUT_OF_RESOURCES  - memory allocation failed
  EFI_SUCCESS           - successful completion

--*/
{
  EFI_HANDLE Handle;
  EFI_STATUS Status;
  EFI_EBC_VM_TEST_PROTOCOL *EbcVmTestProtocol;

  //
  // Allocate memory for the protocol, then fill in the fields
  //
  Status = gBS->AllocatePool (EfiBootServicesData, sizeof (EFI_EBC_VM_TEST_PROTOCOL), (VOID **) &EbcVmTestProtocol);
  if (Status != EFI_SUCCESS) {
    return EFI_OUT_OF_RESOURCES;
  }
  EbcVmTestProtocol->Execute      = (EBC_VM_TEST_EXECUTE) EbcExecuteInstructions;

  DEBUG_CODE_BEGIN ();
    EbcVmTestProtocol->Assemble     = (EBC_VM_TEST_ASM) EbcVmTestUnsupported;
    EbcVmTestProtocol->Disassemble  = (EBC_VM_TEST_DASM) EbcVmTestUnsupported;
  DEBUG_CODE_END ();

  //
  // Publish the protocol
  //
  Handle  = NULL;
  Status  = gBS->InstallProtocolInterface (&Handle, &mEfiEbcVmTestProtocolGuid, EFI_NATIVE_INTERFACE, EbcVmTestProtocol);
  if (EFI_ERROR (Status)) {
    gBS->FreePool (EbcVmTestProtocol);
  }
  return Status;
}
STATIC
EFI_STATUS
EbcVmTestUnsupported ()
{
  return EFI_UNSUPPORTED;
}

