.globl main
main:
    addi sp, sp, -22
    mv a0, sp
    li a1, 22
    call gets
    beqz a0, .Lerror

    mv s0, a0
    li a1, ' '
    call strchr
    beqz a0, .Lerror

    sb zero, 0(a0)
    addi s2, a0, 1

    mv a0, s0
    jal parse_number
    mv s1, a0

    mv a0, s2
    jal parse_number

    add a1, s1, a0

    lui a0, %hi(.LC0)
    addi a0, a0, %lo(.LC0)
    call printf

    addi sp, sp, 22
    li a0, 0
    call exit

.Lerror:
    li a0, 2
    lui a1, %hi(.LC1)
    addi a1, a1, %lo(.LC1)
    li a2, 14
    li a7, 64
    ecall
    addi sp, sp, 22
    li a0, 1
    call exit

parse_number:
    li t0, 1
    lb t1, 0(a0)
    li t2, '-'
    bne t1, t2, .Lpositive
    li t0, -1
    addi a0, a0, 1
.Lpositive:
    addi sp, sp, -16
    sw ra, 12(sp)
    sw a0, 8(sp)

    call atoi

    lw ra, 12(sp)
    lw a1, 8(sp)
    addi sp, sp, 16

    mul a0, a0, t0
    ret

.LC0:
    .string "%d\n"
.LC1:
    .string "Invalid input\n"