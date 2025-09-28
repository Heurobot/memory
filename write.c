#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  //文件操作 进程控制 sleep等杂项服务
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "shm_data.h"
//<fcntl.h>：提供 open() 函数和 O_RDWR, O_CREAT 标志，用于打开文件并获取 fd。

//<sys/stat.h>：提供文件创建时所需的权限模式（例如 0666）相关的定义。

//<sys/mman.h>：提供核心的 mmap() 函数，用于将 fd 映射到内存中。
int main() {
    int fd;
    shared_memory_data *shm_ptr;

    // 1. 创建/打开共享文件
    // O_RDWR: 读写, O_CREAT: 创建, O_TRUNC: 截断/清空
    fd = open(FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0666); //??0666
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // 2. 调整文件大小以容纳共享结构体
    if (ftruncate(fd, sizeof(shared_memory_data)) == -1) { //?调整文件大小
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // 3. 创建内存映射
    shm_ptr = (shared_memory_data *)mmap(NULL, sizeof(shared_memory_data),
                                         PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    memset(shm_ptr,0,sizeof(shared_memory_data));
    close(fd); // 文件描述符可以关闭，映射仍然有效

    // 4. 初始化跨进程互斥锁
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    // 设置互斥锁为进程共享
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(shm_ptr->mutex), &attr);

    // 5. 写入循环
    int count = 0;
    while (1) {
        pthread_mutex_lock(&(shm_ptr->mutex)); // 锁定

        // 写入数据
        snprintf(shm_ptr->message, MSG_SIZE, "Hello from Writer, count: %d", count);
        shm_ptr->ready_to_read = 1; // 设置标志，通知 Reader 可以读取

        printf("Writer wrote: %s\n", shm_ptr->message);
        
        pthread_mutex_unlock(&(shm_ptr->mutex)); // 解锁
        count++;
        sleep(1); // 等待一秒
    }

    // 6. 清理
    pthread_mutex_destroy(&(shm_ptr->mutex));
    munmap(shm_ptr, sizeof(shared_memory_data));
 
    
    return 0;
}