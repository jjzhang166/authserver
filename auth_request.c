/** 处理认证请求包
*/

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <memory.h>
#include <sys/socket.h>

#include "global.h"
#include "auth_request.h"
#include "debug.h"
#include "database_query.h"

/** 处理认证请求
 * clifd: 客户端socket
 * buf: 请求包
 * writeMutex: clifd的写入锁
 */
void handle_request(int clifd, u_char *buf, pthread_mutex_t *writeMutex)
{
	pthread_t tid;
	struct session * sess = (struct session*) malloc(sizeof(struct session));

	if(sess == NULL)
	{
		syslog(LOG_ERR,"内存不足，malloc 失败，在handle_tcp_thread中\n");
		return;
	}
					
	sess->clifd = clifd;
	sess->writeMutex = writeMutex;
	if(parse_RequestPDU(&sess->req, buf) != OPSUCCESS)
	{
		syslog(LOG_ERR,"一个错误的认证请求包!\n");
		return;
	}
	syslog(LOG_INFO, "一个认证请求包!\n");
	sem_wait(&sem);

	//创建一个线程处理这个连接	clifd 将在创建的线程中关闭
	if(pthread_create(&tid, NULL, handle_session_thread, sess) != 0)
	{
		syslog(LOG_ERR, "Create handle session thread failed!\n");
		close(clifd);
		free(sess);
	}
}


/* 这是个线程函数
 * args 中有要处理的任务
 * 线程结束后释放session
 */
void *handle_session_thread(void *args)
{
	unsigned char buf[AUTH_PDU_LEN];
	int rv;
	int remain;
	ReplyPDU_t rep;
	//设置线程分离
	pthread_detach(pthread_self());

	memset(&rep, 0, sizeof(rep));
	struct session *sess = (struct session *)args;
	RequestPDU_t *req = &sess->req;
	DEBUGMSG(("len = %d  T = %d  S = %d  Seq = %d C = %d Pin = %s\n",req->Len, req->T, req->S, req->Seq, req->C, req->Pin));
	rep.Len = req->Len;
	rep.T = T_FREQ_NETMANAGER;
	rep.S   = S_AUTH_REPLY;
	rep.Seq = req->Seq;
	rep.C	= req->C;
	memcpy(rep.Pin, req->Pin, 8);

	// 在数据库中查找
	if(strlen(req->Pin) == PIN_LEN && dbd_check_pin(req->Pin)==OPSUCCESS)
	{
		rep.V = V_SUCCESS;
		DEBUGMSG(("在数据库中找到匹配的PIN\n"));
	}
	else
	{	
		char sql[256];
		snprintf(sql, 256, "insert into terminals_illegal(pin, name, descr) values('%s','%s','%s')", req->Pin, "unknow user","非法用户");
		rep.V = V_UNKNOWUSER;
		if(do_dbd_query(sql)==0)
			syslog(LOG_INFO, "insert into terminals_illegal failed!\n");
		DEBUGMSG(("没有在数据库中找到匹配的PIN\n"));
	}
	
	//创建应答包
	build_ReplyPDU(&rep, buf);
	pthread_mutex_lock(sess->writeMutex);
	remain = AUTH_PDU_LEN;
	while(remain > 0)
	{
		rv = send(sess->clifd, buf, remain, 0);
		if(rv <= 0)
		{
			pthread_mutex_unlock(sess->writeMutex);
			syslog(LOG_ERR, "send data failed, in handle session\n");
			break;
		}
		remain -= rv;	
	}
	syslog(LOG_INFO, "send a reply packet\n");
	pthread_mutex_unlock(sess->writeMutex);
	free(sess);
	sem_post(&sem);
	return NULL;
}
