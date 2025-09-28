#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/ipc.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/shm.h>
#include<string.h>
#include"shm_data.h"


int main(int argc, const char *argv[])
{
	key_t key;
	int shmid;
	void *buf;
	if(((key = ftok("./keytest.txt", 100)) == -1))
	{
		perror("ftok:");
		exit(-1);
	}

	printf("%d\n", key);
	if((shmid = shmget(key, 0, 0|0666)) == -1)//申请结构体大小的共享内存
	{
		perror("shmget:");
		exit(-1);
	}

	printf("%d\n",shmid);
	buf = shmat(shmid, NULL, 0);

    //加锁
    // 4. 初始化跨进程互斥锁
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    // 设置互斥锁为进程共享
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(((shared_memory_data*)buf)->mutex), &attr);//将buf强制转换之后就可以通过结构体来整体复制

    
    char recbuffer[15];
    
    while (1)
    {  
        while (((shared_memory_data *)buf)->ready_to_read == 0)
        {
            printf("[Reader] 等待读取...\n");
            pthread_cond_wait(&(((shared_memory_data *)buf)->cond_read), &(((shared_memory_data *)buf)->mutex));
        }
        pthread_mutex_lock(&((shared_memory_data *)buf)->mutex);
        memcpy( recbuffer,((shared_memory_data *)buf)->message, sizeof(recbuffer));
        printf("读取到共享内存中的消息: %s\n",recbuffer);
        // 4. 重置标志：数据已被消费，可以进行下一次写入
        ((shared_memory_data *)buf)->ready_to_read = 0;

        // 5. [发送信号] 通知等待中的生产者：可以写入新数据了
        pthread_cond_signal(&(((shared_memory_data *)buf)->cond_read));

        pthread_mutex_unlock(&((shared_memory_data *)buf)->mutex);
       
        sleep(1);
    }
    return 0;
}
