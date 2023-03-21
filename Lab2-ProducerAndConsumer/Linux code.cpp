#include <string.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

//宏定义信号量的数量以及缓冲区大小
#define SEM_NUM 3
#define BUF_SIZE 3

//全局定义信号量标识符与共享内存标识符，创建子进程时将一并复制
int semid;
int shmid;

//定义缓冲区结构，为定长数据区与读写指针
typedef struct buffer
{
	unsigned char data[BUF_SIZE];
	int in;
	int out;
} buffer;

//将信号量数组下标0、1、2映射为信号量名称值，便于访问
enum sem_name
{
	mutex,
	empty,
	full
};

//信号量P操作
int P(sem_name name)
{
	struct sembuf psembuf = {name, -1, 0};
	return semop(semid, &psembuf, 1);
}

//信号量V操作
int V(sem_name name)
{
	struct sembuf psembuf = {name, 1, 0};
	return semop(semid, &psembuf, 1);
}

//生产者
void producer(int id)
{
	//通过共享内存标识符将内存绑定到进程中的地址
	buffer *buf_addr = (buffer *)shmat(shmid, nullptr, 0);

	time_t t;
	char tmpBuf[10];

	for (int i = 0; i < 6; i++)
	{
		//随机等待0-3秒
		srand(time(0) + id * 100 + i);
		usleep(rand() % 3000 * 1000);

		//申请信号量
		P(empty);
		P(mutex);

		//用随机值写入缓冲区
		buf_addr->data[buf_addr->in] = rand() % 256;

		//获取当前时间并输出操作内容
		t = time(0);
		strftime(tmpBuf, 10, "%H:%M:%S", localtime(&t));
		printf("Producer %d writes data: %d to buffer %d at %s.\n", id, buf_addr->data[buf_addr->in], buf_addr->in, tmpBuf);
		fflush(stdout);

		//更新写指针
		buf_addr->in = (buf_addr->in + 1) % BUF_SIZE;

		//释放信号量
		V(mutex);
		V(full);
	}
	
	//解绑共享内存
	shmdt((void*)buf_addr);
}

//消费者
void consumer(int id)
{

	buffer *buf_addr = (buffer *)shmat(shmid, nullptr, 0);

	time_t t;
	char tmpBuf[10];

	int content;

	for (int i = 0; i < 4; i++)
	{

		//随机等待0-3秒
		srand(time(0) + id * 100 + i);
		usleep(rand() % 3000 * 1000);

		P(full);
		P(mutex);

		content = buf_addr->data[buf_addr->out];

		//获取当前时间并输出操作内容
		t = time(0);
		strftime(tmpBuf, 10, "%H:%M:%S", localtime(&t));
		printf("Consumer %d reads data: %d from buffer %d at %s.\n", id - 2, content, buf_addr->out, tmpBuf);
		fflush(stdout);

		buf_addr->out = (buf_addr->out + 1) % BUF_SIZE;

		V(mutex);
		V(empty);
	}
	
	//解绑共享内存
	shmdt((void*)buf_addr);
}

int main()
{
	//创建3个信号量
	semid = semget(2333, SEM_NUM, IPC_CREAT);
	semctl(semid, mutex, SETVAL, 1);
	semctl(semid, empty, SETVAL, BUF_SIZE);
	semctl(semid, full, SETVAL, 0);
	
	//创建共享内存
	shmid = shmget(6666, sizeof(buffer), IPC_CREAT);
	
	//初值设置
	buffer *buf_addr = (buffer *)shmat(shmid, nullptr, 0);
	buf_addr->in = 0;
	buf_addr->out = 0;
	
	//解绑共享内存
	shmdt((void*)buf_addr);

	//执行生产者进程
	for (int i = 0; i < 2; i++)
	{
		//创建进程，返回值不为0代表为子进程，执行相应函数，结束时退出进程
		if (fork() == 0)
		{
			producer(i);
			exit(0);
		}
	}

	//执行消费者进程
	for (int i = 2; i < 5; i++)
	{
		//创建进程，返回值不为0代表为子进程，执行相应函数，结束时退出进程
		if (fork() == 0)
		{
			consumer(i);
			exit(0);
		}
	}
	
	//没有子进程时返回-1中止循环
	while (-1 != wait(0));
}
