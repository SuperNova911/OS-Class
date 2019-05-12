#include "kboard.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	int userInput;
	int copyResult;

	// 클립보드에 복사 할 인자를 입력했는지 검사
	if (argc < 2)
	{
		printf("Insert value for copy to clip board\n");
		return -1;
	}

	// kboard 라이브러리를 이용하여 클립보드에 입력한 값을 복사
	userInput = atoi(argv[1]);
	copyResult = kboard_copy(userInput);

	// 클립보드에 값 복사를 실패했을 경우 에러 메시지 출력
	if (copyResult != 0)
	{
		printf("Copy failed, KBoard is full or invalid clip\n");
		return -1;
	}

	printf("Copy success, value: '%d'\n", userInput);

	return 0;
}
