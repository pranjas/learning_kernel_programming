#include <common.h>
#include <linux/module.h>
#include "helper.h"

#ifdef __HELLO_DEBUG__
#define NAME "DEFINED"
#else
#define NAME "UNDEFINED"
#endif

static int __init load_module(void)
{
	struct module *__this_mod = THIS_MODULE;
	func_from_2();
	pr_debug("Loading module name %s\n", __this_mod->name);
	pr_debug("In file %s, NAME = %s\n", __FILE__, NAME);
	return 0; /*Extremely important!*/
}

static void __exit unload_module(void)
{
	struct module *__this_mod = THIS_MODULE;

	pr_debug("UnLoading module name %s\n", __this_mod->name);
}

module_init(load_module);
module_exit(unload_module);
