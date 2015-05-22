#ifndef __AUTH_REQUEST_H__
#define __AUTH_REQUEST_H__

#include "auth_protocol.h"

/** 认证请求session
 */
struct session
{
	int clifd;  			//客户端socket
	RequestPDU_t req;			//认证请求包
	pthread_mutex_t *writeMutex;	//clifd在包级别的互斥锁
};


/** 处理认证请求
 * clifd: 客户端socket
 * buf: 请求包
 * writeMutex: clifd的写入锁
 */
void handle_request(int clifd, u_char *buf, pthread_mutex_t *writeMutex);


/* 这是个线程函数
 * args 中有要处理的任务
 * 线程结束后释放session
 */
void *handle_session_thread(void *args);
#endif /* __AUTH_REQUEST_H__ */
