#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/file.h>
#include <string.h>
#include <errno.h>
#include "zlog.h"
#include "dhms_process.h"
#define LOCKFILE "/tmp/dhms_daemon.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
extern dhms_process_t * daemon_list;
extern unsigned int daemon_list_num;


int rc;
zlog_category_t *c;
int task_delay=300;
int lockfile(int fd)
{
	struct flock fl;
	fl.l_type = F_WRLCK;  /* write lock */
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;  //lock the whole file
	return(fcntl(fd, F_SETLK, &fl));
}



int already_running(const char *filename)
{
	int fd;
	char buf[16];
	fd = open(filename, O_RDWR | O_CREAT, LOCKMODE);
	if (fd < 0)
	{
		printf("can't open %s: %m\n", filename);
		exit(1);
	}
	if(lockfile(fd) == -1)
	{
		if(errno == EACCES || errno == EAGAIN)
		{
			printf("file: %s already locked", filename);
			close(fd);
			return 1;
		}

		printf("can't lock %s: %m\n", filename);
		exit(1);
	}
	ftruncate(fd, 0);
	sprintf(buf, "%ld", (long)getpid());
	write(fd, buf, strlen(buf) + 1);
	return 0;
}
static void sig_usr(int signo)
{
	if(!zlog_reload("/etc/zlog.conf"))
	{
		zlog_info(c,"zlog reload sucess.");
	}
	else
	{
		zlog_info(c, "zlog reload error.");
	}
}
static void sig_usr2(int signo)
{
	if(task_delay==300)
	{
			task_delay=5000;
	}
	else
	{
		task_delay=300;
	}
}

static void dhms_daemon()
{
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGUSR1,sig_usr);
    signal(SIGUSR2,sig_usr2);
    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
	{
		printf("EXIT_FAILURE\n");
        exit(EXIT_FAILURE);
	}
    /* Success: Let the parent terminate */
    if (pid > 0)
	{
		printf("EXIT_SUCCESS\n");
        exit(EXIT_SUCCESS);
	}
    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
	#if 0
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }
	#endif
    /* Open the log file */
}

int main()
{
	float time_use=0;
	struct timeval start;
	struct timeval end;
	dhms_daemon();
	if (already_running(LOCKFILE))
	{
		printf("already running\n");
		exit(0);
	}
	rc = zlog_init("/etc/zlog.conf");
	if (rc){
		printf("zlog init failed.\n");
	}
    c = zlog_get_category("dhms_daemon");
    if(!c){
		printf("get category dhms_daemon fail\n");
	}

   	if(parse_json("/private/dhms_conf.json")!=0)
   	{
   		system("/bin/cp /etc/dhms_conf.json /private/dhms_conf.json");
		parse_json("/etc/dhms_conf.json");
	}
//    gettimeofday(&start,NULL);
    while (1)
    {
//		gettimeofday(&end,NULL);
//		time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
//		gettimeofday(&start,NULL);
//		zlog_debug(c,"zlog_debug %f %ld",time_use,end.tv_usec);
//		zlog_error(c,"zlog_error %f %ld", time_use,end.tv_usec);
//		zlog_debug(c, "test");
		dhms_daemon_process(daemon_list);
        sleep(5);
        
    }
    return EXIT_SUCCESS;
}
