#ifndef __AUTH_PROTOCOL__
#define __AUTH_PROTOCOL__ 

#include<stdio.h>


typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned short u_short;


/* T字段 */
#define T_ZERO			  0x00
#define T_FREQ_BASE 	  0x01  /*频管与基站*/
#define T_FREQ_KERNEL	  0x02  /*频管与核心*/
#define T_FREQ_NETMANAGER 0x03  /*频管与网管*/

/* S字段 */
#define S_ZERO			  0x00
#define S_AUTH_REQUEST 	  0x01	/* 认证请求 上行*/
#define S_AUTH_REPLY	  0x02  /* 认证应答 下行*/
#define S_AUTH_FINISH	  0x03  /* 认证完成 上行*/
#define S_ACCESS_INFO	  0x04  /* 接入信息 上报*/

/* C字段 */
#define C_MOBILE_STATION 0x00  /* 移动台 */
#define C_BASE_STATION   0x01  /* 基站台 */
#define C_FREQ_MANAGER   0x02  /* 频  管 */

/*V字段 认证结果 */
#define V_SUCCESS		0x00
#define V_UNKNOWUSER	0x01
#define V_UNMATCH		0x02

/*在authProtocol.c中，返回操作结果统一使用：*/
#define OPSUCCESS		0
#define OPFAIL			-1


typedef struct MsgPDU
{
	u_int Len;
	u_char T;
	u_char S;
	char body[1];
}MsgPDU_t;

#define AUTH_PDU_LEN 20
typedef struct RequestPDU
{
	u_int Len;		/* PDU 长度 是20 */
	u_char T;		/* T_ZERO 即0    */
	u_char S;		/* S_AUTH_REQUEST */
	u_char Seq;		/* 认证序号 */
	u_char C;
	char Pin[8];	/* PIN 码	*/
	char Reserve[4];/* 保留 	*/
}RequestPDU_t;

typedef struct ReplyPDU
{
	u_int Len;		/* 长度20		 */
	u_char T;		/* T_ZERO 		 */
	u_char S;		/* S_AUTH_REPLY  */
	u_char Seq;		/* 认证序号 	 */
	u_char C;		/* 属性 		 */
	u_char V;		/* 认证结果 	 */
	char Pin[8];	/* PIN 码 		 */
	char Reserve[3];/*保留 			 */
}ReplyPDU_t;


#define ACCINFO_PDU_LEN 34
typedef struct AccessInfo
{
	u_int Len;			/*长度20*/
	u_char T;			/*T_ZERO*/
	u_char S;			/*S_ACCESS_INFO */
	u_char OpType;		/*操作类型 */
	u_char AR;			/*短波移动终端区号*/
	u_short MN;			/*短波移动终端号码*/
	u_char FREQ;		/*业务信道号*/
	u_char SNR;			/*链路质量 */
	char Reserve[6];		/*保留*/
	char BS_IP[16];		/*基站设备IP*/
}AccessInfo_t;

/* 心跳 */
#define HEART_BEAT_PDU_LEN 16
typedef struct HeartBeatPDU
{
	u_int Len;
	u_char T;
	u_char S;
	u_char body[10];
}HeartBeatPDU_t;


/* 只解析 前三项，用于初始协议区分 */
int parse_MsgPDU(MsgPDU_t *msg, u_char *buf, u_int  len);

/*解析认证请求数据包*/
int parse_RequestPDU(RequestPDU_t *req, const u_char *buf);

/*生成认证请求数据包*/
int build_RequestPDU(const RequestPDU_t *req, u_char *buf);

/*解析认证应答数据包*/
int parse_ReplyPDU(ReplyPDU_t *rep, const u_char *buf);

/*生成认证应答数据包*/
int build_ReplyPDU(const ReplyPDU_t *rep, u_char *buf);

/*解析心跳数据包*/
int parse_HeartBeatPDU(HeartBeatPDU_t *hbt, const u_char *buf);

/*生成心跳数据包*/
int build_HeartBeatPDU(const HeartBeatPDU_t *hbt,  u_char *buf);

/*解析接入信息*/
int parse_AccessInfo(AccessInfo_t *acci, const u_char *buf);

/*生成接入信息*/
int build_AccessInfo(const AccessInfo_t *acci, u_char *buf);

#endif /* __AUTH_PROTOCOL__ */
