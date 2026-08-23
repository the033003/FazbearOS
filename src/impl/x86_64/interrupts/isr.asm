BITS 64

global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31

global irq0
global irq1
global irq12

extern interrupt_dispatch

section .text

isr0:
    push qword 0
    push qword 0
    jmp common_interrupt

isr1:
    push qword 0
    push qword 1
    jmp common_interrupt

isr2:
    push qword 0
    push qword 2
    jmp common_interrupt

isr3:
    push qword 0
    push qword 3
    jmp common_interrupt

isr4:
    push qword 0
    push qword 4
    jmp common_interrupt

isr5:
    push qword 0
    push qword 5
    jmp common_interrupt

isr6:
    push qword 0
    push qword 6
    jmp common_interrupt

isr7:
    push qword 0
    push qword 7
    jmp common_interrupt

isr8:
    push qword 8
    jmp common_interrupt

isr9:
    push qword 0
    push qword 9
    jmp common_interrupt

isr10:
    push qword 10
    jmp common_interrupt

isr11:
    push qword 11
    jmp common_interrupt

isr12:
    push qword 12
    jmp common_interrupt

isr13:
    push qword 13
    jmp common_interrupt

isr14:
    push qword 14
    jmp common_interrupt

isr15:
    push qword 0
    push qword 15
    jmp common_interrupt

isr16:
    push qword 0
    push qword 16
    jmp common_interrupt

isr17:
    push qword 0
    push qword 17
    jmp common_interrupt

isr18:
    push qword 0
    push qword 18
    jmp common_interrupt

isr19:
    push qword 0
    push qword 19
    jmp common_interrupt

isr20:
    push qword 0
    push qword 20
    jmp common_interrupt

isr21:
    push qword 0
    push qword 21
    jmp common_interrupt

isr22:
    push qword 0
    push qword 22
    jmp common_interrupt

isr23:
    push qword 0
    push qword 23
    jmp common_interrupt

isr24:
    push qword 0
    push qword 24
    jmp common_interrupt

isr25:
    push qword 0
    push qword 25
    jmp common_interrupt

isr26:
    push qword 0
    push qword 26
    jmp common_interrupt

isr27:
    push qword 0
    push qword 27
    jmp common_interrupt

isr28:
    push qword 0
    push qword 28
    jmp common_interrupt

isr29:
    push qword 0
    push qword 29
    jmp common_interrupt

isr30:
    push qword 0
    push qword 30
    jmp common_interrupt

isr31:
    push qword 0
    push qword 31
    jmp common_interrupt

irq0:
    push qword 0
    push qword 32
    jmp common_interrupt

irq1:
    push qword 0
    push qword 33
    jmp common_interrupt

irq12:
    push qword 0
    push qword 44
    jmp common_interrupt

common_interrupt:

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp

    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp

    cld

    call interrupt_dispatch

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8

    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16

    iretq
