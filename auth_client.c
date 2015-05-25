/*认证客户端，用来测试服务器
 * 客户端仅用于测试
 *
 */

#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>

#include "auth_protocol.h"
#define BUF_SIZE 64
#define PORT 3002
char*  serip = "127.0.0.1";

void *handle_reply_thread(void *args);
void *do_heart_beat(void *args);
void *do_access_info(void *args);

//测试数，从main的参数中获得
int tests = 0;

int main(int argc, char *argv[])
{
	pthread_t tid;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in seraddr;

	if(argc<2)
	{
		printf("need a param(how many tests to run?)\n");
		printf("usage: ./authClient 5 ipaddr\n");
		return -1;
	}
	srand(time(0));
	tests = atoi(argv[1]);
	if(argc>=3)
		serip = argv[2];

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

	if(pthread_create(&tid, NULL, do_heart_beat, (void*)fd) != 0)
	{
		printf("Create handle reply thread failed!\n");
		close(fd);
		exit(-1);
	}

	if(pthread_create(&tid, NULL, do_access_info, (void*)fd) != 0)
	{
		printf("Create handle reply thread failed!\n");
		close(fd);
		exit(-1);
	}

	int i = 0;
	while(i++<tests)
	{
		RequestPDU_t req = {20, T_FREQ_NETMANAGER, S_AUTH_REQUEST,(u_char)i, C_MOBILE_STATION, "00009001"};
		printf("len=%d T=%d pin=%s\n",req.Len, req.T,  req.Pin);
		unsigned char buf[AUTH_PDU_LEN];
		build_RequestPDU(&req, buf);
		send(fd, buf, AUTH_PDU_LEN,0);
		
		req.S = S_AUTH_FINISH;
		build_RequestPDU(&req, buf);
		send(fd, buf, AUTH_PDU_LEN,0);
		/*unsigned char *buf2 = malloc(AUTH_PDU_LEN);
		//for(i = 0; i < AUTH_PDU_LEN; i++)
			//buf2[0] = 45;	
		send(fd, buf2, AUTH_PDU_LEN,0);
		free(buf2);*/
		//usleep(10000);
	}
	sleep(1000);
	close(fd);
	return 0;
}


void *do_heart_beat(void *args)
{
	int serfd = (int)args;
	//int i = 0;
	unsigned char buf[HEART_BEAT_PDU_LEN];// = {HEART_BEAT_PDU_LEN, 0,0,0, T_FREQ_NETMANAGER, 0};

	pthread_detach(pthread_self());
	HeartBeatPDU_t hbt = {HEART_BEAT_PDU_LEN,T_FREQ_NETMANAGER,S_ZERO};
	build_HeartBeatPDU(&hbt, buf);
	int cnt = 0;
	while(1)	
	{
		send(serfd, buf, HEART_BEAT_PDU_LEN,0);
		cnt ++;
		if(cnt > 30)
			break;
		sleep(1);
	}

	return NULL;	
}


void *do_access_info(void *args)
{
	int serfd = (int)args;
	//int i = 0;
	unsigned char buf[ACCINFO_PDU_LEN];

	AccessInfo_t acci = {ACCINFO_PDU_LEN, T_FREQ_NETMANAGER, S_ACCESS_INFO, OP_ADD};
	acci.FREQ = 123;
	acci.SNR = 255;
	memcpy(&acci.PIN, "00009001", 9);
	memcpy(&acci.BS_IP, "198.168.111.222", 16);
// = {ACCINFO_PDU_LEN,0,S_ACCESS_INFO,1,2,3,4,5,"aaa","192.168.1.1"};
	
	pthread_detach(pthread_self());
	build_AccessInfo(&acci, buf);
	//while(i++ < tests)	
		send(serfd, buf, ACCINFO_PDU_LEN,0);

	/*acci.OpType = OP_DEL;
	acci.BS_IP[1] = '7';
	acci.FREQ=251;
	acci.SNR=250;
	build_AccessInfo(&acci, buf);
	send(serfd, buf, ACCINFO_PDU_LEN,0);
*/
	return NULL;	
}


void *handle_reply_thread(void *args)
{
	int serfd = (int)args;
	u_char buf[BUF_SIZE];
	int curn = 0;
	int sum = 0;
	ReplyPDU_t rep;

	pthread_detach(pthread_self());
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
				case T_FREQ_NETMANAGER:		/*认证中 T总为0*/
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
	return NULL;
}

