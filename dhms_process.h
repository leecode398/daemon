#ifndef dhms_daemon_h
#define dhms_daemon_h
#include<sys/types.h>
#define DAEMON_LIST_MAX 64

typedef struct
{
	char * path;
	char ** arg;
	char ** env;
	char * cmd;
	char * ver;
	unsigned int arglen;
	unsigned int envlen;
	unsigned int retry_count;
	pid_t pid;
}dhms_process_t;


typedef struct
{
	dhms_process_t * daemon;
	int process_count;
}dhms_process_manage_t;

void dhms_daemon_process(dhms_process_t * dhms_process_list);
void init_daemon_list(unsigned int list_size);


#endif
