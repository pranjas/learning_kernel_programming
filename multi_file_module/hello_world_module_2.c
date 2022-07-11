#include <common.h>
#include <linux/module.h>
#include "helper.h"
#ifdef __HELLO_DEBUG__
#define NAME "HELLO_DEBUG_DEFINED"
#else
#define NAME "HELLO_DEBUG_UNDEFINED"
#endif

void func_from_2()
{
	pr_debug("From file %s, NAME = %s\n",
			__FILE__, NAME);
}
