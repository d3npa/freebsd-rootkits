#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define TARGET "/tmp/test"

int main()
{
	struct stat _stat;
	struct stat _stat2;

	/* TARGETの最新アクセス時刻と最新更新時刻を取得する */
	stat(TARGET, &_stat);
	printf("\n更新前\n");
	printf("atime -> %lu\n", _stat.st_atime);
	printf("mtime -> %lu\n", _stat.st_mtime);
	printf("ctime -> %lu\n\n", _stat.st_ctime);

	// /* ファイルを更新する */
	// int fd = open(TARGET, O_RDWR);
	// write(fd, "I was here\n", 11);
	// close(fd);

	// /* 更新されたことを確認する */
	// stat(TARGET, &_stat2);
	// printf("更新後\n");
	// printf("atime -> %lu\n", _stat.st_atime);
	// printf("mtime -> %lu\n\n", _stat.st_mtime);

	// /* 更新前のアクセス時刻と更新時刻を修整する */
	// const struct timespec times[2] = {
	// 	_stat.st_atim,
	// 	_stat.st_mtim
	// };
	// utimensat(AT_FDCWD, TARGET, times, 0);

	// /* 修整されたことを確認する */
	// stat(TARGET, &_stat);
	// printf("修整後\n");
	// printf("atime -> %lu\n", _stat.st_atime);
	// printf("mtime -> %lu\n\n", _stat.st_mtime);

	return 0;
}