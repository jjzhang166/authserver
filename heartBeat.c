#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "authProtocol.h"
#include "readConfig.h"
#include "heartBeat.h"
#include "debug.h"

/*分配一个TcpStatus 结构 */
struct TcpStatus *allocTcpStatus()
{
	struct TcpStatus *ts = (struct TcpStatus *)malloc(sizeof(struct TcpStatus));
	if(ts == NULL)
	{
		syslog(LOG_ERR, "malloc failed in handle_tcp_thread\n");
		DEBUGMSG(("malloc failed!\n"));
		exit(-1);
	}
	ts->cnt = 2;	//check_heart_beat_thread 里销毁一次， TCP连接断开时销毁一次
	ts->alive = 1;
	ts->timeStamp = time(NULL);
	pthread_mutex_init(&ts->heartMutex, NULL);
	return ts;

}

/*销毁一个TcpStatus 结构 */
void destroyTcpStatus(struct TcpStatus *ts)
{
	int cnt;
	pthread_mutex_lock(&ts->heartMutex);
	cnt = --ts->cnt;	
	pthread_mutex_lock(&ts->heartMutex);
	if(cnt == 0)
	{
		pthread_mutex_unlock(&ts->heartMutex);
		free(ts);
		DEBUGMSG(("销毁TCP状态结构：struct TcpStatus\n"));
	}
}
/** 心跳检测
 */
void * check_heart_beat_thread(void *arg)
{
	struct TcpStatus *ts = (struct TcpStatus *)arg;
	pthread_detach(pthread_self());
	while(!beStop)
	{
		sleep(HEART_CHECK_INTV);
		DEBUGMSG(("检测心跳.\n"));
		if(time(NULL) - ts->timeStamp > HEART_INTV)
		{
			ts->alive = 0;
			syslog(LOG_ERR, "No heart beat!\n");
			DEBUGMSG(("no heart heat!\n"));
			break;
		}
		ts->alive = 1;
	}
	destroyTcpStatus(ts);
	return NULL;
}
/** 处理心跳
 */
void handle_heart_beat(struct TcpStatus *ts, u_char *buf)
{
	HeartBeatPDU_t hbt;
	if(parse_HeartBeatPDU(&hbt, buf) != OPSUCCESS)
	{
		syslog(LOG_INFO,"一个错误的心跳包!\n");
		return;
	}
	ts->timeStamp = time(NULL);
	syslog(LOG_INFO, "一个心跳包!\n");
	DEBUGMSG(("心跳包.\n"));
	/* 如果下面的操作可能阻塞，创建一个线程去做吧, 在创建线程前sem_wait(&sem); */
}

