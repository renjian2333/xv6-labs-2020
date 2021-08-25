#include "kernel/types.h"
#include "user/user.h"

//fprintf是将字符输出到流（文件）的，printf是输出到标准输出设备（stdout）的

int main(int argc,char *argv[])
{
    //判断参数数量是否正确
    if(argc!=2){
        fprintf(2,"Usage: sleep <number>\n");
        exit(1);
    } 
    //将字符串转换为数字
    int time=atoi(argv[1]);
    printf("Nothing happens for a while!\n");
    //系统调用
    sleep(time);
    exit(0);
}