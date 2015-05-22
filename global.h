#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <semaphore.h>

#define BUF_SIZE 64     				//跳包16B, 认证包20B，接入信息包34B
extern sem_t sem;					//控制线程数

/*操作结果统一使用：*/
#define OPSUCCESS		0
#define OPFAIL		-1
#endif /* __GLOBAL_H__ */
