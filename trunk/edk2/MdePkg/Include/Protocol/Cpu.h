/** @file
  CPU Architectural Protocol as defined in DXE CIS

  This code abstracts the DXE core from processor implementation details.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  Cpu.h

  @par Revision Reference:
  Version 0.91B.

**/

#ifndef __ARCH_PROTOCOL_CPU_H__
#define __ARCH_PROTOCOL_CPU_H__

#include <Protocol/DebugSupport.h>

#define EFI_CPU_ARCH_PROTOCOL_GUID \
  { 0x26baccb1, 0x6f42, 0x11d4, {0xbc, 0xe7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } }

typedef struct _EFI_CPU_ARCH_PROTOCOL   EFI_CPU_ARCH_PROTOCOL;

typedef enum {
  EfiCpuFlushTypeWriteBackInvalidate,
  EfiCpuFlushTypeWriteBack,
  EfiCpuFlushTypeInvalidate,
  EfiCpuMaxFlushType
} EFI_CPU_FLUSH_TYPE;

typedef enum {
  EfiCpuInit,
  EfiCpuMaxInitType
} EFI_CPU_INIT_TYPE;

/**
  EFI_CPU_INTERRUPT_HANDLER that is called when a processor interrupt occurs.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.

  @return None

**/
typedef
VOID
(EFIAPI *EFI_CPU_INTERRUPT_HANDLER) (
  IN CONST  EFI_EXCEPTION_TYPE  InterruptType,
  IN CONST  EFI_SYSTEM_CONTEXT  SystemContext
  );

/**
  This function flushes the range of addresses from Start to Start+Length 
  from the processor's data cache. If Start is not aligned to a cache line 
  boundary, then the bytes before Start to the preceding cache line boundary 
  are also flushed. If Start+Length is not aligned to a cache line boundary, 
  then the bytes past Start+Length to the end of the next cache line boundary 
  are also flushed. The FlushType of EfiCpuFlushTypeWriteBackInvalidate must be 
  supported. If the data cache is fully coherent with all DMA operations, then 
  this function can just return EFI_SUCCESS. If the processor does not support 
  flushing a range of the data cache, then the entire data cache can be flushed.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  Start            The beginning physical address to flush from the processor's data
                           cache.
  @param  Length           The number of bytes to flush from the processor's data cache. This
                           function may flush more bytes than Length specifies depending upon
                           the granularity of the flush operation that the processor supports.
  @param  FlushType        Specifies the type of flush operation to perform.

  @retval EFI_SUCCESS           The address range from Start to Start+Length was flushed from
                                the processor's data cache.
  @retval EFI_UNSUPPORTEDT      The processor does not support the cache flush type specified
                                by FlushType.
  @retval EFI_DEVICE_ERROR      The address range from Start to Start+Length could not be flushed
                                from the processor's data cache.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CPU_FLUSH_DATA_CACHE) (
  IN EFI_CPU_ARCH_PROTOCOL              *This,
  IN EFI_PHYSICAL_ADDRESS               Start,
  IN UINT64                             Length,
  IN EFI_CPU_FLUSH_TYPE                 FlushType
  );


/**
  This function enables interrupt processing by the processor. 

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS           Interrupts are enabled on the processor.
  @retval EFI_DEVICE_ERROR      Interrupts could not be enabled on the processor.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CPU_ENABLE_INTERRUPT) (
  IN EFI_CPU_ARCH_PROTOCOL              *This
  );


/**
  This function disables interrupt processing by the processor.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS           Interrupts are disabled on the processor.
  @retval EFI_DEVICE_ERROR      Interrupts could not be disabled on the processor.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CPU_DISABLE_INTERRUPT) (
  IN EFI_CPU_ARCH_PROTOCOL              *This
  );


/**
  This function retrieves the processor's current interrupt state a returns it in 
  State. If interrupts are currently enabled, then TRUE is returned. If interrupts 
  are currently disabled, then FALSE is returned.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  State            A pointer to the processor's current interrupt state. Set to TRUE if
                           interrupts are enabled and FALSE if interrupts are disabled.

  @retval EFI_SUCCESS           The processor's current interrupt state was returned in State.
  @retval EFI_INVALID_PARAMETER State is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CPU_GET_INTERRUPT_STATE) (
  IN EFI_CPU_ARCH_PROTOCOL              *This,
  OUT BOOLEAN                           *State
  );


/**
  This function generates an INIT on the processor. If this function succeeds, then the
  processor will be reset, and control will not be returned to the caller. If InitType is 
  not supported by this processor, or the processor cannot programmatically generate an 
  INIT without help from external hardware, then EFI_UNSUPPORTED is returned. If an error 
  occurs attempting to generate an INIT, then EFI_DEVICE_ERROR is returned.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  InitType         The type of processor INIT to perform.

  @retval EFI_SUCCESS           The processor INIT was performed. This return code should never be seen.
  @retval EFI_UNSUPPORTED       The processor INIT operation specified by InitType is not supported
                                by this processor.
  @retval EFI_DEVICE_ERROR      The processor INIT failed.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CPU_INIT) (
  IN EFI_CPU_ARCH_PROTOCOL              *This,
  IN EFI_CPU_INIT_TYPE                  InitType
  );


/**
  This function registers and enables the handler specified by InterruptHandler for a processor 
  interrupt or exception type specified by InterruptType. If InterruptHandler is NULL, then the 
  handler for the processor interrupt or exception type specified by InterruptType is uninstalled. 
  The installed handler is called once for each processor interrupt or exception.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  InterruptType    A pointer to the processor's current interrupt state. Set to TRUE if interrupts
                           are enabled and FALSE if interrupts are disabled.
  @param  InterruptHandler A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
                           when a processor interrupt occurs. If this parameter is NULL, then the handler
                           will be uninstalled.

  @retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for InterruptType was
                                previously installed.
  @retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for InterruptType was not
                                previously installed.
  @retval EFI_UNSUPPORTED       The interrupt specified by InterruptType is not supported.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CPU_REGISTER_INTERRUPT_HANDLER) (
  IN EFI_CPU_ARCH_PROTOCOL              *This,
  IN EFI_EXCEPTION_TYPE                 InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER          InterruptHandler
  );


/**
  This function reads the processor timer specified by TimerIndex and returns it in TimerValue.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  TimerIndex       Specifies which processor timer is to be returned in TimerValue. This parameter
                           must be between 0 and NumberOfTimers-1.
  @param  TimerValue       Pointer to the returned timer value.
  @param  TimerPeriod      A pointer to the amount of time that passes in femtoseconds for each increment
                           of TimerValue.

  @retval EFI_SUCCESS           The processor timer value specified by TimerIndex was returned in TimerValue.
  @retval EFI_DEVICE_ERROR      An error occurred attempting to read one of the processor's timers.
  @retval EFI_INVALID_PARAMETER TimerValue is NULL or TimerIndex is not valid.
  @retval EFI_UNSUPPORTED       The processor does not have any readable timers.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CPU_GET_TIMER_VALUE) (
  IN EFI_CPU_ARCH_PROTOCOL              *This,
  IN UINT32                             TimerIndex,
  OUT UINT64                            *TimerValue,
  OUT UINT64                            *TimerPeriod OPTIONAL
  );


/**
  This function modifies the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  BaseAddress      The physical address that is the start address of a memory region.
  @param  Length           The size in bytes of the memory region.
  @param  Attributes       The bit mask of attributes to set for the memory region.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not support for the memory resource
                                range specified by BaseAddress and Length.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CPU_SET_MEMORY_ATTRIBUTES) (
  IN EFI_CPU_ARCH_PROTOCOL              *This,
  IN  EFI_PHYSICAL_ADDRESS              BaseAddress,
  IN  UINT64                            Length,
  IN  UINT64                            Attributes
  );


/**
  @par Protocol Description:
  The EFI_CPU_ARCH_PROTOCOL is used to abstract processor-specific functions from the DXE
  Foundation. This includes flushing caches, enabling and disabling interrupts, hooking interrupt
  vectors and exception vectors, reading internal processor timers, resetting the processor, and
  determining the processor frequency.

  @param FlushDataCache
  Flushes a range of the processor's data cache. If the processor does 
  not contain a data cache, or the data cache is fully coherent, then this 
  function can just return EFI_SUCCESS. If the processor does not support 
  flushing a range of addresses from the data cache, then the entire data 
  cache must be flushed. 

  @param EnableInterrupt   
  Enables interrupt processing by the processor.

  @param DisableInterrupt  
  Disables interrupt processing by the processor.

  @param GetInterruptState 
  Retrieves the processor's current interrupt state.

  @param Init
  Generates an INIT on the processor. If a processor cannot programmatically 
  generate an INIT without help from external hardware, then this function 
  returns EFI_UNSUPPORTED.

  @param RegisterInterruptHandler
  Associates an interrupt service routine with one of the processor's interrupt 
  vectors. This function is typically used by the EFI_TIMER_ARCH_PROTOCOL to 
  hook the timer interrupt in a system. It can also be used by the debugger to 
  hook exception vectors.

  @param GetTimerValue       
  Returns the value of one of the processor's internal timers.

  @param SetMemoryAttributes 
  Attempts to set the attributes of a memory region.

  @param NumberOfTimers
  The number of timers that are available in a processor. The value in this 
  field is a constant that must not be modified after the CPU Architectural 
  Protocol is installed. All consumers must treat this as a read-only field.

  @param DmaBufferAlignment
  The size, in bytes, of the alignment required for DMA buffer allocations. 
  This is typically the size of the largest data cache line in the platform. 
  The value in this field is a constant that must not be modified after the 
  CPU Architectural Protocol is installed. All consumers must treat this as 
  a read-only field.

**/
struct _EFI_CPU_ARCH_PROTOCOL {
  EFI_CPU_FLUSH_DATA_CACHE            FlushDataCache;
  EFI_CPU_ENABLE_INTERRUPT            EnableInterrupt;
  EFI_CPU_DISABLE_INTERRUPT           DisableInterrupt;
  EFI_CPU_GET_INTERRUPT_STATE         GetInterruptState;
  EFI_CPU_INIT                        Init;
  EFI_CPU_REGISTER_INTERRUPT_HANDLER  RegisterInterruptHandler;
  EFI_CPU_GET_TIMER_VALUE             GetTimerValue;
  EFI_CPU_SET_MEMORY_ATTRIBUTES       SetMemoryAttributes;
  UINT32                              NumberOfTimers;
  UINT32                              DmaBufferAlignment;
};

extern EFI_GUID gEfiCpuArchProtocolGuid;

#endif
