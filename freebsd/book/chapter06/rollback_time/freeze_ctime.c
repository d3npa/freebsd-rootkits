#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <kvm.h>
#include <nlist.h>

#define TARGET "/tmp/test"
#define RETURN "\xc3"

int main()
{
	kvm_t *kd;
	struct nlist nl[] = { {NULL}, {NULL} };
	char errbuf[_POSIX2_LINE_MAX];
	unsigned char backup, new;
	time_t tv;
	int fd;
	char contents[64];

	/* kvmの記述子を取得する */
	kd = kvm_openfiles(NULL, NULL, NULL, O_RDWR, errbuf);
	if (kd == NULL) {
		fprintf(stderr, "ERROR: %s\n", errbuf);
		exit(-1);
	}

	/* 目的関数の位置を解決する */
	nl[0].n_name = "ufs_itimes_locked";
	if (kvm_nlist(kd, nl) != 0) {
		fprintf(stderr, "ERROR: %s\n", errbuf);
		exit(-1);
	}

	printf("Function \033[94m%s\033[0m is at \033[94m0x%lx\033[0m\n", 
			nl[0].n_name, nl[0].n_value);

	/* ufs_itimes_lockedの最初命令を読み込む */
	if (kvm_read(kd, nl[0].n_value, &backup, 1) != 1) {
		fprintf(stderr, "ERROR: Could not read from 0x%lx\n", 
			nl[0].n_value);
		exit(-1);
	} 

	printf("First instruction of \033[94m%s\033[0m is "
			"\033[94m0x%hhx\033[0m\n", nl[0].n_name, backup);

	/* ufs_itimes_lockedの最初命令をretに書き換え、関数を無効化する */
	if (kvm_write(kd, nl[0].n_value, RETURN, 1) != 1) {
		fprintf(stderr, "ERROR: Could not write to 0x%lx\n", 
			nl[0].n_value);
		exit(-1);
	}

	if (kvm_read(kd, nl[0].n_value, &new, 1) != 1) {
		fprintf(stderr, "ERROR: Could not read from 0x%lx\n", 
			nl[0].n_value);
		exit(-1);
	} 

	printf("New first instruction of \033[94m%s\033[0m is "
		"\033[94m0x%hhx\033[0m\n", nl[0].n_name, new);


	/* TARGETファイルを更新する */
	time(&tv);
	sprintf(contents, "Edited at %lu\n", tv);
	fd = open(TARGET, O_RDWR);
	write(fd, contents, strlen(contents));
	close(fd);

	/* ufs_itimes_lockedをもとに戻す */
	if (kvm_write(kd, nl[0].n_value, &backup, 1) != 1) {
		fprintf(stderr, "ERROR: Could not write to 0x%lx\n", 
			nl[0].n_value);
		exit(-1);
	}

	kvm_close(kd);

	return 0;
}