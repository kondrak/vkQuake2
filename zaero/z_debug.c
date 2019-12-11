#include "g_local.h"

void assertMsg(const char *msg, const char *file, int line)
{
	gi.dprintf("DEBUG[%s:%i] %s\n", file, line, msg);
}
