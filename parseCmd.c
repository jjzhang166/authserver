/**解析命令行参数
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "parseCmd.h"
#include "readConfig.h"

/** 帮助信息
 */
void usage()
{
	printf("Welcom to use authServer...\n");
	printf("-h print this usage.\n");
	printf("-f don't become a daemon.\n");
	printf("-i server's listen ip address.\n");
	printf("-p server's listen port.\n");
	printf("-d database's driver. such as mysql\n");
	printf("-m database's params. such as host=localhost;user=root;pass=123456;dbname=terminal\n");  
	printf("--config file is %s\n", config_file);
	printf("\n");
}

/** 解析命令行参数
 * 	argc, argv 对应main函数的argc argv
 */
void parse_cmd_line(int argc, char *argv[])
{
	int ch;
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
				beDaemon = 0;
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
				printf(" try -h to see help.\n");	
				exit(-1);
		}
	}
}

