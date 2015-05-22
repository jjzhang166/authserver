/** nm_fremd.c 主程序，初始化服务器
 *  服务器采用多线程技术
 * 多线程服务器说明：第个线程函数要主动调用
 * pthread_detach(pthread_self()); 函数
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "global.h"
#include "auth_protocol.h"
#include "parse_cmd.h"
#include "read_config.h"
#include "debug.h"
#include "heart_beat.h"
#include "database_query.h"
#include "auth_request.h"
#include "access_info.h"

sem_t sem;								//控制线程数

int beStop = 0;  						//要停止服务器吗

int dbd_check_pin(const char *pin);		 //检测pin 码的正确性 

void *handle_tcp_thread(void *args); 	 //处理一个连接的线程函数

void *handle_session_thread(void *args); //处理一个认证的线程 一个连接上有多个认证

/* 注册信号处理ctrl-C */
void sig_fun(int sig)
{
	beStop = 1;
	syslog(LOG_INFO, "认证服务器正在退出...\n");
	if(!beDaemon)
		printf("认证服务器正在退出...\n");
	sleep(3);

	destroy_database();

	sem_destroy(&sem);

/* 退出时的问题:
 * 数据库的资源未释放：pool driver
 * 日志未关闭
 * 监听socket未关闭
 * 客户端socket未关闭
 * 多个线程未退出
 * 可能有与数据库的连接还未关闭
 */
	syslog(LOG_INFO, "认证服务器已经退出.\n");
	if(!beDaemon)
		printf("认证服务器已经退出\n");
 
	closelog();
	exit(0);
}

/** 守护进程
*/
void become_daemon()
{
	int i;
	pid_t pid;
	if(!beDaemon)
		return;
    pid = fork();
	if(pid < 0)
	{
		syslog(LOG_INFO, "Fork 失败， 创建守护进程失败!\n");
		return;
	}
	else if(pid > 0)
			exit(0);
	setsid();
	chdir("/");
	umask(0);
	for(i = 0; i < 3; i++)
		close(i);
}

/** 主函数，完成初始化
*/
int main(int argc, char *argv[])
{
	struct sockaddr_in seraddr;
	int serfd;

	/*read config file */
	if(read_config() == -1)
	{
		DEBUGMSG(("Read config file failed!\n"));
		exit(-1);
	}

	/* handle command lines */
	parse_cmd_line(argc, argv);

	/*打开日志*/
	openlog(ident, option, facility);

	/* 守护进程 */
	become_daemon();
	
	/*注册信号处理函数*/
	signal(SIGINT, sig_fun);

	/* 初始化用于线程数控制的信号量*/
	sem_init(&sem, 0, MAX_THREADS);

	/*初始化apr库*/
	init_database();
  
	
	//1.创建socket	
	serfd = socket(AF_INET, SOCK_STREAM, 0);
	if(serfd < 0)
	{
		syslog(LOG_ERR,"Get socket failed!\n");
		return -1;
	}

	//2.设置监听地址和端口
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(PORT);
	seraddr.sin_addr.s_addr = INADDR_ANY;
	bzero(seraddr.sin_zero, sizeof(seraddr.sin_zero));

	//3.设置地址重用
	int opt=1;
	socklen_t  optlen=sizeof(opt);
	setsockopt(serfd, SOL_SOCKET, SO_REUSEADDR, &opt, optlen);

	//4.绑定socket和地址
	if(bind(serfd, (struct sockaddr*) &seraddr, sizeof(struct sockaddr)) < 0)
	{
		syslog(LOG_ERR,"Bind failed!\n");
		close(serfd);
		return -1;
	}

	//5.开始监听
	if(listen(serfd, MAX_TCP_QUEUE) < 0)
	{
		syslog(LOG_ERR, "Listen error!\n");
		close(serfd);
		return -1;
	}

	syslog(LOG_INFO, "认证服务器已经启动.\n");
	if(!beDaemon)
		printf("认证服务器已经启动!\n");

	//6.开始接受TCP连接
	while(!beStop)
	{
		pthread_t tid;

		//接受一个连接
		int clifd = accept(serfd, NULL, 0);
		if(clifd < 0)
		{
			syslog(LOG_ERR,"accept failed!\n");
			continue;
		}
		DEBUGMSG(("Accept a 频管!\n"));
		
	
		//创建一个线程处理这个连接	clifd 将在创建的线程中关闭
		if(pthread_create(&tid, NULL, handle_tcp_thread, ((void*)clifd)) != 0)
		{
			syslog(LOG_ERR,"Create handle TCP trhead failed!\n");
			close(clifd);
			exit(-1);
		}
	}
	return 0;
}







/* 处理一个TCP连接，args就是该连接的描述符
 * 处理之后关闭连接
 * 设置自己为分离线程
 * 返回NULL
 * 从数据流中分离出 一个个认证
 */
void *handle_tcp_thread(void *args)
{
	int clifd = (int)args;	//得到客户端socket
	u_char buf[BUF_SIZE];	//数据包缓冲区
	int curn = 0;			//当前到数据长度
	struct TcpStatus *ts;	//用于心跳
	struct timeval timeout;
	pthread_t tid;

	//socket 在一个包的级别上要互斥
	pthread_mutex_t socketWriteMutex = PTHREAD_MUTEX_INITIALIZER;	

	//设置线程分离
	pthread_detach(pthread_self());

	//心跳信息
	ts = allocTcpStatus();

	//创建心跳监测线程
	if(pthread_create(&tid, NULL, check_heart_beat_thread, ts) < 0)
	{
		syslog(LOG_ERR, "创建心跳检测线程失败.\n");
		exit(-1);
	} 	
	DEBUGMSG(("创建心跳检测线程成功\n"));	

	syslog(LOG_INFO, "频管已经连接!\n");
	while(!beStop)
	{	
		MsgPDU_t msg;
		
		//设置recv超时
		timeout.tv_sec = HEART_INTV;
		timeout.tv_usec = 0;
		setsockopt(clifd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		int n = recv(clifd, buf + curn, BUF_SIZE-curn, 0);
		if(!ts->alive)
		{
			DEBUGMSG(("没有心跳\n"));
			break;
		}
		if(n<=0)
		{
			if(errno == EAGAIN)
			{
				DEBUGMSG(("Timed out.\n"));
				continue;
			}
			DEBUGMSG(("recv%d bytes breaked\n", n));
			ts->alive = 0;
			break;
		}
		curn += n;
		
		while(1)
		{
			//解析数据包头
			if(parse_MsgPDU(&msg, buf, curn) != OPSUCCESS)
				break;		/*这是因为curn<4*/
			
			//数据包出错
			if(msg.Len > BUF_SIZE || msg.Len <= 0)
			{
				//curn = 0; /*信息长度大于缓冲，清除缓冲*/
				DEBUGMSG(("Bad packet Len: %d\n", msg.Len));
				msg.Len = 1;
				//syslog(LOG_INFO, "error packet, Len not right!\n");
				goto finish;
				break;
			}

			//数据包还没完全接收	
			if(curn < msg.Len)
				break;	   			//接收到一个完整的包
			switch(msg.T)
			{
				case T_FREQ_NETMANAGER:		//认证中 
					switch(msg.S)
					{
						case S_ZERO: //心跳
						{
							handle_heart_beat(ts, buf);
							break;
						}
						case S_AUTH_REQUEST:	//一个认证请求
						{
							
							handle_request(clifd, buf, &socketWriteMutex);
							break;
						}
						case S_AUTH_FINISH:	//认证完成
							syslog(LOG_INFO,"一个认证完成包!\n");
							break;
						case S_ACCESS_INFO:	//接入信息
						{
							handle_access_info(clifd, buf, &socketWriteMutex);
							break;
						}
						default:
							syslog(LOG_INFO,"包格式错误, 未定义的S域\n");
					}	//end switch
				break;

				default:
						syslog(LOG_INFO,"包格式错误, 未定义的T域\n");
			}//end switch
	finish:	
			memmove(buf, buf+msg.Len, curn-msg.Len);
			curn -= msg.Len;
		}//end while
	}

	close(clifd);
	destroyTcpStatus(ts);
	pthread_mutex_destroy(&socketWriteMutex);
	if(!beStop)
		syslog(LOG_INFO, "频管已断开连接!\n");

	return NULL;
}




