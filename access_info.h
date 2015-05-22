#ifndef __ACCESS_INFO_H__
#define __ACCESS_INFO_H__

#include "auth_protocol.h"

/** access info session
 *
 */
struct access_info_session
{
	int clifd;				//client socket
	AccessInfo_t accInfo;		//接入信息包
	pthread_mutex_t *writeMutex;	//clifd在包级别的互斥锁
};


/** 处理接入信息
 * clifd: 客户端socket
 * buf:	  接入信息包缓冲区
 */
void handle_access_info(int clifd, u_char *buf, pthread_mutex_t *writeMutex);


/** 处理接入信息的线程
 * args:	access_info_session
 */
void * handle_access_info_thread(void *args);

#endif /*__ACCESS_INFO_H__ */
