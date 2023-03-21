#include<iostream>
#include<windows.h>
#include<psapi.h>
#include<tlhelp32.h>
using namespace std;


#define ever (;;) //永久循环
#define br printf("\n") //换行
#define print(x) cout<<x<<endl //输出
#define with_description(description,content) cout<<description<<"\t"<<content<<endl; //带描述的输出
#define with_description_format(description,content) cout<<description<<"\t"<<formatByte(content)<<endl; //格式化数据带描述输出

char buffer[10];

SYSTEM_INFO sys_info;

char* formatByte(DWORDLONG byte_num)
{
	constexpr K(1024);
	constexpr M(K*1024);
	constexpr G(M*1024);

	if(byte_num>=G) sprintf(buffer,"%.1f GB",byte_num/(float)G);
	else if(byte_num>=M) sprintf(buffer,"%.1f MB",byte_num/(float)M);
	else if(byte_num>=K) sprintf(buffer,"%.1f KB",byte_num/(float)K);
	else sprintf(buffer,"%.1f B",byte_num/1.0);
	return buffer;
}

void PrintSystemMemoryInfo()
{
	
	
	PERFORMANCE_INFORMATION per_info;
	GetPerformanceInfo(&per_info,sizeof(PERFORMANCE_INFORMATION));
	
	MEMORYSTATUSEX mem_stat;
	mem_stat.dwLength=sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&mem_stat);
	
	with_description_format("分页大小",sys_info.dwPageSize);
	with_description_format("系统总内存",mem_stat.ullTotalPhys);
	with_description_format("虚拟内存",mem_stat.ullTotalVirtual);
	with_description_format("内核占用内存",per_info.KernelTotal*sys_info.dwPageSize);
	with_description_format("当前可用内存",mem_stat.ullAvailPhys);
	with_description("当前进程数",per_info.ProcessCount);
	with_description("当前线程数",per_info.ThreadCount);
	with_description("已打开句柄",per_info.HandleCount);
	
}
	

void CheckRunningProcesses()
{
	
	HANDLE hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

	PROCESSENTRY32 process_entry;
	process_entry.dwSize=sizeof(process_entry);
	BOOL hasNextProcess = Process32First(hProcessSnapshot,&process_entry);
	char title[15]="进程名";
	printf("PID\t%-38s占用内存\n",title);br;
	PROCESS_MEMORY_COUNTERS process_mem;
	//ZeroMemory(&process_mem,sizeof(process_mem));
	HANDLE hProcess;
	
	while(hasNextProcess)
	{
		hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,process_entry.th32ProcessID);
		if(GetProcessMemoryInfo(hProcess,&process_mem,sizeof(process_mem)))
		{
			printf(
				"%lu\t%-38s%s\n",
				process_entry.th32ProcessID,
				process_entry.szExeFile,
				formatByte(process_mem.WorkingSetSize)
			);
		}
		hasNextProcess = Process32Next(hProcessSnapshot,&process_entry);
		CloseHandle(hProcess);
	} 
	CloseHandle(hProcessSnapshot);
}

void PrintMemoryInfo(LPBYTE start,LPBYTE next, char* size, char *state, char* protect, char* type)
{
	
	if(start==0)
	{
		char region[20]="虚拟地址区间";
		printf("%-40s",region);
	}
	else printf("%p-%p\t",start,next-1);
	printf("%-16s",size);
	printf("%-16s",state);
	printf("%-20s",protect);
	printf("%s\n",type);
}

void PrintProcessAddressDistribution(DWORD pid)
{
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,pid);
	if(hProcess==NULL)
	{
		printf("PID无效\n");
		return;
	}
	
	MEMORY_BASIC_INFORMATION memory_info;
	
	LPBYTE address_start = (LPBYTE)sys_info.lpMinimumApplicationAddress;
	LPBYTE address_next;
	char state[10];
	char protect[20];
	char type[10];
	
	PrintMemoryInfo(
		0,
		0,
		"页面大小",
		"页面状态",
		"保护方式",
		"页面类型");
	
	printf("\n");
	
	while(address_start<sys_info.lpMaximumApplicationAddress)
	{
		if(VirtualQueryEx(hProcess,address_start,&memory_info,sizeof(memory_info))!=sizeof(memory_info)) 
			printf("error\n");
		else
		{
			address_next = address_start+memory_info.RegionSize;

			switch (memory_info.State) {
			case MEM_COMMIT:
				sprintf(state,"已提交");
				break;
			case MEM_FREE:
				sprintf(state,"未分配");
				break;
			case MEM_RESERVE:
				sprintf(state,"保留");
				break;
			default:
				sprintf(state,"未知");
			}
			switch (memory_info.Protect) {
			case PAGE_NOACCESS:
				sprintf(protect,"无访问权限");
				break;
			case PAGE_READONLY:
				sprintf(protect,"只读");
				break;
			case PAGE_GUARD|PAGE_READWRITE:
				sprintf(protect,"单次保护可读可写");
				break;
			case PAGE_READWRITE:
				sprintf(protect,"可读可写");
				break;
			case PAGE_EXECUTE_READ:
				sprintf(protect,"可执行读取");
				break;
			case PAGE_WRITECOPY:
				sprintf(protect,"拷贝写入");
				break;
			case 0:
				sprintf(protect,"未定义");
				break;
			default:
				sprintf(protect,"%lx",memory_info.Protect);
				break;
			}
			switch (memory_info.Type) {
			case MEM_IMAGE:
				sprintf(type,"镜像");
				break;
			case MEM_MAPPED:
				sprintf(type,"文件映射");
				break;
			case MEM_PRIVATE:
				sprintf(type,"进程私有");
				break;
			default:
				sprintf(type,"未定义");
			}
			
			PrintMemoryInfo(
				address_start,
				address_next,
				formatByte(memory_info.RegionSize),
				state,
				protect,
				type);
		}
		address_start=address_next;
	}
	CloseHandle(hProcess);
}

int main()
{
	GetSystemInfo(&sys_info);
	int func;
	int pid;
	for ever
	{
		printf("内存监视器\n\n");
		printf("0 内存总览\n");
		printf("1 运行进程信息\n");
		printf("2 查看进程详细信息\n");
		printf("3 退出\n");
		br;
		printf("查询内容：");
		scanf("%d",&func);
		br;
		switch (func) {
		case 0:
			PrintSystemMemoryInfo();
			break;
		case 1:
			CheckRunningProcesses();
			break;
		case 2:
			printf("请输入PID：");
			scanf("%d",&pid);
			br;
			PrintProcessAddressDistribution(pid);
			break;
		case 3:
			return 0;
		default:
			printf("无效，请重新输入\n");
		}
		
		br;br;
	}
}
