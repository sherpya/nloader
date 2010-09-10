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

SECTION RODATA

msg_notimpl db 'Invoked %s which is not implemented.', 0xa,0
msg_called  db 'In %s (called from %p)...',0xa,0
__module__  db 'loader',0

cextern stubs
cextern import_addrs
cextern import_names

cextern printf
cextern abort
cextern exit

SECTION .text

cfunction stub
    push edx
    push ecx
    push eax
    call .delta

.delta:
    mov ecx, stub_end - stub
    pop eax
    sub eax, .delta - stub
    xor edx, edx
    sub eax, [stubs]
    div ecx
    shl eax, 2
    mov edx, [import_addrs]
    mov ecx, [import_names]
    add edx, eax
    add ecx, eax
    mov edx, dword [edx]
    or edx, edx
    je .stubbed

    push edx
    push dword [esp+0x10]
    push dword [ecx]
    push msg_called
    push __module__
    mov ecx, [fs:0xb0] ;  teb->LogModule
    call ecx
    nop
    add esp, 4*4
    pop edx

    pop eax
    pop ecx
    xchg [esp], edx

    ret

.stubbed:
    push dword [ecx]
    push msg_notimpl
    mov ecx, printf
    call ecx
    mov ecx, abort
    call ecx
stub_end:

cfunction sizeof_stub
    mov eax, stub_end - stub
    ret

cfunction to_ep
    mov edx, esp
    mov ecx, [fs:0x18]
    mov esp, [ecx+4]    ; switch stack

    push edx            ; save esp, ebp, ebx, esi, edi
    push ebp
    push ebx
    push esi
    push edi

    mov eax, [edx + 4]  ; exe entrypoint
    mov ecx, [fs:0x30]  ; pass the peb as an arg to the entrypoint
    push ecx
    call eax            ; exec guest

    mov ecx, [fs:0x18]  ; stack is possibly fucked up at this point
    mov ecx, [ecx+4]    ; we roll back to the bottom
    lea esp, [ecx-20]   ; minus the saved stuff

    pop edi             ; restore clobbered regs
    pop esi
    pop ebx
    pop ebp
    pop esp             ; revert the old stack

    ret
