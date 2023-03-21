#include<unistd.h>
#include<string>
#include<errno.h>
#include<string.h>
#include<iostream>
#include<dirent.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/time.h>
#include<utime.h>
using namespace std;

char buf[BUFSIZ];

void CopyAttribute(const char *src,const char * dst)
{
    //获取状态
    struct stat file_stat;
    lstat(src,&file_stat);
    //修改mode和owner
    chmod(dst,file_stat.st_mode);
    chown(dst,file_stat.st_uid,file_stat.st_gid);
    if(S_ISLNK(file_stat.st_mode))
    {
        struct timeval file_time[2];
        file_time[0].tv_sec=file_stat.st_atime;
        file_time[1].tv_sec=file_stat.st_atime;
        //不修改微秒容易造成errno 2: Invalid Arguement错误，这是因为溢出的缘故
        file_time[0].tv_usec=0;
        file_time[1].tv_usec=0;
        lutimes(dst,file_time);
    }
    else
    {
        utimbuf file_time={file_stat.st_atime,file_stat.st_mtime};
        utime(dst,&file_time);
    }
}

void CopyFile(const char* src,const char* dst)
{
    const int src_fd = open(src,O_RDONLY);
    const int dst_fd = open(dst,O_RDWR | O_CREAT|O_TRUNC,0777);
    int len;
    //读取多少就写入多少，读取不出来则停止循环
    while((len=read(src_fd,buf,sizeof(buf)))>0)
    {
        write(dst_fd,buf,len);
    }
    //关闭描述符
    for(auto &fd:{src_fd,dst_fd}) close(fd);
    return;
}

void Traverse(string src,string dst)
{
    DIR *dir = opendir(src.c_str());
  
    struct dirent* p_dir_entry;
    struct stat file_stat;

    while((p_dir_entry=readdir(dir))!=NULL)
    {
        string name = p_dir_entry->d_name;
        string full_src_name = src+name;
        string full_dst_name = dst+name;
        if(name=="."||name=="..") continue;
        lstat(full_src_name.c_str(),&file_stat);
        //是文件夹
        if(S_ISDIR(file_stat.st_mode))
        {
            //创建并遍历
            mkdir(full_dst_name.c_str(),0777);
            Traverse(full_src_name+"/",full_dst_name+"/");
        }
        //是软链接
        else if(S_ISLNK(file_stat.st_mode))
        {
            int end=readlink(full_src_name.c_str(),buf,sizeof(buf));
            buf[end]=0;
            symlink(buf,full_dst_name.c_str());
        }
        //是普通文件
        else
        {
            CopyFile(full_src_name.c_str(),full_dst_name.c_str());
        }
        CopyAttribute(full_src_name.c_str(),full_dst_name.c_str());
    }
}

// void ValidPath(string &path)
// {
//     if(path[0]=='~')
//     {
//         string home = getenv("HOME");
//         path =home+path.substr(1);
//     }
// }


int main(int argc, char const *argv[])
{
    //判断参数个数
    if(argc!=3)
    {
        cout<<"Arguements error"<<endl;
        return 0;
    }
    string src = argv[1];
    string dst = argv[2];
    // string src="../Y8664/";
    // string dst="~/same/";
    // ValidPath(src);
    // ValidPath(dst);


    //不存在目标文件夹则创建
    mkdir(dst.c_str(),0777);
    //遍历
    Traverse(src,dst);
    //复制属性
    CopyAttribute(src.c_str(),dst.c_str());
    return 0;
}
