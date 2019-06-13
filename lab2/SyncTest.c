#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *WriterEnqueue(void *unused);
void *WriterDequeue(void *unused);
void *Reader(void* unused);

int main(int argc, char *argv[])
{
	int index;
	int result;
	int writerEnqueueNumber, writerDequeueNumber, readerNumber;
	int duration;
	pthread_t *writerEnqueueThread;
	pthread_t *writerDequeueThread;
	pthread_t *readerThread;

	if (argc < 4)
	{
		printf("Writer, Reader의 수와 수행 시간(초)을 입력하세요\n");
		return -1;
	}

	// 명령행 인자들의 값으로 Writer 쓰레드 수, Reader 쓰레드 수, 수행 시간을 설정
	writerEnqueueNumber = atoi(argv[1]) / 2;
	writerDequeueNumber = atoi(argv[1]) - (atoi(argv[1]) / 2);
	readerNumber = atoi(argv[2]);
	duration = atoi(argv[3]);

	// Writer, Reader의 쓰레드를 담을 배열의 공간 할당
	writerEnqueueThread = (pthread_t *)malloc(sizeof(pthread_t) * writerEnqueueNumber);
	writerDequeueThread = (pthread_t *)malloc(sizeof(pthread_t) * writerDequeueNumber);
	readerThread = (pthread_t *)malloc(sizeof(pthread_t) * readerNumber);

	// Writer, Reader 쓰레드 생성
	for (index = 0; index < writerEnqueueNumber; index++)
	{
		writerEnqueueThread[index] =
			pthread_create(&writerEnqueueThread[index], NULL, WriterEnqueue, NULL);
	}

	for (index = 0; index < writerDequeueNumber; index++)
	{
		writerDequeueThread[index] =
			pthread_create(&writerDequeueThread[index], NULL, WriterDequeue, NULL);
	}

	for (index = 0; index < readerNumber; index++)
	{
		readerThread[index] = pthread_create(&readerThread[index], NULL, Reader, NULL);
	}

	// Writer, Reader 쓰레드 실행
	for (index = 0; index < writerEnqueueNumber; index++)
	{
		pthread_join(writerEnqueueThread[index], (void**)&result);
	}

	for (index = 0; index < writerDequeueNumber; index++)
	{
		pthread_join(writerDequeueThread[index], (void**)&result);
	}

	for (index = 0; index < readerNumber; index++)
	{
		pthread_join(readerThread[index], (void**)&result);
	}

	// 목표 수행 시간동안 대기
	usleep(duration * 1000 * 1000);

	free(writerEnqueueThread);
	free(writerDequeueThread);
	free(readerThread);

	return 0;
}

// Writer Proc FS의 write() 수행: Kboard Enqueue 작업
void *WriterEnqueue(void *unused)
{
	while (true)
	{
		system("echo 777 > /proc/kboard/writer");
	}
}

// Writer Proc FS의 read() 수행: Kboard Dequeue 작업
void *WriterDequeue(void *unused)
{
	while (true)
	{
		system("cat /proc/kboard/writer");
	}
}

// Reader Proc FS의 read() 수행: Kboard의 Queue에 있는 무작위 값 출력
void *Reader(void *unused)
{
	while (true)
	{
		system("cat /proc/kboard/reader");
	}
}
