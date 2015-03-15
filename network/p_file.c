#include "p_file.h"
#include "log.h"

extern int stop;
extern int glogfd;

p_file_sub(indir);
static void * p_file_main(void *arg)
{
	char *indir = myconfig_getval("file_indir");
	if (indir == NULL)
	{
		LOG(glogfd, LOG_ERROR, "file_indir is empty!\n");
		stop = 1;
		return;
	}
	while (1)
	{
		if (p_file_sub(indir);
		usleep(2);
	}
}

int init_p_file()
{
   	pthread_attr_t attr;
   	pthread_t tid;
	int rc;
	pthread_attr_init(&attr);
	if((rc = pthread_create(&tid, &attr, (void*(*)(void*))p_file_main, NULL)) != 0)
	{
	    printf("\7%s: pthread_create(): %m\n", strerror(errno));
	    return -1;
	}
	return 0;
}

