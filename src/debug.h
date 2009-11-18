#ifndef __DEBUG_H__
#define __DEBUG_H__
#include <stdlib.h>
#include "typedef.h"

void debug(size_t type, const char *ptr, ...);
void debugVerbose(size_t type, const char *ptr, ...);

#endif
