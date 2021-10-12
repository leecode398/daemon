#include "stdio.h"
extern char** environ;
int main(int argc，char **argv)
{
	int nIndex = 0;
    for(nIndex=0;nIndex<argc;nIndex++)
	{
		printf("arg:%s\n",argv[nIndex]);
	}

    for(nIndex = 0; environ[nIndex] != NULL; nIndex++)
    {
        printf("env:%s\n",environ[nIndex]);
    }

}
