#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// 实现一行的读入：每次读入一个字符，直到'\n'
char *readLine()
{
    char *buf = malloc(100);
    char *p = buf;

    while (read(0, p, 1) != 0)
    {
        if (*p == '\n' || *p == '\0')
        {
            *p = '\0';
            return buf;
        }
        p++;
    }
    if (p != buf)
        return buf;

    free(buf);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) // 判断使用是否正确
    {
        printf("Usage: xargs [command]\n");
        exit(1);
    }

    argv++;
    char *ARGV[16]; // 用来记录新的参数数组
    char **p1 = ARGV;
    char **p2 = argv;
    while (*p2 != 0)
    {
        *p1 = *p2;
        p1++;
        p2++;
    }

    char *line;
    while ((line = readLine()) != 0)
    {
        char *p = line;
        char *buf = malloc(36);
        char *pb = buf;

        // 每行可能包含多个参数，逐个字符读入，并根据空格判断
        int ARGC = argc - 1;
        while (*p != 0)
        {
            if (*p == ' ' && buf != pb) // 另一个参数
            {
                *pb = 0;
                ARGV[ARGC] = buf;
                buf = malloc(36);
                pb = buf;
                ARGC++; //参数个数+1
            }
            else
            {
                *pb = *p;
                pb++;
            }
            p++;
        }
        if (buf != pb)
        {
            ARGV[ARGC] = buf;
            ARGC++;
        }
        ARGV[ARGC] = 0;
        free(line);

        // 创建新进程并通过调用exec执行一条命令
        int pid = fork();
        if (pid == 0)
            exec(ARGV[0], ARGV);
        else
            wait(0);
    }
    exit(0);
}
