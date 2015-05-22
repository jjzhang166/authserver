/** 处理接入信息包
*/

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <syslog.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>

#include "global.h"
#include "access_info.h"
#include "debug.h"
#include "database_query.h"


/*  处理接入信息
 * clifd: 客户端socket
 * buf:	  接入信息包缓冲区
 */
void handle_access_info(int clifd, u_char *buf, pthread_mutex_t *writeMutex)
{
	pthread_t tid;
	struct access_info_session * ais = (struct access_info_session *) malloc(sizeof(struct access_info_session));
	if(ais == NULL)
	{
		DEBUGMSG(( "malloc failed in handle access info()\n"));
		return;
	}
	ais->clifd = clifd;
	ais->writeMutex = writeMutex;
	if(parse_AccessInfo(&ais->accInfo, buf) != OPSUCCESS)
	{
		syslog(LOG_INFO, "一个错误的接入信息包!\n");
		return;
	}
	syslog(LOG_INFO,"一个接入信息包!\n");

	//创建一个线程处理这个连接	clifd 将在创建的线程中关闭
	sem_wait(&sem);
	if(pthread_create(&tid, NULL, handle_access_info_thread, ais) != 0)
	{
		syslog(LOG_ERR, "Create handle session thread failed!\n");
		close(clifd);
		free(ais);
	}/* 如果下面的操作可能阻塞，创建一个线程去做吧 sem_wait(&sem);*/
}

/** 处理接入信息的线程
 * args:	access_info_session
 */
void * handle_access_info_thread(void *args)
{
	char sqlcmd[512];
	int rv;
	struct access_info_session * ais = (struct access_info_session *)args;
	AccessInfo_t *accInfo = &ais->accInfo;

	//设置线程分离
	pthread_detach(pthread_self());
	if(ais == NULL)
	{
		DEBUGMSG(( " Bad args, args must not null in handle_access_info_thread\n"));
		goto finish;
	}
	switch(accInfo->OpType)
	{
		case OP_ADD:	//增加
		{
			snprintf(sqlcmd, 512, "insert into frem_access_info(PIN, FREQ, SNR, BS_IP) values('%s', '%d', '%d', '%s')", 
							accInfo->PIN, accInfo->FREQ, accInfo->SNR, accInfo->BS_IP);	
			rv = do_dbd_query(sqlcmd);
			if(rv !=1)
				syslog(LOG_ERR, "Insert into frem_access_info failed.\n");
			break;
		}
		case OP_UPDATE:	//更新
		{
			break;
		}
		case OP_DEL:	//删除
		{
			break;
		}
		default:
		{
			syslog(LOG_ERR, "Bad OpType in access info packet\n");
		}
	}
	free(ais);
finish:
	sem_post(&sem);
	return NULL;
}
