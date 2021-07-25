#include "kernel/types.h"
#include "user/user.h"

int main(int argc, int *argv[])
{
    int p1[2],p2[2];
    char buffer[]={'X'};
    int n=sizeof(buffer);

    //父传子的管道
    pipe(p1);
    //子传父的管道
    pipe(p2);

    int pid=fork();

    if(pid==0){
        //关闭无用的端口
        close(p1[1]);
        close(p2[0]);

        //子进程向pipe2的写端写入数据
        if(write(p2[1],buffer,n)!=n){
            printf("Error: Child write to parent failed!\n");
            exit(1);
        }

        //子进程从pipe1的读端获取数据
        if(read(p1[0],buffer,n)!=n){
            printf("Error: Child read from parent failed!\n");
            exit(1);
        }

        printf("%d: received ping\n",getpid());

        exit(0);
    }
    else {
        close(p1[0]);
        close(p2[1]);

        //父进程向pipe1的写端写入数据
        if(write(p1[1],buffer,n)!=n){
            printf("Error: Parent write to child falied!\n");
            exit(1);
        }

        //等待子进程退出
        wait(0);

        //父进程从pipe2的读端获取数据
        if(read(p2[0],buffer,n)!=n){
            printf("Error: Parent read from child failed!\n");
            exit(1);        
        }

        printf("%d: received pong\n",getpid());
        
        exit(0);

    }
}