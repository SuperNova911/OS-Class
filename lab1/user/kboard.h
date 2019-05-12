#pragma once

// 매개변수로 받은 정수 값을 클립보드로 복사
long kboard_copy(int clip);

// 매개변수로 받은 주소에 클립보드로 부터 값을 붙여넣기 해줌
int kboard_paste(int *clip);

// 클립보드 초기화
void kboard_init();
