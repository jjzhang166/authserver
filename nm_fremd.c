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
#include<sys/types.h>
#include<sys/stat.h>

#include<apu.h>
#include<apr_pools.h>
#include<apr_dbd.h>

#include "authProtocol.h"
#include "parseCmd.h"
#include "readConfig.h"
/**多线程服务器说明：第个线程函数要主动调用
 * pthread_detach(pthread_self()); 函数
 */

#define BUF_SIZE 64     		  //本程序涉及心跳包16B, 认证包20B，接入信息包34B

#define PIN_LEN 8      			  //PIN码8位


sem_t sem;								/*控制线程数*/	

int beStop = 0;  						/*要停止服务器吗*/

int dbd_check_pin(const char *pin);		 /* 检测pin 码的正确性 */
void *handle_tcp_thread(void *args); 	 /*处理一个连接的线程函数*/
void *handle_session_thread(void *args); /*处理一个认证的线程， 一个连接上有多个认证*/

void sig_fun(int sig)
{
	beStop = 1;
	syslog(LOG_INFO, "认证服务器正在退出...\n");
	if(!beDaemon)
		printf("认证服务器正在退出...\n");
	sleep(3);

	apr_terminate();
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

/* 守护进程 */
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

int main(int argc, char *argv[])
{
	struct sockaddr_in seraddr;
	int serfd;

	printf( "认证服务器正在启动...\n");

	/*read config file */
	if(read_config() == -1)
	{
		printf("Read config file failed!\n");
		exit(-1);
	}
	/* handle command lines */
	parse_cmd_line(argc, argv);

	/*打开日志*/
	openlog(ident, option, facility<<3);
	/* 守护进程 */
	become_daemon();
	
	/*注册信号处理函数*/
	signal(SIGINT, sig_fun);

	/* 初始化用于线程数控制的信号量*/
	sem_init(&sem, 0, MAX_THREADS);

	/*初始化apr库*/
    apr_initialize();
	
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
		//printf("Accet a client!\n");
		
	
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

struct session
{
	int clifd;  /*客户端socket*/
	RequestPDU_t req;
};

/* 处理认证请求 */
void handle_request(int clifd, u_char *buf)
{
	pthread_t tid;
	struct session * sess = (struct session*) malloc(sizeof(struct session));
	if(sess == NULL)
	{
		syslog(LOG_ERR,"内存不足，malloc 失败，在handle_tcp_thread中\n");
		return;
	}
					
	sess->clifd = clifd;
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

/*处理心跳
 */
void handle_heart_beat(int clifd, u_char *buf)
{
	HeartBeatPDU_t hbt;
	if(parse_HeartBeatPDU(&hbt, buf) != OPSUCCESS)
	{
		syslog(LOG_INFO,"一个错误的心跳包!\n");
		return;
	}
	syslog(LOG_INFO, "一个心跳包!\n");
	/* 如果下面的操作可能阻塞，创建一个线程去做吧, 在创建线程前sem_wait(&sem); */
}

/* 处理接入信息
 */
void handle_access_info(int clifd, u_char *buf)
{
	AccessInfo_t acci;
	if(parse_AccessInfo(&acci, buf) != OPSUCCESS)
	{
		syslog(LOG_INFO, "一个错误的接入信息包!\n");
		return;
	}
	syslog(LOG_INFO,"一个接入信息包!\n");
	/* 如果下面的操作可能阻塞，创建一个线程去做吧 sem_wait(&sem);*/
}
/* 处理一个TCP连接，args就是该连接的描述符
 * 处理之后关闭连接
 * 设置自己为分离线程
 * 返回NULL
 * 从数据流中分离出 一个个认证
 */
void *handle_tcp_thread(void *args)
{
	int clifd = (int)args;
	u_char buf[BUF_SIZE];
	int curn = 0;
	int sum = 0;
	
	//设置线程分离
	pthread_detach(pthread_self());
	syslog(LOG_INFO, "频管已经连接!\n");
	while(!beStop)
	{	
		MsgPDU_t msg;
		int n = recv(clifd, buf + curn, BUF_SIZE-curn, 0);
		if(n<=0)
		{
			//printf("recv%d bytes breaked\n", n);
			break;
		}
		sum+=n;
		curn += n;
		
		while(1)
		{
			if(parse_MsgPDU(&msg, buf, curn) != OPSUCCESS)
				break;		/*这是因为curn<4*/
			if(msg.Len > BUF_SIZE || msg.Len <= 0)
			{
				//curn = 0; /*信息长度大于缓冲，清除缓冲*/
				msg.Len = 1;
				//syslog(LOG_INFO, "error packet, Len not right!\n");
				goto finish;
				break;
			}
			if(curn < msg.Len)
				break;	   /*数据包还没完全接收*/	
			/*接受到一个完整的包*/
			switch(msg.T)
			{
				case T_ZERO:		/*认证中 T总为0*/
					switch(msg.S)
					{
						case S_ZERO: //心跳
						{
							handle_heart_beat(clifd, buf);
							break;
						}
						case S_AUTH_REQUEST:	//一个认证请求
						{
							
							handle_request(clifd, buf);
							break;
						}
						case S_AUTH_FINISH:	//认证完成
							syslog(LOG_INFO,"一个认证完成包!\n");
							break;
						case S_ACCESS_INFO:	//接入信息
						{
							handle_access_info(clifd, buf);
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
	if(!beStop)
		syslog(LOG_INFO, "频管已断开连接!\n");
	return NULL;
}

/* 这是个线程函数
 * args 中有要处理的任务
 * 线程结束后释放session
 */
void *handle_session_thread(void *args)
{
	char pin[9];
	unsigned char buf[AUTH_PDU_LEN];
	ReplyPDU_t rep;
	//设置线程分离
	pthread_detach(pthread_self());

	memset(&rep, 0, sizeof(rep));
	struct session *sess = (struct session *)args;
	RequestPDU_t *req = &sess->req;
	//printf("len = %d  T = %d  S = %d  Seq = %d C = %d Pin = %s\n",
	//		req->Len, req->T, req->S, req->Seq, req->C, req->Pin);
	rep.Len = req->Len;
	rep.S   = S_AUTH_REPLY;
	rep.Seq = req->Seq;
	rep.C	= req->C;
	memcpy(rep.Pin, req->Pin, 8);

	memcpy(pin, req->Pin, 8);
	pin[8] = '\0';
	if(dbd_check_pin(pin)==OPSUCCESS)
	{
		rep.V = V_SUCCESS;
		//syslog(LOG_INFO,"在数据库中找到匹配的PIN\n");
	}
	else
	{	
		rep.V = V_UNKNOWUSER;
		//syslog(LOG_INFO, "没有在数据库中找到匹配的PIN\n");
	}
	build_ReplyPDU(&rep, buf);
	send(sess->clifd, buf, AUTH_PDU_LEN, 0);
	free(sess);
	sem_post(&sem);
	return NULL;
}

/* 检测PIN 
 * 成功返回OPSUCCESS
 * 失败返回OPFAIL
 */
int dbd_check_pin(const char *pin)
{	

    int rv = 0;
    int i = 0;
    int n;
    const char* entry = NULL;
    char statement[128] = "SELECT * FROM terminals where Pin='";
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row = NULL;
	apr_pool_t *respool  = NULL;
	apr_dbd_t *sql = NULL;
	const apr_dbd_driver_t *driver = NULL;
	if(strlen(pin) != PIN_LEN )
		return OPFAIL;	
    strcat(statement,pin);
    strcat(statement,"'");
    rv = apr_pool_create(&respool, NULL);
	if(rv != APR_SUCCESS)
		goto finish;
    apr_dbd_init(respool);
    rv = apr_dbd_get_driver(respool, dbdriver, &driver);
	switch (rv) {
       	case APR_SUCCESS:
       		//printf("Loaded %s driver OK.\n", name);
          	break;
        case APR_EDSOOPEN:
           syslog(LOG_ERR,"Failed to load driver file apr_dbd_%s.so\n", dbdriver);
           goto finish;
        case APR_ESYMNOTFOUND:
           syslog(LOG_ERR,"Failed to load driver apr_dbd_%s_driver.\n", dbdriver);
           goto finish;
        case APR_ENOTIMPL:
           syslog(LOG_ERR,"No driver available for %s.\n", dbdriver);
           goto finish;
        default:        /* it's a bug if none of the above happen */
           syslog(LOG_ERR,"Internal error loading %s.\n", dbdriver);
           goto finish;
        }
	rv = apr_dbd_open(driver, respool, dbparams, &sql);
    switch (rv) {
        case APR_SUCCESS:
           //printf("Opened %s[%s] OK\n", name, params);
           break;
        case APR_EGENERAL:
           syslog(LOG_ERR,"Failed to open %s[%s]\n", dbdriver, dbparams);
           goto finish;
        default:        /* it's a bug if none of the above happen */
           syslog(LOG_ERR,"Internal error opening %s[%s]\n", dbdriver, dbparams);
           goto finish;
        }

    rv = apr_dbd_select(driver,respool,sql,&res,statement,0);
    if (rv) {
        syslog(LOG_ERR,"Select failed: %s", apr_dbd_error(driver, sql, rv));
        goto finish2;
    }
    for (rv = apr_dbd_get_row(driver, respool, res, &row, -1);
         rv == 0;
         rv = apr_dbd_get_row(driver, respool, res, &row, -1)) {
        //printf("ROW %d:	", i) ;
        ++i;
        for (n = 0; n < apr_dbd_num_cols(driver, res); ++n) {
            entry = apr_dbd_get_entry(driver, row, n);
            if (entry == NULL) {
               // printf("(null)	") ;
               ;
            }
            else {
                //printf("%s	", entry);
                ;
            }
        }
	//fputs("\n", stdout);
    }
	finish2:
		apr_dbd_close(driver, sql);
	finish: ;		
		apr_pool_destroy(respool);
	if(i>0)
		return OPSUCCESS;
	else
		return OPFAIL;
}



