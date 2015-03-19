#include <stdio.h>
#include "authProtocol.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#define BUF_SIZE 64
#define PORT 12345
#define serip "127.0.0.1"

void *handle_reply_thread(void *args);
void *do_heart_beat(void *args);
void *do_access_info(void *args);
int tests = 0;
int main(int argc, char *argv[])
{
	pthread_t tid;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in seraddr;
	if(argc<2)
	{
		printf("need a param(how many tests to run?)\n");
		printf("usage: ./authClient 5\n");
		return -1;
	}
	tests = atoi(argv[1]);

	seraddr.sin_family = AF_INET;	
	seraddr.sin_port = htons(PORT);
	seraddr.sin_addr.s_addr = inet_addr(serip);
	bzero(seraddr.sin_zero, 8);
	if(connect(fd, (struct sockaddr*)&seraddr, sizeof(struct sockaddr)) < 0)
	{
		printf("Connect failed!\n");
		return -1;
	}
 	if(pthread_create(&tid, NULL, handle_reply_thread, (void*)fd) != 0)
							{
								printf("Create handle reply thread failed!\n");
								close(fd);
								exit(-1);
							}
	//设置线程分离
	pthread_detach(tid);

	if(pthread_create(&tid, NULL, do_heart_beat, (void*)fd) != 0)
							{
								printf("Create handle reply thread failed!\n");
								close(fd);
								exit(-1);
							}
	//设置线程分离
	pthread_detach(tid);

	if(pthread_create(&tid, NULL, do_access_info, (void*)fd) != 0)
							{
								printf("Create handle reply thread failed!\n");
								close(fd);
								exit(-1);
							}
	//设置线程分离
	pthread_detach(tid);

	int i = 0;
	while(i++<tests)
	{
		RequestPDU_t req = {20, T_ZERO, S_AUTH_REQUEST,(u_char)i, C_MOBILE_STATION, "123456"};
		printf("len=%d T=%d pin=%s\n",req.Len, req.T,  req.Pin);
		char buf[AUTH_PDU_LEN];
		build_RequestPDU(&req, buf);
		send(fd, buf, AUTH_PDU_LEN,0);
		
		req.S = S_AUTH_FINISH;
		build_RequestPDU(&req, buf);
		send(fd, buf, AUTH_PDU_LEN,0);
		//usleep(10000);
	}
	sleep(100);
	close(fd);
}
void *do_heart_beat(void *args)
{
	int serfd = (int)args;
	int i = 0;
	char buf[HEART_BEAT_PDU_LEN];
	HeartBeatPDU_t hbt = {HEART_BEAT_PDU_LEN,0,S_ZERO,};
	build_HeartBeatPDU(&hbt, buf);
	while(i++ < tests)	
		send(serfd, buf, HEART_BEAT_PDU_LEN,0);
	
}
void *do_access_info(void *args)
{
	int serfd = (int)args;
	int i = 0;
	char buf[ACCINFO_PDU_LEN];
	AccessInfo_t acci = {ACCINFO_PDU_LEN,0,S_ACCESS_INFO,1,2,3,4,5,"aaa","192.168.1.1"};
	build_AccessInfo(&acci, buf);
	while(i++ < tests)	
		send(serfd, buf, ACCINFO_PDU_LEN,0);
	
}
void *handle_reply_thread(void *args)
{
	int serfd = (int)args;
	u_char buf[BUF_SIZE];
	int curn = 0;
	int sum = 0;
	ReplyPDU_t rep;
	printf("Handle reply thread running\n");	
	while(1)
	{	
		MsgPDU_t msg;
		int n = recv(serfd, buf + curn, BUF_SIZE-curn, 0);
		if(n<=0)
		{
			printf("recv%d bytes breaked\n", n);
			break;
		}
		sum+=n;
		curn += n;
	//	printf("收到 %d Bytes, curn = %d\n", n, curn);
		
		while(1)
		{
			if(parse_MsgPDU(&msg, buf, curn) != OPSUCCESS)
				break;		/*这是因为curn<4*/
			if(curn < msg.Len)
				break;	   /*数据包还没完全接收*/	
			if(msg.Len > BUF_SIZE)
			{
				curn = 0; /*信息长度大于缓冲，清除缓冲*/
				break;
			}
			/*接受到一个完整的包*/
			switch(msg.T)
			{
				case T_ZERO:		/*认证中 T总为0*/
					switch(msg.S)
					{
						case S_AUTH_REPLY:
						{
							
					
							parse_ReplyPDU(&rep, buf);
							printf("seq %d 认证结果: ", rep.Seq);
							if(rep.V == V_SUCCESS)
								printf("认证成功！\n");
							else if(rep.V == V_UNKNOWUSER)
								printf("未知用户！\n");
							else  printf("不匹配！\n");
							break;
						}
						default:
							printf("包有问题！\n");
					}	//end switch
				break;

				default:
					printf("没有实现的T\n");
			}//end switch
		
			memmove(buf, buf+msg.Len, curn-msg.Len);
			curn -= msg.Len;
		}//end while
	}

}

