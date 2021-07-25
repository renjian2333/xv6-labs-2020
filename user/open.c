
// 1. copy.c: copy input to output.
/*
#include "kernel/types.h"
#include "user/user.h"

int
main()
{
  char buf[64];
  //Shell会确保默认情况下，当一个程序启动时，文件描述符0连接到
  //console的输入，文件描述符1连接到了console的输出。

  while(1){
    //read的返回值可能是读到的字节数；
    //如果读到了文件末尾没有更多内容了，read会返回0
    //如果出现错误，比如文件描述符不存在，read或许会返回-1
    int n = read(0, buf, sizeof(buf));
    if(n <= 0)
      break;
    write(1, buf, n);
  }

  exit(0);
}
*/


// 2. open.c: create a file, write to it.
/*
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main()
{
  int fd=open("output.txt",O_WRONLY|O_CREATE);
  write(fd,"meishi\n",7);
  exit(0);
}
*/

// 3. fork.c: create a new process
/*
#include "kernel/types.h"
#include "user/user.h"

int main()
{
  int pid;
  pid=fork();

  //fork会拷贝当前进程的内存，并创建一个新的进程，这里的内存包含了进程的指令和数据。
  //父进程和子进程的输出交替进行，输出结果具有不确定性
  //在父进程中fork()返回子进程的pid
  //在子进程中fork()返回0
  printf("fork() returned %d\n",pid);

  if(pid==0){
    printf("child\n");
  }else {
    printf("parent\n");
  }

  exit(0);
}*/

// 4. exec.c: replace aprocess with an executable file
/*
#include "kernel/types.h"
#include "user/user.h"

int main()
{
  char *argv[]={"echo","this","is","echo",0};
  //exec这个系统调用会从指定的文件中读取并加载指令，并替代当前当前调用进程的指令
  //exec系统调用并没有创建新的进程，只是替换了原来进程上下文的内容。
  //通常来说exec系统调用不会返回，它只会在kernel不能运行相应的文件时返回。
  //例如程序文件根本不存在，因为exec系统调用不能找到文件，exec会返回-1来表示
  exec("echo",argv);
  printf("exec failed!\n");
  exit(0);
}*/


// 5. forkexec.c: fork then exec
/*
#include "kernel/types.h"
#include "user/user.h"

int main()
{
  int pid, status;

  pid = fork();
  if(pid == 0){
    char *argv[] = { "echo", "THIS", "IS", "ECHO", 0 };
    exec("echo", argv);
    //在子进程中，若exec正常执行下面两行代码不会运行
    printf("exec failed!\n");
    exit(1);
  } else {
    printf("parent waiting\n");
    //wait等待一个子进程结束
    //Unix中的风格是，如果一个程序成功的退出了，那么exit的参数会是0
    //如果出现了错误，会向exit传递1
    //如果你关心子进程的状态的话，父进程可以读取wait的参数，并决定子进程是否成功的完成了。
    wait(&status);
    printf("the child exited with status %d\n", status);
  }

  exit(0);
}*/

// 6. redirect.c: run a command with output redirected
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main()
{
  int pid;

  pid = fork();
  if(pid == 0){
    //将子进程的输出文件描述符1指向output文件
    //父进程的文件描述符1没有发生改变
    //open会返回当前进程未使用的最小文件描述符序号。因为我们刚刚关闭了文件描述符1，
    //而文件描述符0还对应着console的输入，所以open一定可以返回1。
    close(1);
    open("output.txt", O_WRONLY|O_CREATE);

    char *argv[] = { "echo", "this", "is", "redirected", "echo", 0 };
    exec("echo", argv);
    printf("exec failed!\n");
    exit(1);
  } else {
    wait((int *) 0);
  }

  exit(0);
}
