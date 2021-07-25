#include "kernel/types.h"
#include "user/user.h"

void prime(int fd)
{
    int n;
    read(fd, &n, 4);
    printf("prime %d\n", n); //读到的第一个数字一定是prime

    int p[2];

    int created = 0;
    int k;

    while (read(fd, &k, 4) == 4)
    {
        if (created == 0)
        {
            created = 1;
            pipe(p);
            int pid = fork();
            if (pid == 0)
            {
                close(p[1]);
                prime(p[0]);
            }
            else
            {
                close(p[0]);
            }
        }
        if (k % n != 0)
        {
            if (write(p[1], &k, 4) != 4)
            {
                exit(1);
            }
        }
    }

    close(fd);
    close(p[1]);
    wait(0);
}

// 父进程将所有数字写入管道中，后面的子进程都采取同样操作
// 先从上一级读入一个数字，该数字一定是prime，然后继续读入并根据是否为第一个数的倍数
// 来决定是舍弃还是向后传进下一个进程
int main()
{
    int p[2];
    pipe(p);
    for (int i = 2; i <= 35; i++)
    {
        if (write(p[1], &i, 4) != 4)
        {
            fprintf(2, "Error: write1 failed!!\n");
            exit(1);
        }
    }
    close(p[1]);
    prime(p[0]);
    close(p[0]);

    exit(0);
}