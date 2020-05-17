; nasm -O0 -f elf64 hook.asm
; objdump -dMintel hook.o
BITS 64
GLOBAL hook
msg:
	db	"Hello, world!", 0x0a, 0x00

hook:
	push 	rbp
	mov	rbp, rsp
	sub	rsp, BYTE 0x08
	push 	rax
	push 	rbx
	mov 	rbx, 0x00
	mov 	rax, 0x00
	mov 	[rsp], rbx
	call	[rax]
	pop	rbx
	pop 	rax
	add 	rsp, BYTE 0x08
	pop 	rbp

jump:
	mov	rax, 0x00
	jmp	[rax]
