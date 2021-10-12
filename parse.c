
#include "zlog.h"
#include "dhms_process.h"
#include "parse.h"
#include<unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

extern dhms_process_t * daemon_list;
extern unsigned int daemon_list_num;
extern zlog_category_t *c;

char * endl = 0;
void replace_daemon_list(char ** value,char * src)
{
	unsigned int tmp_len = 0;
	tmp_len=strlen(src)+1;
	*value=(char *)malloc(tmp_len*sizeof(char));
	(*value)[tmp_len] = 0;
	strcpy(*value,src);
	
}

static void parse_daemon_list(cJSON* root)
{
	unsigned int array_size = 0,i = 0, j = 0;
	unsigned int tmp_len = 0;
	cJSON *daemon_array = cJSON_GetObjectItem( root, "app");
	cJSON * aSub;
	cJSON * path,* arg,* env,* version_cmd;
	cJSON * pSub;
	if( daemon_array != NULL ){
		array_size = cJSON_GetArraySize(daemon_array);
	 	if(array_size == 0)
	 	{
			daemon_list == NULL;
			zlog_warn(c,"json array size is 0,use default list.");
			return;
		}
		init_daemon_list(array_size);
		daemon_list_num = array_size;
		for( i = 0; i < array_size;  i++ )
		{
			daemon_list[i].retry_count=1;
			daemon_list[i].pid=0;
			aSub = cJSON_GetArrayItem(daemon_array, i);
			if( aSub == NULL )
			{			
				continue;
			}
	 		path = cJSON_GetObjectItem(aSub,"path");
			replace_daemon_list(&(daemon_list[i].path),path->valuestring);
			zlog_debug(c,"daemon_list[%d].path=%s",i,daemon_list[i].path);
			arg = cJSON_GetObjectItem(aSub,"arg");
			daemon_list[i].arglen = cJSON_GetArraySize (arg);
			zlog_debug(c,"daemon_list[%d].arglen=%d",i,daemon_list[i].arglen);
			if(daemon_list[i].arglen > 0)
			{
				daemon_list[i].arg = (char **)malloc(sizeof(char *)*(daemon_list[i].arglen+1));
				for(j = 0; j <daemon_list[i].arglen; j++)
				{
					pSub = cJSON_GetArrayItem(arg, j);
					replace_daemon_list(&(daemon_list[i].arg[j]),pSub->valuestring);
					zlog_debug(c,"daemon_list[%d].arg[%d]=%s",i,j,daemon_list[i].arg[j]);
				}
				daemon_list[i].arg[daemon_list[i].arglen] = endl;
			}
			
			env = cJSON_GetObjectItem(aSub,"env");
			daemon_list[i].envlen = cJSON_GetArraySize (env);
			zlog_debug(c,"daemon_list[%d].envlen=%d",i,daemon_list[i].envlen);
			if(daemon_list[i].envlen > 0)
			{
				daemon_list[i].env = (char **)malloc(sizeof(char *)*daemon_list[i].envlen);
				for(j = 0; j <daemon_list[i].envlen; j++)
				{
					pSub = cJSON_GetArrayItem(env, j);
					replace_daemon_list(&(daemon_list[i].env[j]),pSub->valuestring);
					zlog_debug(c,"daemon_list[%d].env[%d]=%s",i,j,daemon_list[i].env[j]);
				}
			}
			else
			{
				daemon_list[i].env = (char **)malloc(sizeof(char *));
				daemon_list[i].env[0] = endl;
			}

			
			version_cmd = cJSON_GetObjectItem(aSub,"version_cmd");
			replace_daemon_list(&(daemon_list[i].cmd),version_cmd->valuestring);
			zlog_debug(c,"daemon_list[%d].cmd=%s",i,daemon_list[i].cmd);
//			char * ivalue = aSub->valuestring ;
//			printf("Maclist[%d] : %s",iCnt,ivalue);
		}
	}

}
void delete_daemon_list()
{
	unsigned int i = 0,j=0;;
	for(i=0; i<daemon_list_num; i++)
	{
		for(j=0; j<(daemon_list[i].arglen); j++)
		{
			free(daemon_list[i].arg[j]);
		}
		free(daemon_list[i].arg);
		for(j=0; j<daemon_list[i].envlen; j++)
		{
			free(daemon_list[i].env[j]);
		}
		free(daemon_list[i].env);
		free(daemon_list[i].path);
		free(daemon_list[i].cmd);
		free(daemon_list[i].ver);
	}
	free(daemon_list);
}
int parse_json(const char *filename)
{
	
	cJSON* root = NULL;
	FILE *fp = fopen(filename, "r");
	if(fp == NULL)
	{
		zlog_error(c,"open %s fail.",filename);
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	long len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *data = (char*)malloc(len + 1);
	fread(data, 1, len, fp);
	fclose(fp);


	root = cJSON_Parse(data);
	

	if (root == NULL)
	{
		zlog_error(c,"JSON Parse fail,%s.",cJSON_GetErrorPtr());
	    return 1;
    }
	parse_daemon_list(root);			 	
	cJSON_Delete(root);
	return 0;
}



