#include <linux/spinlock.h>
#include <linux/syscalls.h>
#include <linux/printk.h>
#include <linux/uaccess.h>

#define MAX_CLIP (5)
#define INIT_VALUE (-1)

spinlock_t Lock;

int Ring[MAX_CLIP];		// RingBuffer
int Count = 0;			// 저장 된 값의 개수
int CurrentIndex = 0;	// 붙여넣기 할 값의 인덱스

// 매개변수로 받은 값을 링 버퍼에 넣음
long do_sys_kb_enqueue(int item)
{
    printk(KERN_DEBUG "KBOARD: do_sys_kb_enqueue() Called, item: '%d'\n", item);

	// 전달받은 값이 음수인지 검사
	if (item < 0)
	{
		printk(KERN_DEBUG "KBOARD: item cannot be negative value, item: '%d'\n", item);

		return -2;
	}

	spin_lock(&Lock);

	// 링 버퍼가 가득 찼는지 검사
	if (Count >= MAX_CLIP)
	{
		printk(KERN_DEBUG "KBOARD: Buffer is full, Count: '%d'\n", Count);
		spin_unlock(&Lock);

		return -1;
	}

	// 링 버퍼에 값을 저장하고 Count를 증가
	Ring[(CurrentIndex + Count) % MAX_CLIP] = item;
	Count++;

	spin_unlock(&Lock);

    return 0;
}

// 매개변수로 받은 주소에 링 버퍼에 있는 값을 넣어줌
long do_sys_kb_dequeue(int *user_buf)
{
    printk(KERN_DEBUG "KBOARD: do_sys_kb_dequeue() Called, address: '0x%p'\n", user_buf);

	spin_lock(&Lock);

	// 링 버퍼가 비어있는지 검사
	if (Count <= 0)
	{
		printk(KERN_DEBUG "KBOARD: Buffer is empty, Count: '%d'\n", Count);
		spin_unlock(&Lock);

		return -1;
	}

	// 링 버퍼의 값을 유저에게 복사해 주고 예외처리
	if (copy_to_user(user_buf, &Ring[CurrentIndex], sizeof(Ring[CurrentIndex])) != 0)
	{
		printk(KERN_DEBUG "KBOARD: Failed copy_to_user, Count: '%d', CurrentIndex: '%d', BufferValue: '%d', UserAddress: '0x%p'\n", Count, CurrentIndex, Ring[CurrentIndex], user_buf);
		spin_unlock(&Lock);

		return -2;
	}

	// 붙여넣기가 끝난 값을 초기화
	Ring[CurrentIndex] = INIT_VALUE;

	// CurrentIndex를 다음 칸으로 이동시키고 Count를 감소
	Count--;
	CurrentIndex = (CurrentIndex + 1) % MAX_CLIP;

	spin_unlock(&Lock);

    return 0;
}

// 링 버퍼를 초기화
long do_sys_kb_init(void)
{
	int index;

	spin_lock_init(&Lock);
	spin_lock(&Lock);

	// 링 버퍼의 값을 초기값으로 설정
	for (index = 0; index < MAX_CLIP; index++)
	{
		Ring[index] = INIT_VALUE;
	}
	Count = 0;
	CurrentIndex = 0;

	spin_unlock(&Lock);

	return 0;
}

SYSCALL_DEFINE1(kb_enqueue, int, item)
{
    return do_sys_kb_enqueue(item);
}

SYSCALL_DEFINE1(kb_dequeue, int __user *, user_buf)
{
    return do_sys_kb_dequeue(user_buf);
}

SYSCALL_DEFINE0(kb_init)
{
	return do_sys_kb_init();
}
