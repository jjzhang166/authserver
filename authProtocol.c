#include "authProtocol.h"
#include <memory.h>

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
void parse_RequestPDU(RequestPDU_t *req, const u_char *buf)
{
	req->Len = *(u_int*)buf;
	req->T =  buf[4];
	req->S = buf[5];
	req->Seq = buf[6];
	req->C = buf[7];
	memcpy(req->Pin, buf+8, 8);
	memcpy(req->Reserve, buf+16, 4);
}

/*生成认证请求数据包*/
void build_RequestPDU(const RequestPDU_t *req, u_char *buf)
{
	*(u_int*)buf = req->Len;
	buf[4]=	req->T;
	buf[5] = req->S;
	buf[6] = req->Seq;
	buf[7] = req->C;
	memcpy(buf+8, req->Pin, 8);
	memcpy(buf+16, req->Reserve, 4);
}

/*解析认证应答数据包*/
void parse_ReplyPDU(ReplyPDU_t *rep, const u_char *buf)
{
	rep->Len = *(u_int*)buf;
	rep->T = buf[4];
	rep->S = buf[5];
	rep->Seq = buf[6];
	rep->C = buf[7];
	rep->V = buf[8];
	memcpy(rep->Pin, buf+9, 8);	
	memcpy(rep->Reserve, buf+17, 3);
}

/*生成认证应答数据包*/
void build_ReplyPDU(const ReplyPDU_t *rep, u_char *buf)
{
	*(u_int*)buf = rep->Len;
	buf[4] = rep->T;
	buf[5] = rep->S;
	buf[6] = rep->Seq;
	buf[7] = rep->C;
	buf[8] = rep->V;
	memcpy(buf+9,  rep->Pin, 8);
	memcpy(buf+17, rep->Reserve, 3);
}
