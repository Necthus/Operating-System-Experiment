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

