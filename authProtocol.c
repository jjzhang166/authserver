/**	自定义协议解析包，生成包函数
 */

#include <memory.h>

#include "authProtocol.h"
#include "debug.h"


void bcd2str(const u_char *bcd, char *str)
{
	int i;
	for(i = 0; i < 4; i++)
	{
		str[i*2] = ((bcd[i] & 0xF0) >> 4) + '0';
		str[i*2+1] = (bcd[i] & 0x0F) + '0';
	}
}
void str2bcd(const char *str, u_char *bcd)
{
	int i;
	for(i = 0; i < 8; i+=2)
	{
		bcd[i/2] = ((str[i]-'0') << 4) | (str[i+1] - '0');
	}
	
}
/* 只解析 前三项，用于初始协议区分 */
int parse_MsgPDU(MsgPDU_t *msg, u_char *buf, u_int  len)
{
	if(len < 4)
		return OPFAIL;

	msg->Len = *(u_int *)buf;
	msg->T	 =	buf[4];
	msg->S	 =  buf[5];
	return OPSUCCESS;

}

/*解析认证请求数据包*/
int parse_RequestPDU(RequestPDU_t *req, const u_char *buf)
{
	req->Len = *(u_int*)buf;
	if(req->Len != AUTH_PDU_LEN)
		return OPFAIL;
	req->T =  buf[4];
	req->S = buf[5];
	req->Seq = buf[6];
	req->C = buf[7];

	bcd2str(buf+8+4, req->Pin);
	req->Pin[8] = '\0';	

	memcpy(req->Reserve, buf+16, 4);
	return OPSUCCESS;
}

/*生成认证请求数据包*/
int build_RequestPDU(const RequestPDU_t *req, u_char *buf)
{
	*(u_int*)buf = req->Len;
	buf[4]=	req->T;
	buf[5] = req->S;
	buf[6] = req->Seq;
	buf[7] = req->C;
	memset(buf+8, 0, 4);
	str2bcd(req->Pin, buf+8+4);
	memcpy(buf+16, req->Reserve, 4);
	return OPSUCCESS;
}

/*解析认证应答数据包*/
int parse_ReplyPDU(ReplyPDU_t *rep, const u_char *buf)
{
	rep->Len = *(u_int*)buf;
	if(rep->Len != AUTH_PDU_LEN)
		return OPFAIL;
	rep->T = buf[4];
	rep->S = buf[5];
	rep->Seq = buf[6];
	rep->C = buf[7];
	rep->V = buf[8];

	bcd2str(buf+9, rep->Pin);
	rep->Pin[8] = '\0';

	memcpy(rep->Reserve, buf+17, 3);
	return OPSUCCESS;
}

/*生成认证应答数据包*/
int build_ReplyPDU(const ReplyPDU_t *rep, u_char *buf)
{
	*(u_int*)buf = rep->Len;
	buf[4] = rep->T;
	buf[5] = rep->S;
	buf[6] = rep->Seq;
	buf[7] = rep->C;
	buf[8] = rep->V;

	str2bcd(rep->Pin, buf+9);

	memcpy(buf+17, rep->Reserve, 3);
	return OPSUCCESS;
}

/*解析心跳数据包*/
int parse_HeartBeatPDU(HeartBeatPDU_t *hbt,  const u_char *buf)
{
	hbt->Len = *(u_int*)buf;
	if(hbt->Len != HEART_BEAT_PDU_LEN)
		return OPFAIL;
	hbt->T   = buf[4];
	hbt->S   = buf[5];
	memcpy(hbt->body, buf+6, 10);
	return OPSUCCESS;
}

/*生成心跳数据包*/ 
int build_HeartBeatPDU(const HeartBeatPDU_t *hbt, u_char *buf) 
{
	*(u_int*)buf = hbt->Len;
	buf[4] = hbt->T;
	buf[5] = hbt->S;
	memcpy(buf+6, hbt->body, 10);
	return OPSUCCESS;
}

/*解析接入信息*/
int parse_AccessInfo(AccessInfo_t *acci, const u_char *buf)
{
	u_char tbcd[4]={0};
	acci->Len = *(u_int*)buf;
	if(acci->Len != ACCINFO_PDU_LEN)
		return OPFAIL;
	acci->T = buf[4];
	acci->S = buf[5];
	acci->OpType = buf[6];

	memcpy(tbcd+1, buf+7, 3);
	bcd2str(tbcd, acci->PIN);
	acci->PIN[8] = '\0';

	acci->FREQ = buf[10];
	acci->SNR = buf[11];
	memcpy(acci->Reserve, buf+12, 6);
	memcpy(acci->BS_IP, buf+18, 16);
	return OPSUCCESS;
}

/*生成接入信息*/
int build_AccessInfo(const AccessInfo_t *acci, u_char *buf)
{
	u_char bcd[4] = {0};
	*(u_int*)buf = acci->Len;
	buf[4]= acci->T;
	buf[5] = acci->S;
	buf[6] = acci->OpType;

	str2bcd(acci->PIN, bcd);
	memcpy(buf + 7, bcd + 1, 3); 
	
	buf[10] = acci->FREQ;
	buf[11] = acci->SNR;
	memcpy(buf+12, acci->Reserve, 6);
	memcpy(buf+18, acci->BS_IP, 16);
	return OPSUCCESS;
}
