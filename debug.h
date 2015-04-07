#ifndef __DEBUG_H__
#define __DEBUG_H__

/*调试信息*/
#define _DEBUG_IF_ 1
#define _DEBUG_ON_
#ifdef _DEBUG_ON_
#define DEBUGMSG(x) do {if (_DEBUG_IF_) { printf("%s(%d)-%s: ",__FILE__, __LINE__, __FUNCTION__); debugmsg x;} }while (0)
#else
#define DEBUGMSG(x)
#endif

#endif
