# 操作系统课程设计实验报告

实验名称：生产者消费者问题

姓名/学号：蒋任驰/1120202953

## 一、实验目的

通过在Windows和Linux上用C语言实现进程级别的生产者消费者问题，能够更好地掌握进程间用信号量进行通信的过程，因为调用了系统级别的进程API，也能更好地了解信号量以及进程在系统中的具体实现（创建、调用、释放等）。

## 二、实验内容

生产者与消费者问题

•    一个大小为3的缓冲区，初始为空

•    2个生产者

–  随机等待一段时间，往缓冲区添加数据，

–  若缓冲区已满，等待消费者取走数据后再添加

–  重复6次

•    3个消费者

–  随机等待一段时间，从缓冲区读取数据

–  若缓冲区为空，等待生产者添加数据后再读取

–  重复4次

说明：

•    显示每次添加和读取数据的时间及缓冲区里的数据（指缓冲区里的具体内容）

•    生产者和消费者用进程模拟（不能用线程）
•   Linux/Windows下都需要做

## 三、实验环境

**Windows环境**

- Windows 11 WorkStation 21H2 

- GCC 11.2.0

**Linux环境**

- Ubuntu 22.04.1
- GCC 11.2.0

## 四、程序设计与实现

将缓冲区看成临界资源，生产者与消费者生产与消费的代码看作临界区。



用信号量解决这个问题，定义如下信号量：

1.互斥信号量mutex，控制生产者与消费者互斥使用缓冲区，初值设为1。

2.同步信号量empty，指示空缓冲区可用数量，当数量为0时，生产者不能继续生产，初值为3。

3.同步信号量full，指示有多少缓冲区装了产品，数量为0时，消费者不能继续消费，初值为0。

另外设置两个指针变量in和out，指示当前消费者和生产者从缓冲区消费和生产的位置



用类C语言代码描述如下：

```c
#define BUF_SIZE 3
semaphore mutex=1, empty=3, full=0;
int in=0, out=0;
int buffer[BUF_SIZE-1];

void producer()
{
    P(empty);
    P(mutex);
    produce a product to buffer[in];
    in = (in+1)%BUF_SIZE;
    V(full);
    V(mutex);
}

void consumer()
{
    P(full);
    P(mutex);
    consume a product from buffer[out];
    out = (out+1)%BUF_SIZE;
    V(empty);
    V(mutex);
}
```



具体实现如下：

### Windows

#### 创建进程

```c
BOOL CreateProcess
(
LPCTSTR lpApplicationName,//以可执行文件路径名的方式
LPTSTR lpCommandLine,//以命令提示符命令的方式
LPSECURITY_ATTRIBUTES lpProcessAttributes,
LPSECURITY_ATTRIBUTES lpThreadAttributes,
BOOL bInheritHandles,
DWORD dwCreationFlags,
LPVOID lpEnvironment,
LPCTSTR lpCurrentDirectory,
LPSTARTUPINFO lpStartupInfo,
LPPROCESS_INFORMATIONlpProcessInformation
);
```

此函数需要传递可执行文件路径，为了方便起见，将代码写在一个文件里，我们采用克隆当前进程的方法，每个进程都执行相同代码。为了让不同进程发挥不同的功能（生产者、消费者、主进程），我们采取在创建进程时修改命令行参数的方式，通过条件语句，控制不同进程执行不同的功能。

StartClone：

```c
void StartClone(int nCloneID) {
	//获取exe文件的完整路径
	TCHAR szFilename[MAX_PATH];
	GetModuleFileName(NULL, szFilename, MAX_PATH);

	//创建cmd命令，并以argv的方式传递nCloneID参数
	TCHAR szCmdLine[MAX_PATH];
	sprintf(szCmdLine, "\"%s\" %d", szFilename, nCloneID);
	//printf("%s\n",szCmdLine);

    //......
    
	//尝试创建新进程，指定可执行文件（自身）
	BOOL bCreateOK = CreateProcess(szFilename, szCmdLine, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi);
    
    //......

}
```

Main函数：

```c
int main(int argc, char* argv[]) {
	int nCloneID = 20;
	
	//子进程启动时含有命令行参数，得到nCloneID，执行生产者消费者函数；主进程直接启动，nCloneID默认为20，执行主控制进程
	if (argc > 1) sscanf(argv[1], "%d", &nCloneID);

	//id为0、1，启动生产者函数
	if (nCloneID < 2) producer(nCloneID);

	//id为2，3，4，启动消费者函数
	else if (nCloneID < 5) consumer(nCloneID);

	//主进程如下
	else {
        
		//......
        
		//通过克隆主进程启动5个子进程，启动什么样的进程由传递的id决定
		for (int i = 0; i < 5; i++)
			StartClone(i);
        
        //......
	}
	return 0;
}
```



进程之间需要共享的值有两类：信号量以及共享内存。共享内存又可分为共享变量（in、out）以及缓冲区。创建在主进程中进行。

#### 信号量

以mutex为例

```c
//主进程创建
HANDLE mutex = CreateSemaphore(nullptr, 1, 1, "mutex");

//producer进程打开并使用
void producer(int id) {
	//通过名称获取创建好的信号量的句柄
	HANDLE mutex = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, "mutex");
    
    //执行信号量P操作
	WaitForSingleObject(mutex, INFINITE);

	//执行信号量V操作
	ReleaseSemaphore(mutex, 1, nullptr);
}
```

#### 共享内存

以缓冲区为例

```c
//定义宏方便使用
#define BUF_SIZE 3
#define CreateSharedMemory(name,size) HANDLE name = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,size,#name)
#define OpenSharedMemory(name) HANDLE name = OpenFileMapping(FILE_MAP_ALL_ACCESS,false,#name)
#define GetVariablePointerFromHandle(handle,type) (type*)MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,sizeof(type))
#define GetMemoryPointerFromHandle(handle,size) (char*)MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,size)

//获取1段共享内存作为缓冲区
CreateSharedMemory(shared_buffer, BUF_SIZE);

//清空缓冲区
memset(MapViewOfFile(shared_buffer, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE), 0, BUF_SIZE);

void producer(int id) {
    //获取共享内存和变量的句柄
    OpenSharedMemory(shared_buffer);
    
    //通过句柄获得指针
    char* buffer = GetMemoryPointerFromHandle(shared_buffer, BUF_SIZE);
}
```

#### 随机值与种子

实验中需要用到随机值（随机等待一段时间）

如果不使用srand()函数设置随机数种子，将使得每次运行程序都生成一样的值



一个通用的做法是使用时间戳设置种子 srand(time(0))

但在该问题中，将并发执行5个进程（2个生产者，3个消费者），由于time(0)以秒为单位，这几个并发进程将设置一样的种子，这样获取的随机数也相同。一个做法是设置srand(time(0) + id * 100)，每个进程有不同的id，这样既保证了每次执行程序种子都不同，也保证了并发进程之间种子也不同。

#### 实验结果

![屏幕截图(133)](.\img\1.png)

#### 完整代码

```c
#include <windows.h>
#include <stdio.h>
#include <time.h>

#define BUF_SIZE 3
#define CreateSharedMemory(name,size) HANDLE name = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,size,#name)
#define OpenSharedMemory(name) HANDLE name = OpenFileMapping(FILE_MAP_ALL_ACCESS,false,#name)
#define GetVariablePointerFromHandle(handle,type) (type*)MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,sizeof(type))
#define GetMemoryPointerFromHandle(handle,size) (char*)MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,size)

HANDLE handleOfProcess[5];
HANDLE handleOfThread[5];

void StartClone(int nCloneID);
void producer(int id);
void consumer(int id);

int main(int argc, char* argv[]) {
	int nCloneID = 20;

	//子进程启动时含有命令行参数，得到nCloneID，执行生产者消费者函数；主进程直接启动，无参数，执行主控制进程
	if (argc > 1) sscanf(argv[1], "%d", &nCloneID);

	//id为0、1，启动生产者进程
	if (nCloneID < 2) producer(nCloneID);

	//id为2，3，4，启动消费者进程
	else if (nCloneID < 5) consumer(nCloneID);

	//主进程如下
	else {
		//创建信号量mutex empty full
		HANDLE mutex = CreateSemaphore(nullptr, 1, 1, "mutex");
		HANDLE empty = CreateSemaphore(nullptr, BUF_SIZE, BUF_SIZE, "empty");
		HANDLE full = CreateSemaphore(nullptr, 0, BUF_SIZE, "full");

		//获取1段共享内存作为缓冲区，同时创建2个共享变量，指向缓冲区读取和写入的位置
		CreateSharedMemory(buf_in, sizeof(int));
		CreateSharedMemory(buf_out, sizeof(int));
		CreateSharedMemory(shared_buffer, BUF_SIZE);

		//清空缓冲区
		memset(MapViewOfFile(shared_buffer, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE), 0, BUF_SIZE);

		//获取变量指针并置内容为0
		int* in_ptr = GetVariablePointerFromHandle(buf_in, int);
		int* out_ptr = GetVariablePointerFromHandle(buf_out, int);
		*in_ptr = 0;
		*out_ptr = 0;

		//通过克隆主进程启动5个子进程，启动什么样的进程由传递的id决定
		for (int i = 0; i < 5; i++)
			StartClone(i);

		//等待5个进程结束
		WaitForMultipleObjects(5, handleOfProcess, TRUE, INFINITE);

		//关闭进程句柄
		for (int i = 0; i < 5; i++) {
			CloseHandle(handleOfProcess[i]);
			CloseHandle(handleOfThread[i]);
		}
	}
	return 0;
}

void StartClone(int nCloneID) {
	//获取exe文件的完整路径
	TCHAR szFilename[MAX_PATH];
	GetModuleFileName(NULL, szFilename, MAX_PATH);

	//创建cmd命令，并以argv的方式传递nCloneID参数
	TCHAR szCmdLine[MAX_PATH];
	sprintf(szCmdLine, "\"%s\" %d", szFilename, nCloneID);
	//printf("%s\n",szCmdLine);

	//创建STARTUPINFO，描述新进程的主窗口特性
	STARTUPINFO si;
	ZeroMemory(reinterpret_cast<void*>(&si), sizeof(si));
	si.cb = sizeof(si);

	//创建PROCESS_INFORMATION，储存新进程以及主线程的信息
	PROCESS_INFORMATION pi;

	//尝试创建新进程，指定可执行文件（自身）
	BOOL bCreateOK = CreateProcess(szFilename, szCmdLine, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi);

	//成功则从PROCESS_INFORMATION取出进程以及相应主线程的句柄保存以便使用
	if (bCreateOK) {
		handleOfProcess[nCloneID] = pi.hProcess;
		handleOfThread[nCloneID] = pi.hThread;
	} else {
		printf("Failed to create process!\n");
		exit(0);
	}
}

void producer(int id) {
	//通过名称获取创建好的信号量的句柄
	HANDLE mutex = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, "mutex");
	HANDLE empty = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, "empty");
	HANDLE full = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, "full");

	//获取共享内存和变量的句柄
	OpenSharedMemory(buf_in);
	OpenSharedMemory(shared_buffer);

	//通过句柄获得指针
	int* in_ptr = GetVariablePointerFromHandle(buf_in, int);
	char* buffer = GetMemoryPointerFromHandle(shared_buffer, BUF_SIZE);

	//设置种子
	srand(time(0) + id * 100);

	time_t t;
	char tmpBuf[10];

	//重复执行6次
	for (int i = 0; i < 6; i++) {
        
		//随机等待0-3秒
		Sleep(rand() % 3000);

		//执行信号量P操作
		WaitForSingleObject(empty, INFINITE);
		WaitForSingleObject(mutex, INFINITE);

		//随机向缓冲区写入数据
		buffer[*in_ptr] = rand() % 128;

		//获取当前时间并输出操作内容
		t = time(0);
		strftime(tmpBuf, 10, "%H:%M:%S", localtime(&t));
		printf("Producer %d writes data: %d to buffer %d at %s.\n", id, buffer[*in_ptr], *in_ptr, tmpBuf);
		fflush(stdout);

		//写指针+1
		*in_ptr = (*in_ptr + 1) % BUF_SIZE;

		//执行信号量V操作
		ReleaseSemaphore(mutex, 1, nullptr);
		ReleaseSemaphore(full, 1, nullptr);
	}
}

void consumer(int id) {

	//通过名称获取创建好的信号量的句柄
	HANDLE mutex = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, "mutex");
	HANDLE empty = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, "empty");
	HANDLE full = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, "full");

	//获取共享内存和变量的句柄
	OpenSharedMemory(buf_out);
	OpenSharedMemory(shared_buffer);

	//通过句柄获得指针
	int* out_ptr = GetVariablePointerFromHandle(buf_out, int);
	char* buffer = GetMemoryPointerFromHandle(shared_buffer, BUF_SIZE);

	//设置种子
	srand(time(0) + id * 100);
	char tmpBuf[10];
	time_t t;
	id = id - 2;
	int data;
	//srand(time(0));

	for (int i = 0; i < 4; i++) {

		//随机等待0-3秒
		Sleep(rand() % 3000);

		//执行信号量P操作
		WaitForSingleObject(full, INFINITE);
		WaitForSingleObject(mutex, INFINITE);

		//从缓冲区中取数据
		data = buffer[*out_ptr];

		//获取当前时间并输出操作内容
		t = time(0);
		strftime(tmpBuf, 10, "%H:%M:%S", localtime(&t));
		printf("Consumer %d reads data: %d from buffer %d at %s.\n", id, data, *out_ptr, tmpBuf);
		fflush(stdout);

		//读指针+1
		*out_ptr = (*out_ptr + 1) % BUF_SIZE;

		//执行信号量V操作
		ReleaseSemaphore(mutex, 1, nullptr);
		ReleaseSemaphore(empty, 1, nullptr);
	}
}
```

### Linux

#### 创建进程

Linux中创建子进程采用函数
```c
pid_t fork(void);
```
函数完全克隆主进程，生成一个子进程，包括各种变量值，生成的是一个独立的内存空间，即复制而不是引用。函数由父子进程共同完成，之后返回不同的值给父子进程。对于子进程，返回0；对于父进程，返回子进程的pid，之后父进程和子进程将并行地从fork()后的代码进行执行。

可以在代码中通过返回值判断当前是父进程还是子进程从而执行不同的代码

```c
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
```

#### 信号量

Linux信号量API中，信号量以数组形式存在，可通过从0开始的下标访问到不同的信号量。进程中，标识信号量数组的为整型值semid，即信号量标识符，唯一关联一个信号量（数组）。



semid通过信号量创建函数得到

```c
int semget(key_t key, int num_sems, int sem_flags);
```

key：IPC对象外部键，在难以联系的多进程间可以通过外部键访问到相同对象

num_sems：要创建的信号量数目

sem_flags：指定创建IPC对象的参数，如IPC_CREAT（一个宏定义，代表一个整型），指定创建函数如果对象已经创建则返回semid，未创建则创建后返回semid



信号量初值可通过信号量控制函数设置

```c
int semctl(int semid,int semnum,int cmd, union semun arg);
```

semid：要控制的信号量标识符

semnum：要控制的信号量数组下标，从0开始

cmd：指定对信号量执行的操作，常用SETVAL，代表设置信号量值

arg：联合类型semun，具体设置联合体中的哪种值由cmd决定

```c
union semun {  
    int              val;    /* Value for SETVAL */  
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */  
    unsigned short  *array;  /* Array for GETALL, SETALL */  
    struct seminfo  *__buf;  /* Buffer for IPC_INFO 

};  
```



信号量PV操作可以通过封装semop函数实现

```c
int semop(int semid, struct sembuf *sem_opa, size_t num_sem_ops);
```

semid：......

sembuf：对信号量进行操作封装的一个结构体

```c
struct sembuf{
    short sem_num; //信号量数组中要修改的信号量下标
    short sem_op; //信号量在一次操作中需要改变的值，通常是两个数，一个是-1，即P操作，一个是+1，即V操作。
    short sem_flg; //操作参数，通常为0
};
```

num_sem_ops：要操作信号量的个数



PV封装如下：其中semid是父子进程都拥有的全局变量，sem_name是通过枚举完成的字符到下标的映射

```c
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
```

```c
//将信号量数组下标0、1、2映射为信号量名称值，便于访问
enum sem_name
{
	mutex,
	empty,
	full
};
```

#### 共享内存

共享内存与信号量很相似，都是IPC对象，即进程间通信对象，相关API参数都差不多，由shm开头的函数操作，shm即shared memory。



创建共享内存可以采用shmget函数

```c
int shmget(key_t key, size_t size, int shmflg);
```

size为共享内存空间大小（字节），函数返回一个shmid，即共享内存标识符，与semid相似。



在C语言代码中，变量由指针进行操控，指针是某类进程中的虚拟地址。shmat函数通过shmid返回一个可以操纵此共享内存的指针。

```c
void *shmat(int shmid, const void *shm_addr, int shmflg);
```

shmid：由shmget函数返回的共享内存标识符

shmaddr：通常为空，大致为绑定

shmflag：类似于semflag

函数返回(void*)，即指向共享内存首地址的指针，可以强制更改类型。



为了保护共享内存空间，通常在使用完内存空间后使用解绑函数，将指针地址与实际虚拟内存空间解除绑定。

```c
int shmdt(const void* shmaddr);
```

#### 实验结果

![屏幕截图(130)](.\img\2.png)

#### 完整代码

```c
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
```

##  五、实验收获与体会

本次实验通过调用Windows、Linux下的系统API，实现了信号量、共享内存等基本的进程间通信，完成了生产者消费者问题。通过调用这些API，能更好地了解这些进程间通信量在系统中的存在、组织方式。

实验开始时调用错了头文件，使用了semaphore.h或semaphore，这也是信号量，但是用于线程之间，要完成进程间通信需要与系统联系更加紧密的系统级API。

在Linux实验中，误将semop中的flag设置成了SEM_UNDO，这引起了严重错误。设置SEM_UNDO将会在当前进程结束后撤销所有对信号量执行的操作，无论进程是否正常退出。其本意是如果某进程对信号量的操作是类似加锁（先申请，后释放），如果不设置SEM_UNDO，在进程申请资源后，意外退出，资源将被占用而得不到释放，设置SEM_UNDO则避免这种情况。但在当前生产者与消费者问题中，生产者只申请empty，只释放full，消费者只申请full，释放empty，如果采用SEM_UNDO，则打乱了这种同步关系，生产者进程正常结束后将会对empty和full进行重置。

