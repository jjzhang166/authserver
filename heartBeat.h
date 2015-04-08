#ifndef __HEARTBEAT_H__
#define __HEARTBEAT_H__

//处理心跳
#define HEART_CHECK_INTV 4
#define HEART_INTV 2
struct TcpStatus
{
	int alive;  //频管是否alive
	time_t timeStamp;  //时间戳
	int cnt;
	pthread_mutex_t heartMutex;
};

struct TcpStatus *allocTcpStatus();

/*销毁一个TcpStatus 结构 */
void destroyTcpStatus(struct TcpStatus *ts);

/** 心跳检测
 */
void * check_heart_beat_thread(void *arg);


/** 处理心跳
 */
void handle_heart_beat(struct TcpStatus *ts, u_char *buf);
#endif
