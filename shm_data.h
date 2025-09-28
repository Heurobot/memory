#include <pthread.h>

// 共享内存的大小
#define SHM_SIZE 4096  //这里只是申请了很小的空间来传递数据
// 消息缓冲区大小
#define MSG_SIZE 256
// 共享文件的路径
#define FILE_NAME "file.txt"

typedef struct {
    pthread_mutex_t mutex; // 跨进程互斥锁
    char message[MSG_SIZE]; // 实际共享数据
    int ready_to_read;      // 状态标志：写入完成，可读
} shared_memory_data;