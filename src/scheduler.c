#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/time.h>

#ifdef __APPLE__
#include <sys/uio.h>
#else
#include <sys/io.h>
#endif

#define MAX_LINE 1024
#define MAX_CMDS 100

#define IS_BLANK(c) ((c) == ' ' || (c) == '\t')
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_ALPHA(c) ( ((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') )
#define IS_HEX_DIGIT(c) (((c) >= 'A' && (c) <= 'F') || ((c) >= 'a' && (c) <= 'f'))

void init_daemon(void);

void help(void);

void write_pid(int pid);

int read_pid(void);

int process_exist(int pid);

int is_number(char *s);

void init_daemon(void)
{
    pid_t pid;
    int i;
    pid = fork();
    if(pid > 0)
    {
        write_pid(pid);
        exit(0);
    }
    else if(pid < 0 )
    {
        perror("create process failed\n");
        exit(1);
    }
    else if(pid == 0)
    {
        setsid();
        umask(0);
        for(i = 0; i < NOFILE; ++i)
        {
            close(i);
        }
        return;
    }
}

void write_pid(int pid)
{
    FILE *fp;
    fp = fopen("process.pid", "w+");
    if(fp != NULL)
    {
        char char_pid[10];
        sprintf(char_pid, "%d", pid);
        fprintf(fp, "%s", char_pid);
        fclose(fp);
    }
}

int read_pid(void)
{
    FILE *fp;
    fp = fopen("process.pid", "r");
    if( fp != NULL)
    {
        char char_pid[10];
        long int int_pid;
        fgets(char_pid, 10, fp);
        fclose(fp);
        int_pid = atoi(char_pid);
        return int_pid;
    }
    return 0;
}

void help(void)
{
    printf("TEST01 V1.0\n\n");
    printf("Usage: test01 [start|stop]\n");
    return;
}

int process_exist(int pid)
{
    FILE *fp;
    int count;
    char buf[10];
    char command[150];
    sprintf(command, "ps -ef | grep %d | grep -v grep | wc -l", pid);
    count = 0;
    if((fp = popen(command, "r")) != NULL)
    {
        if((fgets(buf, 10, fp)) != NULL)
        {
            count = atoi(buf);
        }
    }
    pclose(fp);
    return count;
}

int is_number(char *s)
{
    int base = 10;
    char *ptr;
    int type = 0;
    if (s == NULL) return 0;
    ptr = s;
    while (IS_BLANK(*ptr))
    {
        ptr++;
    }
    if (*ptr == '-' || *ptr == '+')
    {
        ptr++;
    }
    if (IS_DIGIT(*ptr) || ptr[0] == '.')
    {
        if (ptr[0] != '.')
        {
            if (ptr[0] == '0' && ptr[1] && (ptr[1] == 'x' || ptr[1] == 'X'))
            {
                type = 2;
                base = 16;
                ptr += 2;
            }
            while (*ptr == '0')
            {
                ptr++;
            }
            while (IS_DIGIT(*ptr) || (base == 16 && IS_HEX_DIGIT(*ptr)))
            {
                ptr++;
            }
        }
        if (base == 10 && *ptr && ptr[0] == '.')
        {
            type = 3;
            ptr++;
        }
        while (type == 3 && base == 10 && IS_DIGIT(*ptr))
        {
            ptr++;
        }
        if (*ptr == 0)
            return (type > 0) ? type : 1;
        else
            type = 0;
    }
    return type;
}

int main(int argc, char **argv)
{
    if(argc == 2)
    {
        int readpid;
        readpid = read_pid();
        if(strcmp(argv[1], "start") == 0)
        {
            if(readpid != 0)
            {
                int exist = process_exist(readpid);
                if(exist > 0)
                {
                    printf("The program has already started \n");
                    printf("Pid: %d \n", readpid);
                    printf("If the program does not run, empty file process.pid\n");
                    return 0;
                }
            }

            init_daemon();
            char file_name[11] = "config.ini";
            int file_exists;
            file_exists = access(file_name, 6);
            if(file_exists == -1)
            {
                printf("配置文件%s不存在或不可读\n", file_name);
                return -1;
            }

            // 读取配置文件
            FILE *fp;
            fp = fopen(file_name, "rt");
            if(fp == NULL)
            {
                printf("配置文件无法读取\n");
                return -1;
            }
            // 执行命令
            char *cmds[MAX_CMDS];
            // 执行时间
            char *mss[MAX_CMDS];
            char str_line[MAX_LINE];
            int line = 0;
            while(1)
            {
                if(fgets(str_line, MAX_LINE, fp) == NULL || line > (MAX_CMDS - 1))
                {
                    break;
                }
                for(int i = 0; i < strlen(str_line); i++)
                {
                    // 将换行命令删除
                    if(str_line[i] == '\n' || str_line[i] == '\r')
                    {
                        str_line[i] = 0;
                    }
                }
                char *psplit;
                char *split = ",";
                int split_times = 1;
                psplit = strtok (str_line, split);
                while(psplit != NULL)
                {
                    if(split_times == 1)
                    {
                        int is_num;
                        is_num = is_number(psplit);
                        if(is_num == 1)
                        {
                            mss[line] = malloc(strlen(psplit) + 1);
                            strcpy(mss[line], psplit);
                        }
                        else
                        {
                            printf("ERROR:配置设置不正确,第%d行 : %s\n", line + 1, str_line);
                            break;
                        }
                    }
                    else if(split_times == 2)
                    {
                        cmds[line] = malloc(strlen(psplit) + 1);
                        strcpy(cmds[line], psplit);
                    }
                    else
                    {
                        break;
                    }
                    psplit = strtok (NULL, split);
                    split_times++;
                }
                line++;
            }

            struct timeval tv_s;
            gettimeofday(&tv_s, NULL);
            long int mictime_s;
            mictime_s = tv_s.tv_sec * 1000 + tv_s.tv_usec / 1000;
            long int exec_time[MAX_CMDS] = {0};

            FILE *log;
            log = fopen("crontab.log", "a");
            char log_start[MAX_LINE];
            sprintf(log_start, "\nSTART------------------------------------\nmic : %ld\n", mictime_s);
            fprintf(log, "%s", log_start);
            fclose(log);

            while(1)
            {
                struct timeval tv_e;
                gettimeofday(&tv_e, NULL);

                long int mictime_e;
                mictime_e = tv_e.tv_sec * 1000 + tv_e.tv_usec / 1000;

                for(int i = 0; i < MAX_CMDS; i++)
                {
                    if(cmds[i] == NULL || mss[i] == NULL)
                    {
                        break;
                    }
                    int mictime_diff;
                    int mss_int;
                    mss_int = atoi(mss[i]);
                    if(exec_time[i] == 0)
                    {
                        mictime_diff = mictime_e - mictime_s;
                    }
                    else
                    {
                        mictime_diff = mictime_e - exec_time[i];
                    }
                    if(mictime_diff >= mss_int)
                    {
                        exec_time[i] = mictime_e;
                        int exec_result;
                        exec_result = system(cmds[i]);
                        char log_content[MAX_LINE];
                        sprintf(log_content, "\nmic : %ld\ncmd : %s\ndis : %s\nres : %d\n", mictime_e, cmds[i], mss[i], exec_result);
                        FILE *log;
                        log = fopen("crontab.log", "a");
                        fprintf(log, "%s", log_content);
                        fclose(log);
                    }
                }
                usleep(1000);
            }
        }
        else if(strcmp(argv[1], "stop") == 0)
        {
            if(readpid != 0)
            {
                int exist = process_exist(readpid);
                if(exist > 0)
                {
                    char system_cmd[20];
                    sprintf(system_cmd, "kill %d", readpid);
                    system(system_cmd);
                }
            }
        }
        else
        {
            help();
        }
    }
    else
    {
        help();
    }
    return 0;
}