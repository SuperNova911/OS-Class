#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/random.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define KBOARD_DIRECTORY "kboard"
#define KBOARD_WRITER "writer"
#define KBOARD_READER "reader"
#define KBOARD_COUNTER "count"
#define KBOARD_DUMPER "dump"
#define RING_BUFFER_SIZE 5
#define RING_BUFFER_INIT_VALUE -1
#define WRITER_BUFFER_SIZE 20

static inline void InitializeSemaphore(struct semaphore *sema, int value);

static int InitializeKboard(void);
static void DestroyKboard(void);

static int KboardWriter_Open(struct inode * inode, struct file * file);
static int KboardWriter_Show(struct seq_file * file, void * unused);
static ssize_t KboardWriter_Write(struct file * file, const char __user * data, size_t length, loff_t * off);
static int KboardReader_Open(struct inode * inode, struct file * file);
static int KboardReader_Show(struct seq_file * file, void * unused);
static int KboardCounter_Open(struct inode * inode, struct file * file);
static int KboardCounter_Show(struct seq_file * file, void * unused);
static int KboardDumper_Open(struct inode * inode, struct file * file);
static int KboardDumper_Show(struct seq_file * file, void * unused);

static void KboardReset(void);

static int __init KboardModuleInit(void);
static void __exit KboardModuleExit(void);

static const struct file_operations KBOARD_WRITER_FILE_OPERATIONS =
{
    .owner      = THIS_MODULE,
    .open       = KboardWriter_Open,
    .write      = KboardWriter_Write,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = seq_release,
};
static const struct file_operations KBOARD_READER_FILE_OPERATIONS =
{
    .owner      = THIS_MODULE,
    .open       = KboardReader_Open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = seq_release,
};
static const struct file_operations KBOARD_COUNTER_FILE_OPERATIONS =
{
    .owner      = THIS_MODULE,
    .open       = KboardCounter_Open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = seq_release,
};
static const struct file_operations KBOARD_DUMPER_FILE_OPERATIONS =
{
    .owner      = THIS_MODULE,
    .open       = KboardDumper_Open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = seq_release,
};

static struct proc_dir_entry *ParentDirectory = NULL;
static struct proc_dir_entry *KboardProcDirectory = NULL;
static struct proc_dir_entry *KboardProcWriter = NULL;
static struct proc_dir_entry *KboardProcReader = NULL;
static struct proc_dir_entry *KboardProcCounter = NULL;
static struct proc_dir_entry *KboardProcDumper = NULL;

static struct semaphore SemaphoreMutex;
static struct semaphore SemaphoreWriter;
static struct semaphore SemaphoreReader;
static int ReaderCount;

static int PerformWriter;
static int PerformReader;

static int RingBuffer[RING_BUFFER_SIZE];
static int RingBufferCount;
static int RingBufferCurrentIndex;

static inline void InitializeSemaphore(struct semaphore *sema, int value)
{
    static struct lock_class_key __key;
    *sema = (struct semaphore) __SEMAPHORE_INITIALIZER(*sema, value);
    lockdep_init_map(&sema->lock.dep_map, "semaphore->lock", &__key, 0);
}

static int InitializeKboard(void)
{
    printk(KERN_DEBUG "'%s'\n", __func__);

    KboardProcDirectory = proc_mkdir(KBOARD_DIRECTORY, ParentDirectory);
    if (KboardProcDirectory == NULL)
    {
        printk("Failed to create /proc/%s\n", KBOARD_DIRECTORY);
        return -1;
    }

    KboardProcWriter = proc_create(KBOARD_WRITER, 0, KboardProcDirectory, &KBOARD_WRITER_FILE_OPERATIONS);
    if (KboardProcWriter == NULL)
    {
        printk("Failed to create /proc/%s/%s\n", KBOARD_DIRECTORY, KBOARD_WRITER);
        remove_proc_entry(KBOARD_DIRECTORY, ParentDirectory);
        return -1;
    }

    KboardProcReader = proc_create(KBOARD_READER, 0, KboardProcDirectory, &KBOARD_READER_FILE_OPERATIONS);
    if (KboardProcReader == NULL)
    {
        printk("Failed to create /proc/%s/%s\n", KBOARD_DIRECTORY, KBOARD_READER);
        remove_proc_entry(KBOARD_DIRECTORY, ParentDirectory);
        return -1;
    }

    KboardProcCounter = proc_create(KBOARD_COUNTER, 0, KboardProcDirectory, &KBOARD_COUNTER_FILE_OPERATIONS);
    if (KboardProcCounter == NULL)
    {
        printk("Failed to create /proc/%s/%s\n", KBOARD_DIRECTORY, KBOARD_COUNTER);
        remove_proc_entry(KBOARD_DIRECTORY, ParentDirectory);
        return -1;
    }
    
    KboardProcDumper = proc_create(KBOARD_DUMPER, 0, KboardProcDirectory, &KBOARD_DUMPER_FILE_OPERATIONS);
    if (KboardProcDumper == NULL)
    {
        printk("Failed to create /proc/%s/%s\n", KBOARD_DIRECTORY, KBOARD_DUMPER);
        remove_proc_entry(KBOARD_DIRECTORY, ParentDirectory);
        return -1;
    }

    printk(KERN_ALERT "Created /proc/%s/%s\n", KBOARD_DIRECTORY, KBOARD_WRITER);
    printk(KERN_ALERT "Created /proc/%s/%s\n", KBOARD_DIRECTORY, KBOARD_READER);
    printk(KERN_ALERT "Created /proc/%s/%s\n", KBOARD_DIRECTORY, KBOARD_COUNTER);
    printk(KERN_ALERT "Created /proc/%s/%s\n", KBOARD_DIRECTORY, KBOARD_DUMPER);

    InitializeSemaphore(&SemaphoreMutex, 1);
    InitializeSemaphore(&SemaphoreWriter, 1);
    InitializeSemaphore(&SemaphoreReader, 6);
    ReaderCount = 0;

    PerformWriter = 0;
    PerformWriter = 0;

    KboardReset();

    return 0;
}

static void DestroyKboard(void)
{
    printk(KERN_DEBUG "'%s'\n", __func__);

    remove_proc_subtree(KBOARD_DIRECTORY, ParentDirectory);
    proc_remove(KboardProcDirectory);
    proc_remove(KboardProcWriter);
    proc_remove(KboardProcReader);
    proc_remove(KboardProcCounter);
    proc_remove(KboardProcDumper);

    printk(KERN_ALERT "Removed /proc/%s/%s\n", KBOARD_DIRECTORY, KBOARD_WRITER);
    printk(KERN_ALERT "Removed /proc/%s/%s\n", KBOARD_DIRECTORY, KBOARD_READER);
    printk(KERN_ALERT "Removed /proc/%s/%s\n", KBOARD_DIRECTORY, KBOARD_COUNTER);
    printk(KERN_ALERT "Removed /proc/%s/%s\n", KBOARD_DIRECTORY, KBOARD_DUMPER);
}

static int KboardWriter_Open(struct inode * inode, struct file * file)
{
    printk(KERN_DEBUG "'%s'\n", __func__);
    return single_open(file, KboardWriter_Show, NULL);
}

static int KboardWriter_Show(struct seq_file * file, void * unused)
{
    int item;

    printk(KERN_DEBUG "'%s'\n", __func__);

    down(&SemaphoreWriter);
    PerformWriter++;

    if (RingBufferCount <= 0)
    {
        printk("%s: Ring buffer is empty, count: '%d'\n", __func__, RingBufferCount);
        up(&SemaphoreWriter);
        return -EPERM;
    }

    item = RingBuffer[RingBufferCurrentIndex];
    RingBuffer[RingBufferCurrentIndex] = RING_BUFFER_INIT_VALUE;
    RingBufferCount--;
    RingBufferCurrentIndex = (RingBufferCurrentIndex + 1) % RING_BUFFER_SIZE;

    up(&SemaphoreWriter);

    seq_printf(file, "Paste: '%d'\n", item);

    return 0;
}

static ssize_t KboardWriter_Write(struct file * file, const char __user * data, size_t length, loff_t * off)
{
    int item;
    char buffer[WRITER_BUFFER_SIZE];

    printk(KERN_DEBUG "'%s'\n", __func__);

    if (length > WRITER_BUFFER_SIZE)
    {
        printk(KERN_DEBUG "%s: Data length is too long, length: '%ld', max: '%d'\n",
            __func__, length, WRITER_BUFFER_SIZE);
        return -E2BIG;
    }

    if (copy_from_user(buffer, data, length) != 0)
    {
        printk(KERN_DEBUG "%s: Failed copy_from_user, UserAddress: '0x%p', Length: '%ld'\n",
            __func__, data, length);
        return -EFAULT;
    }

    if (sscanf(buffer, "%d", &item) != 1)
    {
        printk(KERN_DEBUG "%s: Invaild argument, must input 1 integer", __func__);
        return -EINVAL;
    }

    if (item < 0)
    {
        printk(KERN_DEBUG "%s: Item cannot be negative value, item : '%d'\n", __func__, item);
        return -EINVAL;
    }

    down(&SemaphoreWriter);
    PerformWriter++;

    if (RingBufferCount >= RING_BUFFER_SIZE)
    {
        printk(KERN_DEBUG "%s: Ring buffer is full, count: '%d'\n", __func__, RingBufferCount);
        up(&SemaphoreWriter);
        return -EPERM;
    }

    RingBuffer[(RingBufferCurrentIndex + RingBufferCount) % RING_BUFFER_SIZE] = item;
    RingBufferCount++;

    up(&SemaphoreWriter);
    
    return length;
}

static int KboardReader_Open(struct inode * inode, struct file * file)
{
    printk(KERN_DEBUG "'%s'\n", __func__);
    return single_open(file, KboardReader_Show, NULL);
}

static int KboardReader_Show(struct seq_file * file, void * unused)
{
    unsigned int randomIndex;

    printk(KERN_DEBUG "'%s'\n", __func__);

    get_random_bytes(&randomIndex, sizeof(randomIndex));
    randomIndex = randomIndex % RING_BUFFER_SIZE;

    // down(&SemaphoreMutex);
    ReaderCount++;
    if (ReaderCount == 1)
    {
        // down(&SemaphoreWriter);
    }
    // up(&SemaphoreMutex);
    PerformReader++;

    seq_printf(file, "Read random value from Kboard: index: '%d', value: '%d'\n",
        randomIndex, RingBuffer[randomIndex]);

    // down(&SemaphoreMutex);
    ReaderCount--;
    if (ReaderCount == 0)
    {
        // up(&SemaphoreWriter);
    }
    // up(&SemaphoreMutex);

    return 0;
}

static int KboardCounter_Open(struct inode * inode, struct file * file)
{
    printk(KERN_DEBUG "'%s'\n", __func__);
    return single_open(file, KboardCounter_Show, NULL);
}

static int KboardCounter_Show(struct seq_file * file, void * unused)
{
    printk(KERN_DEBUG "'%s'\n", __func__);
    seq_printf(file, "Kboard Count: '%d'\n", RingBufferCount);
    return 0;
}

static int KboardDumper_Open(struct inode * inode, struct file * file)
{
    printk(KERN_DEBUG "'%s'\n", __func__);
    return single_open(file, KboardDumper_Show, NULL);
}

static int KboardDumper_Show(struct seq_file * file, void * unused)
{
    int index;

    printk(KERN_DEBUG "'%s'\n", __func__);
    
    seq_printf(file, "====== Kboard Status ======\n");
    seq_printf(file, "[RingBuffer]\n");
    for (index = 0; index < RING_BUFFER_SIZE; index++)
    {
        seq_printf(file, "index: '%d', value: '%d'\n", index, RingBuffer[index]);
    }
    seq_printf(file, "[Count: '%d']\n", RingBufferCount);
    seq_printf(file, "[CurrentIndex: '%d']\n", RingBufferCurrentIndex);
    seq_printf(file, "[Writer: '%d' times, Reader: '%d' times]\n", PerformWriter, PerformReader);
    seq_printf(file, "===========================\n");
    
    return 0;
}

static void KboardReset(void)
{
    int index;

    printk(KERN_DEBUG "'%s'\n", __func__);

    for (index = 0; index < RING_BUFFER_SIZE; index++)
    {
        RingBuffer[index] = RING_BUFFER_INIT_VALUE;
    }
    RingBufferCount = 0;
    RingBufferCurrentIndex = 0;
}

static int __init KboardModuleInit(void)
{
    printk(KERN_DEBUG "'%s'\n", __func__);
    return InitializeKboard();
}

static void __exit KboardModuleExit(void)
{
    printk(KERN_DEBUG "'%s'\n", __func__);
    DestroyKboard();
}

module_init(KboardModuleInit);
module_exit(KboardModuleExit);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Suwhan Kim <suwhan77@naver.com>");
MODULE_DESCRIPTION("KBoard V2");