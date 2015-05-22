/** 心跳功能
 *  在TCP连接中，调用allocTcpStatus(...)函数得到
 *  struct TcpStatus结构
 *  因为要创建一个线程来定时检查心跳，而TCP连接也
 *  可能断开，心跳也可能没有，所以，这两个线程谁
 *  先退出不确定，而TcpStatus要到最后退出的那个线
 *  程退出是才销毁，TcpStatus中的cnt变量被初始化
 *  为2，线程退出是减1，为0时销毁。
 *
 */
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

/** 分配一个TcpStatus 结构 
 *  注意，要两次调用destroyTcpStatus()才能销毁
 *  return 成功返回struct TcpStatus 指针
 *  	   失败返回NULL
 * */
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
	while(!beStop && ts->alive)
	{
		sleep(check_heart_beat_itv);
		DEBUGMSG(("检测心跳.\n"));
		if(time(NULL) - ts->timeStamp > heart_beat_itv)
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
	syslog(LOG_INFO, "Heart Beat\n");
	DEBUGMSG(("心跳包.\n"));
	/* 如果下面的操作可能阻塞，创建一个线程去做吧, 在创建线程前sem_wait(&sem); */
}

