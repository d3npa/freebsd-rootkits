; nasm -O0 -f elf64 hook.asm
; objdump -dMintel hook.o
BITS 64
GLOBAL hook

msg:
	db	"Hello, world!", 0x0a, 0x00
hook:
	; レジスタの値を保存
	push 	rax
	push 	rdi
	push 	rsi
	push	rdx
	; uprintを呼び出す
	xor	rsi, rsi
	xor 	rdx, rdx
	mov 	QWORD rdi, 0x00
	mov 	QWORD rax, 0x00
	call 	rax
	; レジスタをリストア
	pop	rdx
	pop	rsi
	pop 	rdi
	pop 	rax
jump:
	mov 	QWORD rax, 0x00
	jmp 	rax
