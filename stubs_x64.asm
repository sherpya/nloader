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

BITS 64
default rel

SECTION RODATA

msg_notimpl db 'Invoked %s which is not implemented.', 0xa, 0
msg_called  db 'In %s (called from %p)...', 0xa, 0
__module__  db 'loader', 0

cextern import_addrs
cextern import_names

cextern printf
cextern abort

SECTION .text

; Per-import stub layout (17 bytes), built at runtime by the loader:
;
;   68 ii ii ii ii                  push  imm32                    ; idx
;   48 B8 dd dd dd dd dd dd dd dd   mov   rax, imm64 stub_dispatch
;   FF E0                           jmp   rax
;
; rel32 jmp would be 10 bytes but cannot reach when stubs are mmap'd far from
; the dispatcher (PIE base is ~5GB away from typical mmap region). RAX is
; volatile in the Win64 ABI and unused at call entry, so we clobber it freely.
;
; The stub does NOT execute Win64-ABI prologue/epilogue: it forwards directly
; to stub_dispatch with rcx,rdx,r8,r9 untouched and the index pushed below the
; original return address.
;
; On entry to stub_dispatch:
;   [rsp + 0x00] = idx (pushed by stub, sign-extended from imm32)
;   [rsp + 0x08] = retaddr of caller of the stub
;   args in rcx, rdx, r8, r9, then [rsp + 0x10..]
;   rsp % 16 == 0
;
; stub_dispatch resolves import_addrs[idx], optionally logs the call, then
; restores the original arg registers and tail-calls the resolved function so
; the resolved function's RET returns straight to the original caller.

cfunction stub_dispatch
    push    rbp                         ; rsp%16 = 8
    push    rbx                         ; rsp%16 = 0
    mov     rbp, rsp                    ; anchor
    ; After the two pushes:
    ;   [rbp + 0x00] = saved rbx
    ;   [rbp + 0x08] = saved rbp
    ;   [rbp + 0x10] = idx (32-bit, sign-extended)
    ;   [rbp + 0x18] = retaddr of caller of the stub

    mov     ebx, dword [rbp + 0x10]     ; ebx = idx (zero-extends)

    ; Local frame: 0x20 shadow + 0x20 arg saves + 0x10 stash/pad = 0x50
    ; rsp%16 stays 0.
    sub     rsp, 0x50
    mov     [rsp + 0x20], rcx
    mov     [rsp + 0x28], rdx
    mov     [rsp + 0x30], r8
    mov     [rsp + 0x38], r9
    ; [rsp + 0x40] reserved as r10 stash across LogModule call
    ; [rsp + 0x48] padding

    mov     rax, [rel import_addrs]
    mov     r10, [rax + rbx*8]          ; resolved fn (or NULL)
    mov     rax, [rel import_names]
    mov     r11, [rax + rbx*8]          ; name

    test    r10, r10
    jz      .stubbed

    ; teb->LogModule may be NULL (during early bring-up). Skip if so.
    mov     rax, [gs:TEB_LOGMODULE_OFFSET]
    test    rax, rax
    jz      .nolog

    ; LogModule(__module__, msg_called, name, retaddr)
    ; LogModule is host SysV ABI (so its va_list is compatible with libc
    ; vfprintf). Pass args in (rdi, rsi, rdx, rcx); zero EAX for the SysV
    ; variadic convention (no XMM args). Move the call target out of rax
    ; first, since the EAX zero clobbers the low bits of the function ptr.
    mov     [rsp + 0x40], r10           ; stash fn ptr (volatile across call)
    mov     rbx, rax                    ; rbx = LogModule (saved across call)
    lea     rdi, [rel __module__]
    lea     rsi, [rel msg_called]
    mov     rdx, r11                    ; name
    mov     rcx, [rbp + 0x18]           ; retaddr
    xor     eax, eax                    ; SysV: AL = #XMM args (0)
    call    rbx
    mov     r10, [rsp + 0x40]

.nolog:
    mov     rcx, [rsp + 0x20]
    mov     rdx, [rsp + 0x28]
    mov     r8,  [rsp + 0x30]
    mov     r9,  [rsp + 0x38]

    ; Tear down. Goal: stack should look like a normal call to resolved_fn,
    ; i.e. [rsp] = original retaddr.
    mov     rax, [rbp + 0x18]           ; original retaddr
    mov     rsp, rbp                    ; pop local frame
    pop     rbx
    pop     rbp
    ; Now [rsp + 0x00] = idx, [rsp + 0x08] = retaddr (still both on stack)
    add     rsp, 0x10                   ; remove idx + retaddr
    push    rax                         ; restore retaddr at [rsp]
    jmp     r10                         ; tail-call

.stubbed:
    ; printf and abort are libc — SysV ABI: arg0 in rdi, arg1 in rsi.
    lea     rdi, [rel msg_notimpl]
    mov     rsi, r11
    xor     eax, eax                    ; no XMM args for the variadic call
    call    printf wrt ..plt
    xor     eax, eax
    call    abort wrt ..plt
    ; abort is noreturn; control never falls through

; sizeof_stub: bytes per emitted stub (push imm32 + mov rax, imm64 + jmp rax)
cfunction sizeof_stub
    mov     eax, 17
    ret

; to_ep: switch to TEB-defined stack and call the entrypoint with peb in rcx.
; void *to_ep(void *entrypoint);
;
; Called from C using the host SysV ABI (entrypoint arrives in rdi). The
; entrypoint itself is Win64-ABI and receives the Peb in rcx. Caller-side
; stack is restored on return.

cfunction to_ep
    push    rbp                         ; save non-volatiles we touch
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15

    mov     r14, rdi                    ; entrypoint (SysV arg0 from C caller)
    mov     r12, rsp                    ; remember caller's stack

    mov     rax, [gs:TEB_STACKBASE_OFFSET]   ; new stack top (StackBase)
    mov     rcx, [gs:TEB_PEB_OFFSET]         ; arg0 = peb

    ; Switch stack. StackBase points one past the highest valid byte; align
    ; down to 16 and reserve 32 bytes shadow space. rsp%16 stays 0, so after
    ; the `call` pushes the 8-byte retaddr, the entrypoint sees rsp%16 == 8
    ; as required by the Win64 ABI.
    and     rax, -16
    sub     rax, 0x20
    mov     rsp, rax

    call    r14                         ; entrypoint(peb)

    ; Restore old stack
    mov     rsp, r12

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    ret
