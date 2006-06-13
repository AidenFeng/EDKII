#------------------------------------------------------------------------------
#
# Copyright (c) 2006, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   SetMem32.Asm
#
# Abstract:
#
#   SetMem32 function
#
# Notes:
#
#------------------------------------------------------------------------------

    .386: 
    .code: 

.global _InternalMemSetMem32
_InternalMemSetMem32:
    push    %edi
    movl    16(%esp),%eax
    movl    8(%esp),%edi
    movl    12(%esp),%ecx
    rep
    stosl
    movl    8(%esp),%eax
    pop     %edi
    ret
