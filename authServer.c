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
#include "authProtocol.h"
unsigned short PORT = 12345;	  //监听端口
unsigned int MAX_TCP_QUEUE = 5;   //listen 的参数
unsigned char IPADDR[17];
#define BUF_SIZE 64     		  //本程序涉及心跳包16B, 认证包20B，接入信息包34B
/*日志配置*/
char ident[64] = "authServer";
int option = LOG_CONS | LOG_PID;
int facility = 0;

/*数据库配置*/
#include<apu.h>
#include<apr_pools.h>
#include<apr_dbd.h>	
char dbdriver[128] = "mysql";			 /*数据库驱动程序名*/
char dbparams[256] = "host=localhost;user=root;pass=123456;dbname=terminal";

/*是否成为守护进程*/
int beDeamon = 1;

int dbd_check_pin(const char *pin);		 /* 检测pin 码的正确性 */
void *handle_tcp_thread(void *args); 	 /*处理一个连接的线程函数*/
void *handle_session_thread(void *args); /*处理一个认证的线程， 一个连接上有多个认证*/
void read_config();
void parse_cmd_line(int argc, char *argv[]);

#define MAX_THREADS 100
sem_t sem;								/*控制线程数*/	

int beStop = 0;  						/*要停止服务器吗*/
void sig_fun(int sig)
{
	int i;
	beStop = 1;
	syslog(LOG_INFO, "认证服务器正在退出...\n");
	if(!beDeamon)
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
	if(!beDeamon)
		printf("认证服务器已经退出\n");
 
	closelog();
	exit(0);
}

/* 守护进程 */
void become_deamon()
{
	int i;
	pid_t pid;
	if(!beDeamon)
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
	/*打开日志*/
	openlog(ident, option, facility);

	printf( "认证服务器正在启动...\n");
	syslog(LOG_INFO, "认证服务器正在启动...\n");
	/*read config file */
	read_config();
	
	/* handle command lines */
	parse_cmd_line(argc, argv);
	/* 守护进程 */
	become_deamon();
	
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
		printf("Get socket failed!\n");
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
		printf("Bind failed!\n");
		close(serfd);
		return -1;
	}

	//5.开始监听
	if(listen(serfd, MAX_TCP_QUEUE) < 0)
	{
		printf("Listen error!\n");
		close(serfd);
		return -1;
	}

	syslog(LOG_INFO, "认证服务器已经启动.\n");
	if(!beDeamon)
		printf("认证服务器已经启动!\n");
	//6.开始接受TCP连接
	while(!beStop)
	{
		pthread_t tid;
		//接受一个连接
		int clifd = accept(serfd, NULL, 0);
		if(clifd < 0)
		{
			syslog(LOG_INFO,"accept failed!\n");
			continue;
		}
		//printf("Accet a client!\n");
		
	
		//创建一个线程处理这个连接	clifd 将在创建的线程中关闭
		if(pthread_create(&tid, NULL, handle_tcp_thread, ((void*)clifd)) != 0)
		{
			syslog(LOG_INFO,"Create handle TCP trhead failed!\n");
			close(clifd);
			exit(-1);
		}
		//设置线程分离
		pthread_detach(tid);
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
		syslog(LOG_INFO,"内存不足，malloc 失败，在handle_tcp_thread中\n");
		return;
	}
					
	sess->clifd = clifd;
	if(parse_RequestPDU(&sess->req, buf) != OPSUCCESS)
	{
		syslog(LOG_INFO,"一个错误的认证请求包!\n");
		return;
	}
	syslog(LOG_INFO, "一个认证请求包!\n");
	sem_wait(&sem);
	//创建一个线程处理这个连接	clifd 将在创建的线程中关闭
	if(pthread_create(&tid, NULL, handle_session_thread, sess) != 0)
	{
		syslog(LOG_INFO, "Create handle session thread failed!\n");
		close(clifd);
		free(sess);
	}
	//设置线程分离
	pthread_detach(tid);
	/*handle_session_thread(sess);*/
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
			if(curn < msg.Len)
				break;	   /*数据包还没完全接收*/	
			if(msg.Len > BUF_SIZE)
			{
				curn = 0; /*信息长度大于缓冲，清除缓冲*/
				syslog(LOG_INFO, "error packet, Len not right!\n");
				break;
			}
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
		
			memmove(buf, buf+msg.Len, curn-msg.Len);
			curn -= msg.Len;
		}//end while
	}

	close(clifd);
	if(!beStop)
		syslog(LOG_INFO, "频管已断开连接!\n");
}

/* 这是个线程函数
 * args 中有要处理的任务
 * 线程结束后释放session
 */
void *handle_session_thread(void *args)
{
	char pin[9];
	char buf[AUTH_PDU_LEN];
	ReplyPDU_t rep;
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
		syslog(LOG_INFO,"在数据库中找到匹配的PIN\n");
	}
	else
	{	
		rep.V = V_UNKNOWUSER;
		syslog(LOG_INFO, "没有在数据库中找到匹配的PIN\n");
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
           syslog(LOG_INFO,"Failed to load driver file apr_dbd_%s.so\n", dbdriver);
           goto finish;
        case APR_ESYMNOTFOUND:
           syslog(LOG_INFO,"Failed to load driver apr_dbd_%s_driver.\n", dbdriver);
           goto finish;
        case APR_ENOTIMPL:
           syslog(LOG_INFO,"No driver available for %s.\n", dbdriver);
           goto finish;
        default:        /* it's a bug if none of the above happen */
           syslog(LOG_INFO,"Internal error loading %s.\n", dbdriver);
           goto finish;
        }
	rv = apr_dbd_open(driver, respool, dbparams, &sql);
    switch (rv) {
        case APR_SUCCESS:
           //printf("Opened %s[%s] OK\n", name, params);
           break;
        case APR_EGENERAL:
           syslog(LOG_INFO,"Failed to open %s[%s]\n", dbdriver, dbparams);
           goto finish;
        default:        /* it's a bug if none of the above happen */
           syslog(LOG_INFO,"Internal error opening %s[%s]\n", dbdriver, dbparams);
           goto finish;
        }

    rv = apr_dbd_select(driver,respool,sql,&res,statement,0);
    if (rv) {
        syslog(LOG_INFO,"Select failed: %s", apr_dbd_error(driver, sql, rv));
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


/*读取配置文件 配置文件要放在/etc/authServer.conf*/
const char *config_file = "/etc/authServer.conf";
void set_config_value(char *name, char *value)
{
	int name_len = strlen(name);
	int value_len = strlen(value);

	if(strcmp(name,"IPADDR") == 0)
			strncpy(IPADDR,value,16);
	else if(strcmp(name,"PORT") == 0)
			PORT = atoi(value);
	else if(strcmp(name, "TCP_QUEUE") == 0)
			MAX_TCP_QUEUE = atoi(value);
	else if(strcmp(name, "LOGIDENT") == 0)
			strncpy(ident, value, 64);
	else if(strcmp(name, "FACILITY") == 0)
			facility = atoi(value);
	else if(strcmp(name, "DBDRIVER") == 0)
			strncpy(dbdriver, value, 256);
	else if(strcmp(name, "DBPARAMS") == 0)
			strncpy(dbparams, value, 256);
	else if(strcmp(name, "BEDEAMON") == 0)
			beDeamon = atoi(value);
	else syslog(LOG_INFO,"Unknown name and value: %s %s\n", name, value);
}
/*读取配置文件*/
void read_config()
{
	char line[512];
	FILE *fp = fopen(config_file, "r");
	int t;
	if(fp == NULL)
	{
		syslog(LOG_INFO,"读取配置文件失败！请把配置文件放在/etc/authServer.conf\n");
		return;
	}

	while(fgets(line, 512, fp) != NULL)
	{
		int len = strlen(line);
		char *ns, *vs, *p;
		int i = 0;
		if(line[len-1] != '\n')
			while(fgetc(fp)!='\n');
		//printf("%s", line);

		/* 找到变量名的起点 */
		for(i = 0; i < len; i++)
			if(line[i] !=' ' && line[i]!= '\t' && line[i]!='\r'&& line[i]!='\n')
				break;
		if(i < len)
			ns = &line[i];
		else
			continue;
		/* 找到变量名的终点 */
		for(i = i+1; i < len; i++)
			if(line[i] == ' ' || line[i] == '\t' || line[i]=='\r' || line[i] == '\n')
				break;
		if(i<len)
			 line[i] = '\0';
		else
			continue;
		/* 找到变量值的起点 */
		for(i = i+1; i < len; i++)
			if(line[i] !=' ' && line[i]!= '\t' && line[i]!='\r' && line[i]!='\n')
				break;
		if(i < len)
			vs = &line[i];
		else
			continue;
		/* 找到变量值的终点 */
		t = i;
		for(i = len-1; i > t; i--)
			if(line[i] == ' ' || line[i] == '\t' || line[i]=='\r' ||  line[i] == '\n')
			;
			else
				break;
		i++;
		if(i<len)
				line[i] = '\0';
		else
			continue;

		if(strlen(ns)<=0 || strlen(vs)<=0)
			continue;
		if(*ns=='#')  //注释
			continue;
		/*把变量名变为的大写*/
		p = ns;
		while(*p != '\0')
		{
			if(*p>='a' && *p<='z')
				*p -= 32;
			p++;
		}
		set_config_value(ns, vs);
	}
	syslog(LOG_INFO,"读取配置文件(%s)成功\n", config_file);
}

void usage()
{
	printf("Welcom to use authServer...\n");
	printf("-h print this usage.\n");
	printf("-f don't become a deamon.\n");
	printf("-i server's listen ip address.\n");
	printf("-p server's listen port.\n");
	printf("-d database's driver. such as mysql\n");
	printf("-m database's params. such as host=localhost;user=root;pass=123456;dbname=terminal\n");  
	printf("--config file is %s\n", config_file);
	printf("\n");
}
void parse_cmd_line(int argc, char *argv[])
{
	int ch, opterr = 0;
	char *cmd="i:p:d:m::hf";
	while((ch = getopt(argc, argv, cmd)) != -1)
	{
		switch(ch)
		{
			case 'h':
				usage();
				exit(-1);
				break;
			case 'f':
				beDeamon = 0;
				break;
			case 'p':
				PORT = atoi(optarg);
				break;
			case 'd':
				strncpy(dbdriver, optarg, 128);
				break;
			case 'm' :
				strncpy(dbparams, optarg, 256);
				break;
			case 'i' :
				strncpy(IPADDR, optarg, 16);
				break;
			default:
				printf(" try -h to see help.\n", ch);	
				exit(-1);
		}
	}
}
