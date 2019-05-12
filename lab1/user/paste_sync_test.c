#include "kboard.h"

#include <stdio.h>
#include <stdlib.h>

int main()
{
	// 클립보드로 부터 붙여넣기 한 값
	int clipBoardValue;

	// 예상되는 클립보드의 값
	int expectValue = 0;

	while (1)
	{
		// 클립보드가 비어있으면 아무일도 하지 않음
		if (kboard_paste(&clipBoardValue) != 0)
		{
			continue;
		}

		// 클립보드로 부터 받은 값이 예상되는 값과 다를 경우 동기화 문제 발생
		if (clipBoardValue != expectValue)
		{
			printf("Validation fault, clipBoard: '%d', expect: '%d'\n", clipBoardValue, expectValue);
			break;
		}

		// 다음에 받아야 할 값을 1 만큼 증가
		expectValue++;
	}

	return 0;
}
