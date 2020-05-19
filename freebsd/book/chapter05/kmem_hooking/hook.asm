; nasm -O0 -f elf64 hook.asm
; objdump -dMintel hook.o
BITS 64
GLOBAL hook

msg:
	db	"Hello, world!", 0x0a, 0x00

hook:
	; レジスタの値を保存
	push 	rax
	push 	rbx
	push 	rdi
	push 	r14
	; uprintを呼び出す
	xor 	rax, rax
	xor 	rbx, rbx
	mov 	QWORD rdi, 0x00
	mov 	QWORD r14, 0x00
	call 	r14
	; レジスタをリストア
	pop 	r14
	pop 	rdi
	pop 	rbx
	pop 	rax
jump:
	mov 	QWORD r14, 0x00
	jmp 	r14
