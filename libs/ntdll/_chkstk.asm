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

%elifidn __OUTPUT_FORMAT__,elf64

BITS 64
default rel

SECTION .rodata
msg_stackover_x64 db 'Stack overflow, ', 0

cextern stderr
cextern fputs
cextern abort

SECTION .text

; MSVC x64 __chkstk:
;   IN: RAX = bytes the caller wants to subtract from RSP.
;   It probes the requested pages but does NOT modify RSP itself; the caller
;   does `sub rsp, rax` after the call. Must preserve all GP registers
;   except R10/R11 (volatile in Win64 ABI).
;
; Our minimal implementation: validate against TEB.NtTib.StackLimit (gs:0x10)
; and abort on overflow. The probe loop is omitted — Linux grows the mapping
; automatically as long as the access is within our allocated stack range.
;
; Export both `__chkstk` (the actual MSVC x64 import name; two underscores)
; and `_chkstk` (one underscore, kept for any caller using the i386-style
; name). Both labels point at the same code.
cfunction __chkstk
cfunction _chkstk
    push    r10
    push    r11

    mov     r10, [gs:0x10]              ; NtTib.StackLimit (lower bound)
    mov     r11, rsp
    sub     r11, rax                    ; new sp would land here
    cmp     r11, r10
    jb      .stack_smash

    pop     r11
    pop     r10
    ret

.stack_smash:
    ; Cross into SysV ABI to call libc. fputs(s, stream): rdi=s, rsi=stream.
    lea     rdi, [rel msg_stackover_x64]
    mov     rax, [rel stderr wrt ..gotpcrel]
    mov     rsi, [rax]
    call    fputs wrt ..plt
    xor     eax, eax
    call    abort wrt ..plt

%else

BITS 32

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
