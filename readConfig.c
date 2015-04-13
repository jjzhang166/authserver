/** 所有配置信息
 *  从配置文件读取配额信息
 */
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>

#include "readConfig.h"
#include "debug.h"

#define IP_LEN 16
#define DBDRIVER_LEN 128
#define DBPARAMS_LEN 256
#define LOG_IDENT_LNE 64

/*读取配置文件 配置文件要放在/etc/nm_fremd.conf*/
const char *config_file = "/etc/nm_fremd.conf";

/*认证线程控制*/
int MAX_THREADS = 100;			  

/*监听端口*/
unsigned short PORT = 12345;	 

/*listen 的参数*/
unsigned int MAX_TCP_QUEUE = 5;   

/*监听IP*/
char IPADDR[IP_LEN+1];				  

/*是否成为守护进程*/
int beDaemon = 1;

/*日志配置*/
char ident[LOG_IDENT_LNE] = "nm_fremd";
int option = LOG_CONS | LOG_PID;
int facility = 0;

/*数据库配置*/
char dbdriver[DBDRIVER_LEN] = "mysql";			 /*数据库驱动程序名*/
char dbparams[DBPARAMS_LEN] = "host=localhost;user=root;pass=123456;dbname=terminal";

/*读取配置文件 配置文件要放在/etc/nm_fremd.conf*/
int set_config_value(char *name, char *value)
{

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
	else if(strcmp(name, "BEDAEMON") == 0)
			beDaemon = atoi(value);
	else if(strcmp(name, "MAXTHREADS") == 0)
		MAX_THREADS = atoi(value);
	else
	{
		 DEBUGMSG(("Unknown name and value: %s %s\n", name, value));
		 return -1;
	}
	return 0;
}

/*读取配置文件
 * 成功返回0
 * 失败返回-1
 * 读取配置文件失败，退出服务器
 * */
int read_config()
{
	char line[512];
	FILE *fp = fopen(config_file, "r");
	int t;
	if(fp == NULL)
	{
		printf("读取配置文件失败！请把配置文件放在/etc/nm-fremd.conf\n");
		return -1;
	}

	while(fgets(line, 512, fp) != NULL)
	{
		int len = strlen(line);
		char *ns, *vs, *p;
		int i = 0;
		if(line[len-1] != '\n')
			while(fgetc(fp)!='\n');
		DEBUGMSG(("%s", line));

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

		DEBUGMSG(("config: name: %s \t value:%s\n", ns, vs));
		if(set_config_value(ns, vs)==-1)
			return -1;
	}

	DEBUGMSG(("读取配置文件(%s)成功\n", config_file));
	return 0;
}


