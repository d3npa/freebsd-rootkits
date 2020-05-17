; nasm -O0 -f elf64 hook.asm
; objdump -dMintel hook.o
BITS 64
GLOBAL hook

msg:
	db	"Hello, world!", 0x0a, 0x00

hook:
	; レジスタの値を保存
	push 	rdi
	push 	rax

	; uprintを呼び出す
	mov 	QWORD rdi, 0x00
	mov 	QWORD rax, 0x00
	call 	rax

	; レジスタをリストア
	pop 	rax
	pop 	rdi

jump:
	mov 	QWORD rax, 0x00
	jmp 	rax
