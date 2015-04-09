/** 调试信息打印函数
 */

#include<stdio.h>
#include<stdarg.h>
#include<stdlib.h>

#include"debug.h"

/* 打印信息到stdout 
 * 使用方法与printf一样，一般
 * 不直接使用这个函数，使用宏DEBUGMSG
 * DEBUGMSG(("a=%d'n",12));
 * */
void debugmsg(const char *format, ...)
{
	va_list         debugargs;
	va_start(debugargs, format);
	vprintf(format, debugargs);
	va_end(debugargs);
}
