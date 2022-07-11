#include <common.h>
#include <linux/sysfs.h>

#define ROOT_KOBJ_NAME          "pks_kobj"
#define ROOT_ATTR1_NAME         "pks_kobj_attr1"
#define ROOT_ATTR2_NAME         "pks_kobj_attr2"
#define ROOT_ATTRS_COUNT        2

ssize_t rootfs_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf);

ssize_t rootfs_store(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t count);


/* 1 Word of storage per attribute */
#define ROOT_ATTR_STORAGE_SIZE  (ROOT_ATTRS_COUNT * sizeof(long))
/*
 * This is our directory sort of for our sysfs files
 */
struct kobject *root_kobj;

/*
 * These are the files under pks_kobj we'll see. 
 * */
struct kobj_attribute root_kobj_attr1 = __ATTR(root_kobj_attr1, S_IWUSR|S_IRUGO,
		rootfs_show, rootfs_store);

struct kobj_attribute root_kobj_attr2 = __ATTR(root_kobj_attr2, S_IWUSR|S_IRUGO,
		rootfs_show, rootfs_store);

const struct attribute *root_kobj_attr[] = {    &root_kobj_attr1.attr,
	&root_kobj_attr2.attr, NULL};
/*
 * We need storage to get/put data from/to user land. Let's just create 
 * a static array for this.
 */
static char attribute_storage[ROOT_ATTR_STORAGE_SIZE];

static int __init init_sysfs_objs(struct kobject *root_kobj_parent)
{
	int err = 0;
	root_kobj = kobject_create_and_add(ROOT_KOBJ_NAME, root_kobj_parent);
	if (!root_kobj) {
		err = -ENOMEM;
		goto no_root_kobj;
	}
	err = sysfs_create_files(root_kobj, root_kobj_attr);
	if (err)
		goto err_create_files;
	return 0;

err_create_files:
	kobject_put(root_kobj);
no_root_kobj:
	return err;
}
static int __init load_module(void)
{
	struct module *__this_mod = THIS_MODULE;
	if (__this_mod)
		pr_debug("Loading module %s\n", __this_mod->name);
	return init_sysfs_objs(NULL);
}
static void __exit cleanup_sysfs_objs(void)
{
	sysfs_remove_files(root_kobj, root_kobj_attr);
	kobject_put(root_kobj);
}

static void __exit unload_module(void)
{
	struct module *__this_mod = THIS_MODULE;
	if (__this_mod)
		pr_debug("Unloading module %s\n", __this_mod->name);
	cleanup_sysfs_objs();
}

ssize_t rootfs_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf) {
	long *storage = (long*)attribute_storage;
	int written = 0;
	if (attr == &root_kobj_attr2) {
		storage++;
	}
	written = snprintf(buf, PAGE_SIZE, "%ld", *storage);
	if (written >= PAGE_SIZE)
		buf[PAGE_SIZE - 1] = '\0';
	return strlen(buf);
}

ssize_t rootfs_store(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t count)
{
	int ret = 0;
	long *storage = (long*)attribute_storage;
	if (attr == &root_kobj_attr2) {
		storage++;
	}
	ret = kstrtol(buf, 10, storage); 
	if (ret) {
		pr_debug("Requires only decimal number. Entered %s\n", buf);
	} else {
		pr_debug("Changing to  %ld \n", *storage);
		ret = sizeof(long);
	}
	return ret;
}

module_init(load_module);
module_exit(unload_module);
