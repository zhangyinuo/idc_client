#include "p_file.h"
#include "log.h"

extern int glogfd;
extern int max_p_file_thread;

static int get_url_dstip(char *buf, char **dstip, char **url)
{
	char *s = buf;
	int i = 0;
	for (; i < 3; i++)
	{
		s = strchr(s, '\t');
		if (s == NULL)
			return -1;
		s++;
	}

	url = s;
	s = strchr(s, '\t');
	if (s == NULL)
		return -1;
	*s = 0x0;
	s++;
	for (i = 0; i < 2; i++)
	{
		s = strchr(s, '\t');
		if (s == NULL)
			return -1;
		s++;
	}

	dstip = s;
	s = strchr(s, '\t');
	if (s == NULL)
		return -1;
	*s = 0x0;
	return 0;
}

static int check_urls(char *url)
{
	int r = 0;
	char *s = strchr(url, '?');
	if (s)
		*s = 0x0;
	char *t = strrchr(url, '.');
	if (t)
	{
		char *t1
	}
	*s = '?';
	return r;
}

static int p_file_detail(char *file)
{
	FILE *fp = fopen(file, "r");
	if (fp == NULL)
	{
		LOG(glogfd, LOG_ERROR, "openfile %s err %m\n", file);
		return -1;
	}

	char *url;
	char *dstip;
	char buf[2048] = {0x0};
	while (fgets(buf, sizeof(buf), fp))
	{
		if (get_url_dstip(buf, &dstip, &url))
		{
			LOG(glogfd, LOG_ERROR, "get url dstip error %s\n", buf);
			continue;
		}

		if (check_urls(url))
			continue;
	}

	fclose(fp);
	return 0;
}

static int p_file_sub(char *path, int idx)
{
	int ret = 0;
	DIR *dp;
	struct dirent *dirp;
	if ((dp = opendir(path)) == NULL) 
	{
		LOG(glogfd, LOG_ERROR, "opendir %s err  %m\n", path);
		ret = -1;
		return ret;
	}
	LOG(glogfd, LOG_TRACE, "opendir %s ok \n", path);
	while((dirp = readdir(dp)) != NULL) 
	{
		if (dirp->d_name[0] == '.')
			continue;
		char file[256] = {0x0};
		snprintf(file, sizeof(file), "%s/%s", path, dirp->d_name);

		if (idx != r5hash(file)%max_p_file_thread)
			continue;

		if (p_file_detail(file))
		{
			ret = -1;
			break;
		}
	}
	closedir(dp);
	return ret;
}

static void * p_file_main(void *arg)
{
	int *idx = (int *)arg;
	char *indir = myconfig_get_value("file_indir");
	if (indir == NULL)
	{
		LOG(glogfd, LOG_ERROR, "file_indir is empty!\n");
		stop = 1;
		return NULL;
	}
	LOG(glogfd, LOG_DEBUG, "%d thread start process!\n", *idx);
	while (1)
	{
		if (p_file_sub(indir, *idx))
		{
			LOG(glogfd, LOG_ERROR, "p_file_sub error %m!\n");
			stop = 1;
			return NULL;
		}
		usleep(2);
	}
	return NULL;
}

int init_p_file(int idx)
{
	int arg = idx;
   	pthread_attr_t attr;
   	pthread_t tid;
	int rc;
	pthread_attr_init(&attr);
	if((rc = pthread_create(&tid, &attr, (void*(*)(void*))p_file_main, (void *)&arg)) != 0)
	{
	    printf("\7%s: pthread_create(): %m\n", strerror(errno));
	    return -1;
	}
	return 0;
}

