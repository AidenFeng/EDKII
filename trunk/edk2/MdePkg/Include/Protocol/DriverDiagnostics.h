/** @file
  EFI Driver Diagnostics Protocol

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __EFI_DRIVER_DIAGNOSTICS_H__
#define __EFI_DRIVER_DIAGNOSTICS_H__

//
// Global ID for the Driver Diagnostics Protocol as defined in EFI 1.10.
//
#define EFI_DRIVER_DIAGNOSTICS_PROTOCOL_GUID \
  { \
    0x0784924f, 0xe296, 0x11d4, {0x9a, 0x49, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

typedef struct _EFI_DRIVER_DIAGNOSTICS_PROTOCOL  EFI_DRIVER_DIAGNOSTICS_PROTOCOL;

typedef enum {
  EfiDriverDiagnosticTypeStandard     = 0,
  EfiDriverDiagnosticTypeExtended     = 1,
  EfiDriverDiagnosticTypeManufacturing= 2,
  EfiDriverDiagnosticTypeMaximum
} EFI_DRIVER_DIAGNOSTIC_TYPE;

/**
  Runs diagnostics on a controller.

  @param  This             A pointer to the EFI_DRIVER_DIAGNOSTICS_PROTOCOL instance.
  @param  ControllerHandle The handle of the controller to run diagnostics on.
  @param  ChildHandle      The handle of the child controller to run diagnostics on
                           This is an optional parameter that may be NULL.  It will
                           be NULL for device drivers.  It will also be NULL for a
                           bus drivers that wish to run diagnostics on the bus
                           controller.  It will not be NULL for a bus driver that
                           wishes to run diagnostics on one of its child controllers.
  @param  DiagnosticType   Indicates type of diagnostics to perform on the controller
                           specified by ControllerHandle and ChildHandle.   See
                           "Related Definitions" for the list of supported types.
  @param  Language         A pointer to a three character ISO 639-2 language
                           identifier.  This is the language in which the optional
                           error message should be returned in Buffer, and it must
                           match one of the languages specified in SupportedLanguages.
                           The number of languages supported by a driver is up to
                           the driver writer.
  @param  ErrorType        A GUID that defines the format of the data returned in Buffer.
  @param  BufferSize       The size, in bytes, of the data returned in Buffer.
  @param  Buffer           A buffer that contains a Null-terminated Unicode string
                           plus some additional data whose format is defined by
                           ErrorType.  Buffer is allocated by this function with
                           AllocatePool(), and it is the caller's responsibility
                           to free it with a call to FreePool().

  @retval EFI_SUCCESS           The controller specified by ControllerHandle and
                                ChildHandle passed the diagnostic.
  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ErrorType is NULL.
  @retval EFI_INVALID_PARAMETER BufferType is NULL.
  @retval EFI_INVALID_PARAMETER Buffer is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                running diagnostics for the controller specified
                                by ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support the
                                type of diagnostic specified by DiagnosticType.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support the
                                language specified by Language.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to complete
                                the diagnostics.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to return
                                the status information in ErrorType, BufferSize,
                                and Buffer.
  @retval EFI_DEVICE_ERROR      The controller specified by ControllerHandle and
                                ChildHandle did not pass the diagnostic.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DRIVER_DIAGNOSTICS_RUN_DIAGNOSTICS) (
  IN EFI_DRIVER_DIAGNOSTICS_PROTOCOL                        *This,
  IN  EFI_HANDLE                                            ControllerHandle,
  IN  EFI_HANDLE                                            ChildHandle  OPTIONAL,
  IN  EFI_DRIVER_DIAGNOSTIC_TYPE                            DiagnosticType,
  IN  CHAR8                                                 *Language,
  OUT EFI_GUID                                              **ErrorType,
  OUT UINTN                                                 *BufferSize,
  OUT CHAR16                                                **Buffer
  );


//
//

/**
  Interface structure for the Driver Diagnostics Protocol.

  @par Protocol Description:
  Used to perform diagnostics on a controller that an EFI Driver is managing.

  @param RunDiagnostics      Runs diagnostics on a controller.
  @param SupportedLanguages  A Null-terminated ASCII string that contains one or more
                             ISO 639-2 language codes.  This is the list of language 
                             codes that this protocol supports.

**/
struct _EFI_DRIVER_DIAGNOSTICS_PROTOCOL {
  EFI_DRIVER_DIAGNOSTICS_RUN_DIAGNOSTICS  RunDiagnostics;
  CHAR8                                   *SupportedLanguages;
};

extern EFI_GUID gEfiDriverDiagnosticsProtocolGuid;

#endif
