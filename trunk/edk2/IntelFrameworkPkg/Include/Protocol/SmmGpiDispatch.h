/** @file
  This file declares Smm Gpi Smi Child Protocol.
  
  The EFI_SMM_GPI_DISPATCH_PROTOCOL is defined in Framework of EFI SMM Core Interface Spec
  Version 0.9. It provides the ability to install child handlers for the given event types.
  Several inputs can be enabled. This purpose of this interface is to generate an
  SMI in response to any of these inputs having a true value provided.
  
  Copyright (c) 2007 - 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMM_GPI_DISPATCH_H_
#define _SMM_GPI_DISPATCH_H_


//
// Global ID for the GPI SMI Protocol
//
#define EFI_SMM_GPI_DISPATCH_PROTOCOL_GUID \
  { \
    0xe0744b81, 0x9513, 0x49cd, {0x8c, 0xea, 0xe9, 0x24, 0x5e, 0x70, 0x39, 0xda } \
  }

typedef struct _EFI_SMM_GPI_DISPATCH_PROTOCOL  EFI_SMM_GPI_DISPATCH_PROTOCOL;

//
// Related Definitions
//

//
// GpiMask is a bit mask of 32 possible general purpose inputs that can generate a
// a SMI.  Bit 0 corresponds to logical GPI[0], 1 corresponds to logical GPI[1], etc.
//
// The logical GPI index to physical pin on device is described by the GPI device name
// found on the same handle as the GpiSmi child dispatch protocol.  The GPI device name
// is defined as protocol with a GUID name and NULL protocol pointer.
//
typedef struct {
  UINTN GpiNum;
} EFI_SMM_GPI_DISPATCH_CONTEXT;

//
// Member functions
//

/**
  Dispatch function for a GPI SMI handler.

  @param  DispatchHandle        Handle of this dispatch function.
  @param  DispatchContext       Pointer to the dispatch function's context.
                                The DispatchContext fields are filled in by the dispatching driver prior to
                                invoking this dispatch function.
**/
typedef
VOID
(EFIAPI *EFI_SMM_GPI_DISPATCH)(
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_GPI_DISPATCH_CONTEXT  *DispatchContext
  );

/**
  Register a child SMI source dispatch function with a parent SMM driver

  @param  This                  Pointer to the EFI_SMM_GPI_DISPATCH_PROTOCOL instance.
  @param  DispatchFunction      Function to install.
  @param  DispatchContext       Pointer to the dispatch function's context.
                                The caller fills this context in before calling
                                the register function to indicate to the register
                                function the GPI(s) for which the dispatch function
                                should be invoked.
  @param  DispatchHandle        Handle generated by the dispatcher to track the 
                                function instance.

  @retval EFI_SUCCESS           The dispatch function has been successfully
                                registered and the SMI source has been enabled.
  @retval EFI_DEVICE_ERROR      The driver was unable to enable the SMI source.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory (system or SMM) to manage this
                                child.
  @retval EFI_INVALID_PARAMETER DispatchContext is invalid. The GPI input value
                                is not within valid range.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_GPI_REGISTER)(
  IN EFI_SMM_GPI_DISPATCH_PROTOCOL           *This,
  IN EFI_SMM_GPI_DISPATCH                    DispatchFunction,
  IN EFI_SMM_GPI_DISPATCH_CONTEXT            *DispatchContext,
  OUT EFI_HANDLE                             *DispatchHandle
  );

/**
  Unregisters a General Purpose Input (GPI) service.

  @param  This                  Pointer to the EFI_SMM_GPI_DISPATCH_PROTOCOL instance.
  @param  DispatchHandle        Handle of the service to remove.

  @retval EFI_SUCCESS           The dispatch function has been successfully
                                unregistered and the SMI source has been disabled
                                if there are no other registered child dispatch
                                functions for this SMI source.
  @retval EFI_INVALID_PARAMETER DispatchHandle is invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_GPI_UNREGISTER)(
  IN EFI_SMM_GPI_DISPATCH_PROTOCOL           *This,
  IN EFI_HANDLE                              DispatchHandle
  );

//
// Interface structure for the SMM GPI SMI Dispatch Protocol
//
struct _EFI_SMM_GPI_DISPATCH_PROTOCOL {
  EFI_SMM_GPI_REGISTER    Register;
  EFI_SMM_GPI_UNREGISTER  UnRegister;

  ///
  /// Denotes the maximum value of inputs that can have handlers attached.
  ///
  UINTN                   NumSupportedGpis;
};

extern EFI_GUID gEfiSmmGpiDispatchProtocolGuid;

#endif

