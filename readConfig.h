#ifndef __READCONFIG_H__
#define __READCONFIG_H__

extern int beStop; 					//要停止服务器吗
extern const char *config_file;	  	//配置文件
extern int MAX_THREADS;		  		//最多线程数
extern unsigned short PORT;	  	  	//监听端口
extern unsigned int MAX_TCP_QUEUE;  //listen 的参数
extern char IPADDR[17];				//监听地址

/*是否成为守护进程*/
extern int beDaemon;

/*日志配置*/
extern char ident[64];
extern int option;
extern int facility;

/*数据库配置*/
extern char dbdriver[128];			//数据库驱动程序名
extern char dbparams[256];			//数据库URL


/*读取配置文件 配置文件要放在/etc/authServer.conf*/
int set_config_value(char *name, char *value);

/*读取配置文件*/
int read_config();

#endif /*__READCONFIG_H__*/

