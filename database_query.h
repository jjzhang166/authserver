#ifndef __DATABASE_QUERY_H__
#define __DATABASE_QUERY_H__

/** 初始化apr 库 
*/
void init_database();

/** 停止apr 库 
*/
void destroy_database();

/* 检测PIN 
 * pin: ping码
 * 返回值：
 * 		成功返回OPSUCCESS
 * 		失败返回OPFAIL
 */
int dbd_check_pin(const char *strpin);

/**做SQL insert update delete
 * sqlcmd	要执行的SQL命令
 * 返回:    受影响的行数
 *
 */
int do_dbd_query(const char *sqlcmd);
#endif /* __DATABASE_QUERY_H__ */
