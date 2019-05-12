#include "kboard.h"

#include <stdio.h>
#include <stdlib.h>

int main()
{
	int clipBoardValue;
	int pasteResult;

	// kboard 라이브러리를 이용하여 클립보드의 값을 붙여넣기
	pasteResult = kboard_paste(&clipBoardValue);

	// 클립보드가 비어있을 경우 에러 메시지 출력
	if (pasteResult != 0)
	{
		printf("Paste failed, KBoard is empty\n");
		return -1;
	}

	printf("Paste success, value: '%d'\n", clipBoardValue);

	return 0;
}
