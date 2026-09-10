; Quite frankly, I don't know how to write assembly on my own.
section .text

; The ISR table defined in our C code. The REAL ISR's.
extern interrupt_table

%define INTERRUPT_OFFSET 0x78 ; 64 (bits)
                              ; * 15 (All the general purpose registers we pushed)
                              ; / 8 (Convert it all to bytes).
                              ; Alternatively we could just use `offsetof` to calculate
                              ; the offset and hardcode it here, instead of calculating it manually.

isr_common:
    ; Push all general purpose registers to the stack.
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    cld ; Apparently, the "SysV" ABI mandates this.
        ; https://wiki.osdev.org/System_V_ABI#x86-64.
    ; Preparing for the call.
    mov rdi, rsp ; As per the SysV ABI, the first argument is rdi.
                 ; Since we've pushed all general purpose registers + some stuff down there
                 ; this basically becomes a huge object. (that we will define in C)

    mov rax, [rsp + INTERRUPT_OFFSET] ; Getting the interrupt number.
    lea rbx, [rel interrupt_table] ; Get the interrupt table.
    mov rax, [rbx + rax * 8] ; The equivalent of `rax = *(interrupt_table + index * 8)`.
                             ; Learned this from Assault Cube when I was reversing the entitylist LMAO.

    ; Check if the handler was even registered.
    cmp rax, 0
    jnz .handler_found

    ; If no handler was found, we get #DF (Double fault) and call it.
    ; Otherwise, our code will just pop the stack and ignore the exception silently.
    mov rax, 0x08 ; #DF interrupt number.
    mov rax, [rbx + rax * 8]

.handler_found:
    ; Call the interrupt handler.
    call rax

    ; Pop all general purpose registers off the stack.
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    ; Pop the stuff we pushed in the ISR stub.
    add rsp, 16

    iretq ; Return... Interrupt return. Fancy.

; Assign a variable for our cute little macro. (We're about to perform some MAGIC!!!)
%assign interrupt_number 0
%rep 256 ; Repeat 256 times.
isr_stub_ %+ interrupt_number: ; Fancy way of doing `void isr_stub_##isr()`.
    ; Apparently some interrupts return an error code while others don't.
    ; https://wiki.osdev.org/Interrupt_Descriptor_Table#IDT_items.
    %if (interrupt_number == 8)  || (interrupt_number == 10) || (interrupt_number == 11) || (interrupt_number == 12) || \
        (interrupt_number == 13) || (interrupt_number == 14) || (interrupt_number == 17) || (interrupt_number == 21)
        ; No need for an error code.
        ; Apparently NASM doesn't have the `!=` operator. :sob:
    %else
        push qword 0 ; Dummy error code.
    %endif
    ; Push the interrupt number to the stack.
    push qword interrupt_number
    ; Call the wrapper function. This will push everything to the stack and
    ; call the real handler. When the handler is called we would've already
    ; pushed interrupt_number and most general-purpose registers to
    ; the stack -- we will need to put that into account when implementing the handlers.
    jmp isr_common
%assign interrupt_number interrupt_number + 1
%endrep

; Now we got the interrupts. But, we need a way to iterate through them
; in our C code. One way is to make a simple table. We could either make
; that table in our C code using some macro magic, or here, using macro magic.

; We will need to write our table somewhere. .rodata (read-only data) will work.
section .rodata
global interrupt_stub_table ; Make it global so our C code can access it.
interrupt_stub_table: ; The actual symbol.
%assign interrupt_number 0
%rep 256
    ; "dq means "define quadword" [i.e. define 64 bit item(s)]."
    ; - https://stackoverflow.com/a/53856228.
    dq isr_stub_ %+ interrupt_number
%assign interrupt_number interrupt_number + 1
%endrep

