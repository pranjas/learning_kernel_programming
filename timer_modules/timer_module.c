#include <common.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/moduleparam.h>


#define MAX_CARTOON_TIMES	20

static int MAX_CARTOONS = 20;
module_param(MAX_CARTOONS, int, 0);
MODULE_PARM_DESC(MAX_CARTOONS, "Maximum number of cartoons to create");
/*
 * The timer is embedded within a larger struct
 * so it's easier to get reference of the data
 * we required.
 * */

struct cartoon {
	struct timer_list timer;
	char name[32]; /*Cartoons have small names.*/
	int times; /*How many times this cartoon must appear*/
	bool done;
};

static struct kmem_cache *cachep = NULL;

static void timer_func(struct timer_list *which_timer)
{
	/*
	 * Timer's don't have a process context, so it means
	 * we don't have a valid current, but still it can be
	 * anything.
	 *
	 * No sleepy stuff here, GFP_ATOMIC is the only safe flag,
	 * no mutex only spin_lock can be used here.
	 * And also no per_cpu data in this function.
	 * */
	struct cartoon *cartoon = container_of(which_timer, struct cartoon,
						timer);
	unsigned long random_time;
	BUG_ON(!cartoon);
	/*
	 * printk is safe for even atomic stuff!
	 * */
	pr_debug("Cartoon %s has just come to meddle...\n", cartoon->name);
	get_random_bytes(&random_time, sizeof(random_time));
	random_time = 1 + (random_time % 60); /*Let's limit to at least 1 second*/
	if (cartoon->times > 0) {
		mod_timer(which_timer,
				jiffies + msecs_to_jiffies(random_time * 1000));
		cartoon->times--;
		pr_debug("Cartoon %s will now come after %ld seconds\n",
				cartoon->name,random_time);
	} else {
		/*
		 * Ask for this timer to be removed.
		 * */
		cartoon->done = true;
		pr_debug("Cartoon %s is now done troubling us\n", cartoon->name);
	}
	pr_debug("Exiting %s for cartoon %s\n", __func__, cartoon->name);
}

/*
 * This is called for all the objects created in the cache
 * not just one. So be careful not to initialize the timer or
 * other unnecessary stuff here.
 * Just initialize what is necessary to work a new cartoon object.
 * */
static void __init cartoon_ctor(void *obj)
{
	static int i = 0;
	struct cartoon *cartoon = (struct cartoon*) obj;
	cartoon->done = false;
	get_random_bytes(&cartoon->times, sizeof(cartoon->times));
	cartoon->times = 5 + ((unsigned long)(cartoon->times) % MAX_CARTOON_TIMES);
	sprintf(cartoon->name,"cartoon-%d", ++i);
}

static void setup_cartoon_timer(struct cartoon *cartoon)
{
	unsigned long random_time; 
	get_random_bytes(&random_time, sizeof(random_time));
	random_time = 1 + (random_time % 60); /*Let's limit to at least 1 second*/
	cartoon->timer.expires = jiffies + 
		msecs_to_jiffies(random_time * 1000);
	timer_setup(&cartoon->timer, timer_func, 0);
	pr_debug("Setting up timer for %s for %ld seconds\n", cartoon->name,
			random_time);
	pr_debug("Cartoon %s will appear %d times\n",cartoon->name,
			cartoon->times);
	add_timer(&cartoon->timer);
}

static int __init init_cartoon_cache(void)
{
	cachep = kmem_cache_create("cartoon-cache", sizeof(struct cartoon), 0,
					SLAB_POISON, cartoon_ctor);
	if (!cachep) {
		pr_debug("Couldn't initialize the mem cache for cartoons :(\n");
		return -EINVAL;
	}
	return 0;
}

static bool setup_cartoons(struct cartoon **cartoons, int num)
{
	int i = 0;
	bool init_ok = true;
	while (num-- > 0) {
		cartoons[i] = kmem_cache_alloc(cachep, GFP_KERNEL);
		if (!cartoons[i])
			init_ok = false;
		else
			setup_cartoon_timer(cartoons[i]);
		i++;
	}
	return init_ok;
}


struct cartoon **all_cartoons = NULL;

static void cancel_cartoon_timer_sync(struct cartoon *cartoon)
{
	if (!cartoon)
		return;
	del_timer_sync(&cartoon->timer);
}

static void cancel_all_cartoons(struct cartoon **cartoons)
{
	int i = 0;
	for (; i < MAX_CARTOONS; i++) {
		cancel_cartoon_timer_sync(cartoons[i]);
	}
}

static void cancel_and_free_all_cartoons(struct cartoon **cartoons)
{
	int i = 0;
	cancel_all_cartoons(cartoons);
	for (; i < MAX_CARTOONS; i++) {
		if (cartoons[i])
			kmem_cache_free(cachep, cartoons[i]);
	}
}

static int __init cartoon_module_load(void)
{
	int ret = 0;
	ret = init_cartoon_cache();
	if (ret)
		goto error;
	all_cartoons = kmalloc(MAX_CARTOONS * sizeof(void *), GFP_KERNEL);
	
	if (!all_cartoons) {
		ret = -ENOMEM;
		goto destroy_cache;
	}
	if (!setup_cartoons(all_cartoons, MAX_CARTOONS)) {
		ret = -ENOMEM;
		goto setup_cartoon_fail;
	}
	return 0;
setup_cartoon_fail:
	cancel_and_free_all_cartoons(all_cartoons);
	kfree(all_cartoons);
destroy_cache:
	kmem_cache_destroy(cachep);
error:
	return ret;
}

static void __exit cartoon_module_unload(void)
{
	/*
	 * First cancel all timers.
	 * */
	cancel_and_free_all_cartoons(all_cartoons);
	kfree(all_cartoons);
	kmem_cache_destroy(cachep);
	pr_debug("Cartoon module has now been unloaded\n");
}

module_init(cartoon_module_load);
module_exit(cartoon_module_unload);
