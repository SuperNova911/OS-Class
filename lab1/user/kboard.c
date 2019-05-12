#include "kboard.h"

#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

// 클립보드에 복사
long kboard_copy(int clip)
{
	return syscall(335, clip);
}

// 클립보드의 값을 붙여넣기
int kboard_paste(int* clip)
{
	return syscall(336, clip);
}

// 클립보드 초기화
void kboard_init()
{
	syscall(337);
}
