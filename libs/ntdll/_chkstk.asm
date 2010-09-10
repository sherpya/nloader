; * Copyright (c) 2010, Sherpya <sherpya@netfarm.it>, aCaB <acab@0xacab.net>
; * All rights reserved.
; *
; * Redistribution and use in source and binary forms, with or without
; * modification, are permitted provided that the following conditions are met:
; *     * Redistributions of source code must retain the above copyright
; *       notice, this list of conditions and the following disclaimer.
; *     * Redistributions in binary form must reproduce the above copyright
; *       notice, this list of conditions and the following disclaimer in the
; *       documentation and/or other materials provided with the distribution.
; *     * Neither the name of the copyright holders nor the
; *       names of its contributors may be used to endorse or promote products
; *       derived from this software without specific prior written permission.
; *
; * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
; * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
; * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
; * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
; * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
; * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

%include "asmdefs.inc"

%ifidn __OUTPUT_FORMAT__,win32

SECTION .text
cfunction _chkstk
    push ecx
    mov ecx, esp
    add ecx, 8

alloca_probe:
    cmp eax, 0x1000
    jb .done
    sub ecx, 0x1000
    or dword [ecx], 0
    sub eax, 0x1000
    jmp alloca_probe

.done:
    sub ecx, eax
    or dword [ecx], 0
    mov eax, esp
    mov esp, ecx
    mov ecx, dword [eax]
    mov eax, dword [eax+4]
    jmp eax

%else

SECTION .rodata
msg_stackover db 'Stack overflow, ', 0

cextern stderr
cextern fputs
cextern abort

SECTION .text
cfunction _chkstk
    push ecx                    ; avoid clobbering ecx
    mov ecx, [fs:0x18]          ; teb self
    neg eax
    mov ecx, [ecx+8]            ; absolute stack top
    lea eax, [eax*1+esp+4]      ; new stack
    cmp eax, ecx                ; nuff stack space ?
    pop ecx                     ; restore ecx
    jb .stack_smash
    xchg eax, esp               ; all good
    mov eax, [eax]
    jmp eax

.stack_smash:                   ; no more stack
    push dword [stderr]
    push msg_stackover
    call fputs
    call abort

%endif
