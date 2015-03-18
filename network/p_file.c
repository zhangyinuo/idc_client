#include <sys/prctl.h>
#include "p_file.h"
#include "GeneralHashFunctions.h"
#include "log.h"
#include "vfs_localfile.h"

extern int glogfd;
extern int max_p_file_thread;

extern int topper_queue;
extern int botter_queue;
static __thread int idx_queue = 1;

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

	*url = s;
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

	*dstip = s;
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
		char *t1 = strchr(t, '/');
		if (!t1)
			r = strncmp(t+1, "htm", 3);
	}
	*s = '?';
	return r;
}

static int push_new_task(t_task_base *base)
{
	if (try_touch_tmp_file(base) == LOCALFILE_OK)
	{
		LOG(glogfd, LOG_TRACE, "fname[%s:%s] do_newtask dup!\n", base->url, base->dstip);
		return -1;
	}
	t_vfs_tasklist *task0 = NULL;
	if (vfs_get_task(&task0, TASK_HOME))
	{
		LOG(glogfd, LOG_ERROR, "fname[%s:%s] do_newtask error!\n", base->url, base->dstip);
		return -1;
	}
	t_vfs_taskinfo *task = &(task0->task);
	memset(&(task->base), 0, sizeof(task->base));
	memcpy(&(task->base), base, sizeof(task->base));
	idx_queue++;
	if (idx_queue > topper_queue)
		idx_queue = botter_queue;
	vfs_set_task(task0, idx_queue);
	LOG(glogfd, LOG_NORMAL, "fname[%s:%s:%d] do_newtask ok!\n", base->url, base->dstip, idx_queue);
	return 0;
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

		t_task_base base;
		memset(&base, 0, sizeof(base));
		snprintf(base.url, sizeof(base.url), "%s", url);
		snprintf(base.dstip, sizeof(base.dstip), "%s", dstip);


		uint32_t h1, h2, h3;
		get_3_hash(base.url, &h1, &h2, &h3);
		uint32_t idx = h1 & 0x3F;
		snprintf(base.filename, sizeof(base.filename), "%s/%u/%u/%u/%u", g_config.docroot, idx, h1, h2, h3);
		if (access(base.filename, F_OK) == 0)
			continue;
		snprintf(base.tmpfile, sizeof(base.tmpfile), "%s/%u/%u/%u/%u.tmp", g_config.docroot, idx, h1, h2, h3);
		push_new_task(&base);
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
#ifndef PR_SET_NAME
#define PR_SET_NAME 15
#endif
	int *idx = (int *)arg;
	char name[16] = {0x0};
	snprintf(name, sizeof(name), "p_file_%d", *idx);
	prctl(PR_SET_NAME, name, 0, 0, 0);
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

