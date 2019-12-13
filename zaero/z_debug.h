#ifndef __Z_DEBUG_H
#define __Z_DEBUG_H

#if defined(_DEBUG) && defined(_Z_TESTMODE)
void assertMsg(const char *msg, const char *file, int line);

#define DEBUG(expr, msg)	((!(expr)) ? assertMsg(msg, __FILE__, __LINE__),1 : 0)
#else
#define DEBUG(expr, msg)	0
#endif

#endif
