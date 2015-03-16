#include <stdio.h>
#include "authProtocol.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#define PORT 12345
#define serip "127.0.0.1"
int main()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in seraddr;
	seraddr.sin_family = AF_INET;	
	seraddr.sin_port = htons(PORT);
	seraddr.sin_addr.s_addr = inet_addr(serip);
	bzero(seraddr.sin_zero, 8);
	if(connect(fd, (struct sockaddr*)&seraddr, sizeof(struct sockaddr)) < 0)
	{
		printf("Connect failed!\n");
		return -1;
	}
	
	int i = 0;
	while(i++ < 10000)
	{
		RequestPDU_t req = {20, T_ZERO, S_AUTH_REQUEST,(u_char)i, C_MOBILE_STATION, "12345678"};
		printf("len=%d T=%d pin=%s\n",req.Len, req.T,  req.Pin);
		char buf[AUTH_PDU_LEN];
		build_RequestPDU(&req, buf);
		send(fd, buf, AUTH_PDU_LEN,0);
		
		req.S = S_AUTH_FINISH;
		build_RequestPDU(&req, buf);
		send(fd, buf, AUTH_PDU_LEN,0);
		//usleep(10000);
	}
	sleep(5);
	close(fd);
}
