#include<stdio.h>
#include<stdarg.h>
#include<stdlib.h>
#include"debug.h"
void debugmsg(const char *format, ...)
{
	va_list         debugargs;
	va_start(debugargs, format);
	vprintf(format, debugargs);
	va_end(debugargs);
}
