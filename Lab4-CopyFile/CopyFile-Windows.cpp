#include<iostream>
#include<windows.h>
#include<string>
using namespace std;

void Traverse(string src,string dst);

void CopyAttribute(string src,string dst,HANDLE hSrc,HANDLE hDst)
{
	DWORD attribute = GetFileAttributes(src.c_str());
	SetFileAttributes(dst.c_str(),attribute);
	FILETIME creation_time,access_time,write_time;
	if(!GetFileTime(hSrc,&creation_time,&access_time,&write_time))
	{
		cout<<src<<" get file time error."<<endl;
	}
	if(!SetFileTime(hDst,&creation_time,&access_time,&write_time))
	{
		cout<<dst<<" set file time error."<<endl;
	}
	return;
}

void MyCopyDirectory(string src,string dst)
{
	CreateDirectory(dst.c_str(),NULL);
	HANDLE hSrcDir=CreateFile(
		src.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL
		);
	
	HANDLE hDstDir=CreateFile(
		dst.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL
		);
	//递归调用Traverse函数
	Traverse(src+"\\",dst+"\\");
	//先调用再复制属性
	CopyAttribute(src,dst,hSrcDir,hDstDir);
	CloseHandle(hSrcDir);
	CloseHandle(hDstDir);
	return ;
}

void MyCopyFile(string src,string dst)
{
	//打开已存在源文件
	HANDLE hSrcFile=CreateFile(
		src.c_str(),
		GENERIC_READ,
		0,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
		);
	//打开或创目标建文件
	HANDLE hDstFile=CreateFile(
		dst.c_str(),
		GENERIC_READ|GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
		);
	//获取文件大小
	DWORD file_size = GetFileSize(hSrcFile,nullptr);
	//创建缓冲区
	LPVOID buffer = malloc(file_size);
	DWORD read_num=0;
	//从源文件读写到目标文件
	ReadFile(hSrcFile,buffer,file_size,&read_num,nullptr);
	WriteFile(hDstFile,buffer,file_size,&read_num,nullptr);
	//复制属性和时间
	CopyAttribute(src,dst,hSrcFile,hDstFile);
	CloseHandle(hSrcFile);
	CloseHandle(hDstFile);
	free(buffer);
	return ;
}



void Traverse(string src,string dst)
{
	//文件数据
	WIN32_FIND_DATA file_data;
	//获取第一个文件句柄
	HANDLE hFirstFile=FindFirstFile((src+"*").c_str(),&file_data);
	if(hFirstFile==INVALID_HANDLE_VALUE) printf("error\n");
	do
	{
		//获取文件名或目录名
		string name = file_data.cFileName;
		//如果是当前目录或者上级目录则跳过
		if(name=="."||name=="..") continue;
		//名称的完整路径
		string src_full_name = src+name;
		string dst_full_name = dst+name;
		//是目录
		if(file_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		{
			//复制目录
			MyCopyDirectory(src_full_name,dst_full_name);
		}
		else
		{
			//否则复制文件
			MyCopyFile(src_full_name,dst_full_name);
		}
	//循环结束，获得下一文件数据，获取不成功退出循环
	}while(FindNextFile(hFirstFile,&file_data));
	//关闭首文件句柄
	FindClose(hFirstFile);
	return ;
}



int main(int argc,char *argv[])
{
	
	if(argc!=3)
	{
		printf("Arguements Error\n");
		return 0;
	}
	//获取输入输出路径
	string src=argv[1];
	string dst=argv[2];
	CreateDirectory(dst.c_str(),NULL);
	//调用遍历函数
	Traverse(src,dst);
	return 0;
}



