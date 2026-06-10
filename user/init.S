.section .text
.global _start
_start:
    movq    $1,              %rax
    movq    $1,              %rdi
    leaq    msg(%rip),       %rsi
    movq    $msg_len,        %rdx
    syscall

    movq    $60,             %rax
    xorq    %rdi,            %rdi
    syscall

.section .rodata
msg:
    .ascii  "Hello, World!\n"
.set msg_len, . - msg