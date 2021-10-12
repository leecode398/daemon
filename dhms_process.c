
#include "zlog.h"
#include "dhms_process.h"
#include<unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/prctl.h>
#include <signal.h>
#include <dirent.h>

#define VERSION_FILE_NAME "/tmp/dhms_version"
#define ONE_ROW_MAX_CHR 128
#define MAX_RETRY_COUNT 10
extern zlog_category_t *c;
extern char **environ;

dhms_process_t * daemon_list = NULL;
unsigned int daemon_list_num = 0;
dhms_process_t default_dhms_process_list[]={
	{
		.path="/system/bin/redis-server",
		.arg=0,
		.env=0,
		.cmd="-v | awk '{print $3}' | awk -F '=' '{print $2}'",
		.ver=NULL,
		.arglen=0,
		.envlen=0,
		.retry_count=0,
		.pid=0
	},
	{
		.path="/system/bin/tum_keyMonitor",
		.arg=0,
		.env=0,
		.cmd="-v | head -1 | awk '{print $3}'",
		.ver=NULL,
		.arglen=0,
		.envlen=0,
		.retry_count=0,
		.pid=0
	},
	{
		.path="/system/bin/sessiongo",
		.arg=0,
		.env=0,
		.cmd="--version | sed -n '4p'",
		.ver=NULL,
		.arglen=0,
		.envlen=0,
		.retry_count=0,
		.pid=0
	},
	{
		.path="/system/bin/tum_consumer",
		.arg=0,
		.env=0,
		.cmd="-v | awk '{print $3}'",
		.ver=NULL,
		.arglen=0,
		.envlen=0,
		.retry_count=0,
		.pid=0
	},
	{
		.path="/system/bin/tum_producer",
		.arg=0,
		.env=0,
		.cmd="-v | awk '{print $3}'",
		.ver=NULL,
		.arglen=0,
		.envlen=0,
		.retry_count=0,
		.pid=0
	},
	{
		.path="/system/bin/tum_controller",
		.arg=0,
		.env=0,
		.cmd="-v | awk '{print $3}'",
		.ver=NULL,
		.arglen=0,
		.envlen=0,
		.retry_count=0,
		.pid=0
	},
	{
		.path="/system/bin/dota_linux_arm",
		.arg=0,
		.env=0,
		.cmd="--version | sed -n '4p'",
		.ver=NULL,
		.arglen=0,
		.envlen=0,
		.retry_count=0,
		.pid=0
	},
	{
		.path="/system/bin/netmonitor",
		.arg=0,
		.env=0,
		.cmd="-v | awk '{print $3}'",
		.ver=NULL,
		.arglen=0,
		.envlen=0,
		.retry_count=0,
		.pid=0
	}
};
void getPidByName(pid_t *pid, char *task_name)
 {
     DIR *dir;
     struct dirent *ptr;
     FILE *fp;
     char filepath[50];
     char cur_task_name[50];
     char buf[1024];
	 buf[1023] = 0;
 	 *pid = 0;
     dir = opendir("/proc"); 
     if (NULL != dir)
     {
         while ((ptr = readdir(dir)) != NULL) 
         {
             
             if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
                 continue;
             if (DT_DIR != ptr->d_type)
                 continue;
            
             sprintf(filepath, "/proc/%s/status", ptr->d_name);
             fp = fopen(filepath, "r");
             if (NULL != fp)
             {
                 if( fgets(buf, 1023, fp)== NULL ){
                     fclose(fp);
                     continue;
                 }
                 sscanf(buf, "%*s %s", cur_task_name);
         
                 //如果文件内容满足要求则打印路径的名字（即进程的PID）
                 if (!strcmp(task_name, cur_task_name)){
                     sscanf(ptr->d_name, "%d", pid);
                 }
                 fclose(fp);
             }
         }
         closedir(dir);
     }
 }
 
 void getNameByPid(pid_t pid, char *task_name) {
    char proc_pid_path[1024];
    char buf[1024];
	buf[1023] = 0;
    sprintf(proc_pid_path, "/proc/%d/status", pid);
    FILE* fp = fopen(proc_pid_path, "r");
    if(NULL != fp){
        if( fgets(buf, 1023, fp)== NULL ){
            fclose(fp);
        }
        fclose(fp);
        sscanf(buf, "%*s %s", task_name);
    }
 }
 
static int update_version(char *path,char *version)
{
	char filename[] = VERSION_FILE_NAME; 
	FILE* fp;
	char buf[ONE_ROW_MAX_CHR];
	long int seek = 0;
	
	fp = fopen(filename, "r+");
	if (fp == NULL) {
		fp = fopen(filename, "w+");
	}
	rewind (fp);
	seek = ftell(fp);
	
	while(fgets(buf, ONE_ROW_MAX_CHR, fp) != NULL)
	{
		if(strstr(buf,path)==NULL)
		{
			seek = ftell(fp);
		}
		else
		{
			fseek(fp,seek,SEEK_SET);
			fprintf(fp,"%-64s%-64s\n",path,version);
			fclose(fp);
			return 0; 
		}
	}
	fprintf(fp,"%-64s%-64s\n",path,version);
	fclose(fp);
	return 0;
}

static void kill_process(pid_t pid)
{
	char shell_cmd[256];
	sprintf(shell_cmd,"kill -9 %d\n",pid);
	system(shell_cmd);
}
static void check_process_run_sta(char * pname)
{
	pid_t pid;
	getPidByName(&pid, pname);
	if(pid == 0)return;
	kill_process(pid);
	
}
static int run_shell(char *cmdstring, char **buf, int *len)//注意:buf此函数内仅分配内存,使用完毕后需要手动释放buf
{
	int   fd[2];
    pid_t pid;
	char data[4096];
    memset(data, 0, 4096);
	
	if (pipe(fd) < 0) 
    {
        return -1;
    }
	pid = fork();
    if (pid < 0) 
    {
         return -1;
    }
    if (pid > 0)   
	{
		close(fd[1]);    
        *len = read(fd[0], data, 4095)+1;
		if(data[*len]=='\n')
			data[*len] = 0;
        *buf=(char *)malloc(sizeof(char)*(*len));
//		zlog_info(c,"%s ver:%s.",cmdstring,data);
		memcpy(*buf,data,*len);
        close(fd[0]);
        if (waitpid(pid, NULL, 0) > 0)
        {
            return -1;
        }
    }
    if(pid == 0)           
    {
        close(fd[0]);  
        if (fd[1] != STDOUT_FILENO)
        {
            if (dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO)
            {
                return -1;
            }
            close(fd[1]);
        } 
        if (execl("/bin/sh", "sh", "-c", cmdstring, (char*)0) == -1)
        {
            return -1;
        }
    } 
    return 0;
}
inline void get_process_verison(char * cmd,char * *version,int *version_len)
{
	run_shell(cmd,version,version_len);
}
static int backup_process(char * process_path)
{
	int ret = 0;
	char backup_process_path[128]={0};
	sprintf(backup_process_path,"%s.old",process_path);
	ret = rename(process_path,backup_process_path);
	return ret;
}
static int recovery_process(char * process_path)
{
	char backup_process_path[128]={0};
	sprintf(backup_process_path,"%s.old",process_path);
	if(access(backup_process_path, F_OK)==0)
	{
		remove(process_path);
		rename(backup_process_path,process_path);
		chmod(process_path, 777);
		zlog_info(c,"%s recovery.",process_path);
	}
}
static int check_update_process(char * process_path)
{
	char new_process_path[128]={0};
	sprintf(new_process_path,"%s.new",process_path);
	if(access(new_process_path, F_OK)==0)
	{
		backup_process(process_path);
		rename(new_process_path,process_path);
		chmod(process_path, 777);
		zlog_info(c,"%s update.",process_path);
		return 1;
	}
	return 0;
}
static void run_process(dhms_process_t * dhms_process)
{
	pid_t pid=0;;
	char ** dhms_env = NULL;
	unsigned int i = 0;
	pid = fork();
	
	if(pid == 0)
	{
		prctl(PR_SET_PDEATHSIG,SIGKILL);
		dhms_env = dhms_process->env;
		for(i=0; i<dhms_process->envlen; i++ )
		{
			putenv(dhms_env[i]);
		}
		if(execve(dhms_process->path, dhms_process->arg ,environ)==-1)
		{
			zlog_error(c,"execve %s.",strerror(errno));
			zlog_error(c,"dhms_process->path %s.",dhms_process->path);
			zlog_error(c,"dhms_process->arg:");
			for(i=0; i<(dhms_process->arglen+1); i++)
			{
				zlog_error(c,"\t%s",dhms_process->arg[i]);
			}
			zlog_error(c,"environ:");
			dhms_env = environ;
			while((*dhms_env)!=NULL)
			{
				zlog_error(c,"\t%s",*dhms_env);
				dhms_env++;
			}
			
		}
		exit(EXIT_SUCCESS);
	}
	if(pid >0)
	{
		zlog_info(c,"%s PID:%d",dhms_process->path,pid);
		dhms_process->pid = pid;
	}
}

static void dhms_record_version(dhms_process_t *dhms_process)
{
	int ver_len = 0;
	if(dhms_process->ver!=NULL)
	{
		free(dhms_process->ver);
		dhms_process->ver = NULL;
	}
	get_process_verison(dhms_process->cmd, &(dhms_process->ver), &ver_len);
	update_version(dhms_process->path,dhms_process->ver);
}


void dhms_daemon_process(dhms_process_t * dhms_process_list)
{
	unsigned int i=0; 
	unsigned int j=0;
	pid_t wpid = 0;
	if(dhms_process_list == NULL)
	{
		dhms_process_list =default_dhms_process_list;
	}
	for(i=0; i<daemon_list_num; i++)
	{
		if(dhms_process_list[i].pid == 0)
		{
			zlog_warn(c,"%s not runing,now run it.",dhms_process_list[i].path);
			check_update_process(dhms_process_list[i].path);
			dhms_record_version(dhms_process_list+i);
			run_process(dhms_process_list+i);
			dhms_process_list[i].retry_count = 1;
			continue;
		}
		wpid = waitpid(dhms_process_list[i].pid, NULL, WNOHANG);
		if(wpid == dhms_process_list[i].pid || wpid == -1)
		{
		//	dhms_process_list[i].pid = 0;
			zlog_error(c,"%s exit,retry count %d",dhms_process_list[i].path,dhms_process_list[i].retry_count);
			check_process_run_sta(dhms_process_list[i].arg[0]);
			
			if(dhms_process_list[i].retry_count >= MAX_RETRY_COUNT)
			{
				dhms_process_list[i].retry_count = 1;
				recovery_process(dhms_process_list[i].path);
			}
			else
			{
				if(check_update_process(dhms_process_list[i].path))
				{
					dhms_process_list[i].retry_count = 1;
				}
			}
			dhms_record_version(dhms_process_list+i);
			run_process(dhms_process_list+i);
			dhms_process_list[i].retry_count ++;

		}
	}
	
}
void init_daemon_list(unsigned int list_size)
{
	if(list_size == 0)
	{
		daemon_list = NULL;
	}
	daemon_list = (dhms_process_t *)malloc(list_size * sizeof(dhms_process_t));
}

