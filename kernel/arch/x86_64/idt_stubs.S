.code64
.section .text

.macro isr_no_err num
.global isr_stub_\num
isr_stub_\num:
    pushq   $0              /* dummy error code */
    pushq   $\num           /* vector number    */
    jmp     isr_common
.endm

.macro isr_err num
.global isr_stub_\num
isr_stub_\num:
    pushq   $\num
    jmp     isr_common
.endm

isr_no_err  0   /* #DE  Divide Error            */
isr_no_err  1   /* #DB  Debug                   */
isr_no_err  2   /* NMI                          */
isr_no_err  3   /* #BP  Breakpoint              */
isr_no_err  4   /* #OF  Overflow                */
isr_no_err  5   /* #BR  Bound Range Exceeded    */
isr_no_err  6   /* #UD  Invalid Opcode          */
isr_no_err  7   /* #NM  Device Not Available    */
isr_err     8   /* #DF  Double Fault  (err=0)   */
isr_no_err  9   /* Coproc Segment Overrun (legacy) */
isr_err    10   /* #TS  Invalid TSS             */
isr_err    11   /* #NP  Segment Not Present     */
isr_err    12   /* #SS  Stack Segment Fault     */
isr_err    13   /* #GP  General Protection      */
isr_err    14   /* #PF  Page Fault              */
isr_no_err 15   /* (reserved)                   */
isr_no_err 16   /* #MF  x87 FP Exception        */
isr_err    17   /* #AC  Alignment Check         */
isr_no_err 18   /* #MC  Machine Check           */
isr_no_err 19   /* #XF  SIMD FP Exception       */
isr_no_err 20
isr_no_err 21
isr_no_err 22
isr_no_err 23
isr_no_err 24
isr_no_err 25
isr_no_err 26
isr_no_err 27
isr_no_err 28
isr_no_err 29
isr_err    30   /* #SX  Security Exception      */
isr_no_err 31

/* IRQs 32-47: remapped PIC (IRQ0-7 -> 32-39, IRQ8-15 -> 40-47) */
isr_no_err 32   /* IRQ0  pit timer              */
isr_no_err 33   /* IRQ1  ps/2 keyboard          */
isr_no_err 34
isr_no_err 35
isr_no_err 36
isr_no_err 37
isr_no_err 38
isr_no_err 39
isr_no_err 40
isr_no_err 41
isr_no_err 42
isr_no_err 43
isr_no_err 44
isr_no_err 45
isr_no_err 46
isr_no_err 47

isr_no_err 128

.section .rodata
.global isr_stub_table
isr_stub_table:
    .quad isr_stub_0,  isr_stub_1,  isr_stub_2,  isr_stub_3
    .quad isr_stub_4,  isr_stub_5,  isr_stub_6,  isr_stub_7
    .quad isr_stub_8,  isr_stub_9,  isr_stub_10, isr_stub_11
    .quad isr_stub_12, isr_stub_13, isr_stub_14, isr_stub_15
    .quad isr_stub_16, isr_stub_17, isr_stub_18, isr_stub_19
    .quad isr_stub_20, isr_stub_21, isr_stub_22, isr_stub_23
    .quad isr_stub_24, isr_stub_25, isr_stub_26, isr_stub_27
    .quad isr_stub_28, isr_stub_29, isr_stub_30, isr_stub_31
    
    .quad isr_stub_32, isr_stub_33, isr_stub_34, isr_stub_35
    .quad isr_stub_36, isr_stub_37, isr_stub_38, isr_stub_39
    .quad isr_stub_40, isr_stub_41, isr_stub_42, isr_stub_43
    .quad isr_stub_44, isr_stub_45, isr_stub_46, isr_stub_47
    /* int 0x80 at table index 48 */
    .quad isr_stub_128

.section .text
isr_common:
    /* CS is at RSP+24 (after dummy+num, before iretq frame: RIP/CS/RFLAGS/RSP/SS) */
    testb   $3, 24(%rsp)
    jz      .Lno_swapgs_in
    swapgs
.Lno_swapgs_in:

    pushq   %rax
    pushq   %rbx
    pushq   %rcx
    pushq   %rdx
    pushq   %rsi
    pushq   %rdi
    pushq   %rbp
    pushq   %r8
    pushq   %r9
    pushq   %r10
    pushq   %r11
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15

    movq    %rsp, %rdi
    call    isr_dispatch

    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %r11
    popq    %r10
    popq    %r9
    popq    %r8
    popq    %rbp
    popq    %rdi
    popq    %rsi
    popq    %rdx
    popq    %rcx
    popq    %rbx
    popq    %rax

    /* same layout: CS still at RSP+24 */
    testb   $3, 24(%rsp)
    jz      .Lno_swapgs_out
    swapgs
.Lno_swapgs_out:

    addq    $16, %rsp
    iretq
