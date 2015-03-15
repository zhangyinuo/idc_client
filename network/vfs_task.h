/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/

#ifndef _VFS_TASK_H_
#define _VFS_TASK_H_

/*
 *������ƣ�ҵ���߳����Ѻ���������ݷ��� CLEAN������У�Agent�̶߳�ʱ����ö������ݣ�����HOME��ҵ���߳�ѭ������
 *��ʱ������ҵ���߳��Լ�������������CLEAN���У���AGENT�̻߳���
 */

#include "list.h"
#include "vfs_init.h"
#include "nm_app_vfs.h"
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#define SYNCIP 256
#define DOMAIN_PREFIX "fcs"

#define TASK_HOME 0
#define MAX_TASK_QUEUE 65

enum {TASK_MOD_UP = 0, TASK_MOD_DOWN, TASK_MOD_CANCEL};

enum {OPER_IDLE, OPER_GET_REQ, OPER_GET_RSP, OPER_PUT, SYNC_2_GROUP};  /*�������� GET FCS-��CS�� CS-��CS��PUT CS-��CS*/

enum {TASK_ADDFILE = '0', TASK_DELFILE, TASK_MODFILE, TASK_LINKFILE, TASK_SYNCDIR};  /* �������� */

enum {OVER_UNKNOWN = 0, OVER_OK, OVER_E_MD5, OVER_PEERERR, TASK_EXIST, OVER_PEERCLOSE, OVER_UNLINK, OVER_TIMEOUT, OVER_MALLOC, OVER_SRC_DOMAIN_ERR, OVER_SRC_IP_OFFLINE, OVER_E_OPEN_SRCFILE, OVER_E_OPEN_DSTFILE, OVER_E_IP, OVER_E_TYPE, OVER_SEND_LEN, OVER_TOO_MANY_TRY, OVER_DISK_ERR, OVER_LAST};  /*�������ʱ��״̬*/

enum {GET_TASK_ERR = -1, GET_TASK_OK, GET_TASK_NOTHING};  /*��ָ������ȡ����Ľ��*/

enum {TASK_DST = 0, TASK_SOURCE, TASK_SRC_NOSYNC, TASK_SYNC_ISDIR, TASK_SYNC_VOSS_FILE}; /*���������Ƿ���Ҫ��ͬ�����ͬ�� */

extern const char *over_status[OVER_LAST]; 
typedef struct {
	char dstip[16];
	char url[1024];
	char filename[256];
	char tmpfile[256];
	off_t fsize;
	off_t lastlen;
}t_task_base;

typedef struct {
	t_task_base base;
	void *user;
}t_vfs_taskinfo;

typedef struct {
	t_vfs_taskinfo task;
	list_head_t llist;
	list_head_t hlist;
	list_head_t userlist;
	uint32_t upip;
	uint8_t status;
	uint8_t bk[3];
} t_vfs_tasklist;

typedef struct {
	list_head_t alist;
	time_t hbtime;
	int fd;
	int local_in_fd; /* ��cs���ܶԶ��ļ�����ʱ���򿪵ı��ؾ�� ��fd�ɲ���Լ����� */
	uint8_t sock_stat;   /* SOCK_STAT */
	uint8_t nostandby; // 1: delay process 
	t_task_base base;
} http_peer;

typedef void (*timeout_task)(t_vfs_tasklist *task);

int vfs_get_task(t_vfs_tasklist **task, int status);

int vfs_set_task(t_vfs_tasklist *task, int status);

int init_task_info();

int try_touch_tmp_file(t_task_base *base);

int add_task_to_alltask(t_vfs_tasklist *task);

int mod_task_level(char *filename, int type);

int get_task_from_alltask(t_vfs_tasklist **task, t_task_base *base);

int get_timeout_task_from_alltask(int timeout, timeout_task cb);

int scan_some_status_task(int status, timeout_task cb);

int get_task_count(int status);

void do_timeout_task();

void report_2_nm();

#endif
