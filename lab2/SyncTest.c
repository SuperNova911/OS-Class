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

	writerEnqueueNumber = atoi(argv[1]) / 2;
	writerDequeueNumber = atoi(argv[1]) - (atoi(argv[1]) / 2);
	readerNumber = atoi(argv[2]);
	duration = atoi(argv[3]);

	writerEnqueueThread = (pthread_t *)malloc(sizeof(pthread_t) * writerEnqueueNumber);
	writerDequeueThread = (pthread_t *)malloc(sizeof(pthread_t) * writerDequeueNumber);
	readerThread = (pthread_t *)malloc(sizeof(pthread_t) * readerNumber);

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

	usleep(duration * 1000 * 1000);

	return 0;
}

void *WriterEnqueue(void *unused)
{
	while (true)
	{
		system("echo 777 > /proc/kboard/writer");
	}
}

void *WriterDequeue(void *unused)
{
	while (true)
	{
		system("cat /proc/kboard/writer");
	}
}

void *Reader(void *unused)
{
	while (true)
	{
		system("cat /proc/kboard/reader");
	}
}
