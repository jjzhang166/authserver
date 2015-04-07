#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>

#include "readConfig.h"


/*读取配置文件 配置文件要放在/etc/authServerd.conf*/
const char *config_file = "/etc/authServerd.conf";

int MAX_THREADS = 100;
unsigned short PORT = 12345;	  //监听端口
unsigned int MAX_TCP_QUEUE = 5;   //listen 的参数
char IPADDR[17];

/*是否成为守护进程*/
int beDeamon = 1;

/*日志配置*/
char ident[64] = "authServerd";
int option = LOG_CONS | LOG_PID;
int facility = 0;

/*数据库配置*/
char dbdriver[128] = "mysql";			 /*数据库驱动程序名*/
char dbparams[256] = "host=localhost;user=root;pass=123456;dbname=terminal";

/*读取配置文件 配置文件要放在/etc/authServer.conf*/
void set_config_value(char *name, char *value)
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
	else if(strcmp(name, "BEDEAMON") == 0)
			beDeamon = atoi(value);
	else if(strcmp(name, "MAXTHREADS") == 0)
		MAX_THREADS = atoi(value);
	else printf("Unknown name and value: %s %s\n", name, value);
}

/*读取配置文件*/
void read_config()
{
	char line[512];
	FILE *fp = fopen(config_file, "r");
	int t;
	if(fp == NULL)
	{
		printf("读取配置文件失败！请把配置文件放在/etc/authServer.conf\n");
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
	printf("读取配置文件(%s)成功\n", config_file);
}


