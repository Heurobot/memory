#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "shm_data.h"

int main() {
    int fd;
    shared_memory_data *shm_ptr;

    // 1. 打开共享文件 (等待 Writer 创建)
    do {
        fd = open(FILE_NAME, O_RDWR, 0666);
        if (fd == -1) {
            printf("Waiting for writer to create file...\n");
            sleep(1);
        }
    } while (fd == -1);

    // 2. 映射文件到内存
    shm_ptr = (shared_memory_data *)mmap(NULL, sizeof(shared_memory_data),
                                         PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    close(fd);

    // 3. 读取循环
    int read_count = 0;
    while (1) {
        // 使用一个简单的循环/标志来等待数据，避免一直持有锁
        while (shm_ptr->ready_to_read == 0) {
            sleep(1); // 100ms
        }
        
        pthread_mutex_lock(&(shm_ptr->mutex)); // 锁定

        // 读取数据
        if (shm_ptr->ready_to_read == 1) {
            printf("Reader read: %s\n", shm_ptr->message);
            shm_ptr->ready_to_read = 0; // 清除标志，通知 Writer 可以继续写入
            read_count++;
        }
        
        pthread_mutex_unlock(&(shm_ptr->mutex)); // 解锁
    }

    // 4. 清理
    munmap(shm_ptr, sizeof(shared_memory_data));
    
    return 0;
}