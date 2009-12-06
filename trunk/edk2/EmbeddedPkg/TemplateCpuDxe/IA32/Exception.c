/** @file

  Copyright (c) 2008-2009, Apple Inc. All rights reserved.
  
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
 
#include <CpuDxe.h>


EFI_EXCEPTION_CALLBACK  gExceptionHandlers[0x100];


/**
  This function registers and enables the handler specified by InterruptHandler for a processor 
  interrupt or exception type specified by InterruptType. If InterruptHandler is NULL, then the 
  handler for the processor interrupt or exception type specified by InterruptType is uninstalled. 
  The installed handler is called once for each processor interrupt or exception.

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
EFI_STATUS
RegisterInterruptHandler (
  IN EFI_EXCEPTION_TYPE             InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER      InterruptHandler
  )
{
	if (InterruptType > 0xFF) {
		return EFI_UNSUPPORTED;
	}

	if ((InterruptHandler == NULL) && (gExceptionHandlers[InterruptType] == NULL)) {
		return EFI_INVALID_PARAMETER;
	}

	if ((InterruptHandler != NULL) && (gExceptionHandlers[InterruptType] != NULL)) {
		return EFI_ALREADY_STARTED;
	}

	gExceptionHandlers[InterruptType] = InterruptHandler;

	return EFI_SUCCESS;
}




VOID
EFIAPI
DefaultExceptionHandler (
	IN     EFI_EXCEPTION_TYPE						ExceptionType,
	IN OUT EFI_SYSTEM_CONTEXT						SystemContext
	)
{
	DEBUG ((EFI_D_ERROR, "Exception %d from %x\n", ExceptionType, SystemContext.SystemContextIa32->Eip));
  ASSERT (FALSE);

	return;
}



EFI_STATUS
InitializeExceptions (
	IN EFI_CPU_ARCH_PROTOCOL    *Cpu
	)
{
  // You need to initialize gExceptionHandlers[] to point to DefaultExceptionHandler()
  // and write all the assembly to handle the interrupts.
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}



/**
  This function reads the processor timer specified by TimerIndex and returns it in TimerValue.

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
EFI_STATUS
EFIAPI
GetTimerValue (
  IN  UINT32                         TimerIndex,
  OUT UINT64                         *TimerValue,
  OUT UINT64                         *TimerPeriod   OPTIONAL
  )
{
  if (TimerValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (TimerIndex == 0) {
    *TimerValue = AsmReadTsc ();
    if (TimerPeriod != NULL) {
      //
      // BugBug: Hard coded. Don't know how to do this generically
      //
      *TimerPeriod = 1000000000;
    }
    return EFI_SUCCESS;
  }
  return EFI_INVALID_PARAMETER;
}


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

  @param  Start            The beginning physical address to flush from the processor's data
                           cache.
  @param  Length           The number of bytes to flush from the processor's data cache. This
                           function may flush more bytes than Length specifies depending upon
                           the granularity of the flush operation that the processor supports.
  @param  FlushType        Specifies the type of flush operation to perform.

  @retval EFI_SUCCESS           The address range from Start to Start+Length was flushed from
                                the processor's data cache.
  @retval EFI_UNSUPPORTED       The processor does not support the cache flush type specified
                                by FlushType.
  @retval EFI_DEVICE_ERROR      The address range from Start to Start+Length could not be flushed
                                from the processor's data cache.

**/
EFI_STATUS
EFIAPI
FlushCpuDataCache (
  IN EFI_PHYSICAL_ADDRESS            Start,
  IN UINT64                          Length,
  IN EFI_CPU_FLUSH_TYPE              FlushType
  )
{
  if (FlushType == EfiCpuFlushTypeWriteBackInvalidate) {
    AsmWbinvd ();
    return EFI_SUCCESS;
  } else if (FlushType == EfiCpuFlushTypeInvalidate) {
    AsmInvd ();  
    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
}




