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

    // 2. 初始化条件变量属性 (新增)
    pthread_condattr_t c_attr;
    pthread_condattr_init(&c_attr);
    // 设置条件变量为进程共享 (PTHREAD_PROCESS_SHARED)
    pthread_condattr_setpshared(&c_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&(((shared_memory_data*)buf)->cond_read), &c_attr);
    pthread_condattr_destroy(&c_attr);
   
    char arr[5] = "xxx";
    static int num = 0;
    char buffer[15];
    
    while (1)
    {
        while (((shared_memory_data *)buf)->ready_to_read == 1)
        {
            printf("[Producer] 等待消费者读取...\n");
            pthread_cond_wait(&(((shared_memory_data *)buf)->cond_read), &(((shared_memory_data *)buf)->mutex));
        }

        snprintf(buffer,sizeof(buffer),"%s%d",arr,num);
        pthread_mutex_lock(&((shared_memory_data *)buf)->mutex);
        memcpy(((shared_memory_data *)buf)->message, buffer, sizeof(buffer));
        printf("共享内存中的消息: %s\n",((shared_memory_data *)buf)->message);
        //恢复标志，发送读取信号
        ((shared_memory_data *)buf)->ready_to_read=1;
        pthread_cond_signal(&(((shared_memory_data *)buf)->cond_read)); 

        pthread_mutex_unlock(&((shared_memory_data *)buf)->mutex);
        num++;
        sleep(1);
    }
    return 0;
}
