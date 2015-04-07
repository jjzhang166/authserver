#include <memory.h>

#include "authProtocol.h"


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
	memcpy(req->Pin, buf+8, 8);
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
	memcpy(buf+8, req->Pin, 8);
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
	memcpy(rep->Pin, buf+9, 8);	
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
	memcpy(buf+9,  rep->Pin, 8);
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
	acci->Len = *(u_int*)buf;
	if(acci->Len != ACCINFO_PDU_LEN)
		return OPFAIL;
	acci->T = buf[4];
	acci->S = buf[5];
	acci->OpType = buf[6];
	acci->AR = buf[7];
	acci->MN = *(u_short *)(buf+8);
	acci->FREQ = buf[10];
	memcpy(acci->Reserve, buf+11, 6);
	memcpy(acci->BS_IP, buf+17, 16);
	return OPSUCCESS;
}

/*生成接入信息*/
int build_AccessInfo(const AccessInfo_t *acci, u_char *buf)
{
	*(u_int*)buf = acci->Len;
	buf[4]= acci->T;
	buf[5] = acci->S;
	buf[6] = acci->OpType;
	buf[7] = acci->AR;
	*(u_short*)(buf+8) = acci->MN;
	buf[10] = acci->FREQ;
	memcpy(buf+11, acci->Reserve, 6);
	memcpy(buf+17, acci->BS_IP, 16);
	return OPSUCCESS;
}
