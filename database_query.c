/** 所有数据库操作相关
*/
#include <stdio.h>
#include <apu.h>
#include <apr_pools.h>
#include <apr_dbd.h>
#include <syslog.h>

#include "global.h"
#include "debug.h"
#include "database_query.h"
#include "read_config.h"


/** 初始化apr 库 
*/
void init_database()
{
     apr_initialize();;
}

/** 停止apr 库 
*/
void destroy_database()
{
    apr_terminate();
}

/* 检测PIN 
 * pin: ping码
 * 返回值：
 * 		成功返回OPSUCCESS
 * 		失败返回OPFAIL
 */
int dbd_check_pin(const char *strpin)
{	

    int rv = 0;
    int i = 0;
    int n;
    const char* entry = NULL;
    char statement[128] = "SELECT * FROM terminals where Pin='";
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row = NULL;
    apr_pool_t *respool  = NULL;
    apr_dbd_t *sql = NULL;
    const apr_dbd_driver_t *driver = NULL;	

    //构建查询语句
    strcat(statement,strpin);
    strcat(statement,"'");
	
    //查询数据库
    rv = apr_pool_create(&respool, NULL);
	if(rv != APR_SUCCESS)
		goto finish;
    apr_dbd_init(respool);
    rv = apr_dbd_get_driver(respool, dbdriver, &driver);
	switch (rv) 
	{
       	case APR_SUCCESS:
       		DEBUGMSG(("Loaded %s driver OK.\n", dbdriver));
          	break;
        case APR_EDSOOPEN:
           syslog(LOG_ERR,"Failed to load driver file apr_dbd_%s.so\n", dbdriver);
           goto finish;
        case APR_ESYMNOTFOUND:
           syslog(LOG_ERR,"Failed to load driver apr_dbd_%s_driver.\n", dbdriver);
           goto finish;
        case APR_ENOTIMPL:
           syslog(LOG_ERR,"No driver available for %s.\n", dbdriver);
           goto finish;
        default:        /* it's a bug if none of the above happen */
           syslog(LOG_ERR,"Internal error loading %s.\n", dbdriver);
           goto finish;
        }
	rv = apr_dbd_open(driver, respool, dbparams, &sql);
    switch (rv)
	 {
        case APR_SUCCESS:
           DEBUGMSG(("Opened %s[%s] OK\n", dbdriver, dbparams));
           break;
        case APR_EGENERAL:
           syslog(LOG_ERR,"Failed to open %s[%s]\n", dbdriver, dbparams);
           goto finish;
        default:        /* it's a bug if none of the above happen */
           syslog(LOG_ERR,"Internal error opening %s[%s]\n", dbdriver, dbparams);
           goto finish;
        }

    rv = apr_dbd_select(driver,respool,sql,&res,statement,0);
    if (rv)
	{
        syslog(LOG_ERR,"Select failed: %s", apr_dbd_error(driver, sql, rv));
        goto finish2;
    }

   i = 0;
    for (rv = apr_dbd_get_row(driver, respool, res, &row, -1);
         rv == 0;
         rv = apr_dbd_get_row(driver, respool, res, &row, -1))
	{
        //printf("ROW %d:	", i) ;
        ++i;
        for (n = 0; n < apr_dbd_num_cols(driver, res); ++n)
		{
            entry = apr_dbd_get_entry(driver, row, n);
            if (entry == NULL) 
			{
               // printf("(null)	") ;
               ;
            }
            else 
			{
                //printf("%s	", entry);
                ;
            }
        }
    }
	finish2:
		apr_dbd_close(driver, sql);
	finish: ;		
		apr_pool_destroy(respool);

	if(i>0)
		return OPSUCCESS;
	else
		return OPFAIL;
}


/**做SQL insert update delete
 * sqlcmd	要执行的SQL命令
 * 返回:    受影响的行数
 *
 */
int do_dbd_query(const char *sqlcmd)
{	

    int rv = 0;
	int nrows;
    //apr_dbd_results_t *res = NULL;
	apr_pool_t *respool  = NULL;
	apr_dbd_t *sql = NULL;
	const apr_dbd_driver_t *driver = NULL;

	
	//查询数据库
    rv = apr_pool_create(&respool, NULL);
	if(rv != APR_SUCCESS)
		goto finish;
    apr_dbd_init(respool);
    rv = apr_dbd_get_driver(respool, dbdriver, &driver);
	switch (rv) 
	{
       	case APR_SUCCESS:
       		DEBUGMSG(("Loaded %s driver OK.\n", dbdriver));
          	break;
        case APR_EDSOOPEN:
           syslog(LOG_ERR,"Failed to load driver file apr_dbd_%s.so\n", dbdriver);
           goto finish;
        case APR_ESYMNOTFOUND:
           syslog(LOG_ERR,"Failed to load driver apr_dbd_%s_driver.\n", dbdriver);
           goto finish;
        case APR_ENOTIMPL:
           syslog(LOG_ERR,"No driver available for %s.\n", dbdriver);
           goto finish;
        default:        /* it's a bug if none of the above happen */
           syslog(LOG_ERR,"Internal error loading %s.\n", dbdriver);
           goto finish;
        }
	rv = apr_dbd_open(driver, respool, dbparams, &sql);
    switch (rv)
	 {
        case APR_SUCCESS:
           DEBUGMSG(("Opened %s[%s] OK\n", dbdriver, dbparams));
           break;
        case APR_EGENERAL:
           syslog(LOG_ERR,"Failed to open %s[%s]\n", dbdriver, dbparams);
           goto finish;
        default:        /* it's a bug if none of the above happen */
           syslog(LOG_ERR,"Internal error opening %s[%s]\n", dbdriver, dbparams);
           goto finish;
        }

    rv = apr_dbd_query(driver, sql, &nrows, sqlcmd);
    if (rv)
	{
        syslog(LOG_ERR,"Query failed: %s", apr_dbd_error(driver, sql, rv));
        goto finish2;
    }

   finish2:
		apr_dbd_close(driver, sql);
	finish: ;		
		apr_pool_destroy(respool);

	/*if(rv == 0)
		return OPSUCCESS;
	else
		return OPFAIL;*/
	return nrows;
}
