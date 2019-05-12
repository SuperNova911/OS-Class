#include "kboard.h"

#include <stdio.h>
#include <stdlib.h>

int main()
{
	// 클립보드에 복사 할 값
	int value = 0;

	while (1)
	{
		// 클립보드가 가득 찼으면 value 값을 증가 시키지 않음
		if (kboard_copy(value) != 0)
		{
			continue;
		}

		// 복사 할 값을 1 만큼 증가
		value++;
	}

	return 0;
}
