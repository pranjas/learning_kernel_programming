#include <common.h>
#include <linux/module.h>

static int __init load_module(void)
{
	struct module *__this_mod = THIS_MODULE;

	pr_debug("Loading module name %s\n", __this_mod->name);
	return 0; /*Extremely important!*/
}

static void __exit unload_module(void)
{
	struct module *__this_mod = THIS_MODULE;

	pr_debug("UnLoading module name %s\n", __this_mod->name);
}

module_init(load_module);
module_exit(unload_module);
