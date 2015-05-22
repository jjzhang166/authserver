#ifndef __HEART_BEAT_H__
#define __HEART_BEAT_H__

//心跳间隔和检测心跳的间隔
#define HEART_CHECK_INTV 4
#define HEART_INTV 2

struct TcpStatus
{
	int alive; 		 		//频管是否alive
	time_t timeStamp;  		//时间戳
	int cnt;		   		//本结构使用计数
	pthread_mutex_t heartMutex; //本结构互斥
};


/** 分配一个初始化好的struct TcpStatus结构
 *  注意，要两次调用destroyTcpStatus()才能销毁
 *  return 成功返回struct TcpStatus 指针
 */
struct TcpStatus *allocTcpStatus();

/**  销毁一个TcpStatus 结构 
 *	ts: 要销毁的TcpStatus结构指针
 *  注意，要两次调用destroyTcpStatus()才能销毁
 * */
void destroyTcpStatus(struct TcpStatus *ts);

/** 心跳检测
 *  线程函数
 *  arg 要传入struct TcpStatus 结构
 */
void * check_heart_beat_thread(void *arg);


/** 处理心跳
 *  ts: struct TcpStatus
 *  buf: 心跳数据包
 */
void handle_heart_beat(struct TcpStatus *ts, u_char *buf);

#endif /*__HEARTBEAT_H__*/
