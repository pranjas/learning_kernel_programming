#include <common.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <linux/uaccess.h>

#ifdef USE_LOCKING
#include <linux/mutex.h>
#endif
struct dummy_misc_dev {
	struct miscdevice misc_dev;
	struct page *dev_buffer;
#ifdef USE_LOCKING
	struct mutex lock;
#endif
	size_t len;
};

/*
 * Modify this function if you require more than one page
 * of storage.
 * */
static size_t dummy_stored_data_len(struct dummy_misc_dev *dummy_dev)
{
	return dummy_dev->len;
}

/*
 * Modify this function if you require more than one page of
 * storage or some other logic..
 * */
static void dummy_stored_data_len_set(struct dummy_misc_dev *dummy_dev, size_t len)
{
	dummy_dev->len = len;
}

/*
 * File position will be wrapped after page size
 * */
ssize_t misc_write(struct file *fp, const char __user * buf, size_t bufsize,
			loff_t *pos)
{
	/*
	 * Get the containing device structure.
	 * */
	struct dummy_misc_dev *dummy_dev = 
		container_of(fp->private_data, struct dummy_misc_dev,
			       	misc_dev);
	loff_t current_pos = 0;
	size_t dev_buf_rem = 0;
	char *dev_buffer = NULL;
	const size_t to_write = bufsize;
	int ret = 0;
#ifdef USE_LOCKING
	mutex_lock(&dummy_dev->lock);
#endif
	current_pos = *pos % PAGE_SIZE;
	dev_buf_rem = PAGE_SIZE - current_pos;
	dev_buffer = kmap(dummy_dev->dev_buffer);
	pr_debug("Entering %s\n", __func__);

	while (!ret && bufsize > 0) {
		size_t to_copy = bufsize > dev_buf_rem ? dev_buf_rem : bufsize;
		if (copy_from_user(dev_buffer + current_pos, buf, to_copy)) {
			ret = -EACCES;
		} else {
			buf = buf + to_copy;
			*pos += to_copy;;
			current_pos = *pos % PAGE_SIZE;
			bufsize -= to_copy;
		}
	}
	dummy_stored_data_len_set(dummy_dev, current_pos);
	kunmap(dummy_dev->dev_buffer);
	pr_debug("Exiting %s\n", __func__);
#ifdef USE_LOCKING
	mutex_unlock(&dummy_dev->lock);
#endif
	return to_write - bufsize;
}

/*
 * Our dummy device has at most one PAGE of storage.
 * Use that length, otherwise some common programs would
 * keep on reading from the device.
 * */
static ssize_t misc_read(struct file *fp, char __user *buf,
				size_t bufsize, loff_t *pos)
{
	int ret = 0;
	loff_t current_pos = *pos % PAGE_SIZE;
	/*
	 * Get the containing device structure.
	 * */
	struct dummy_misc_dev *dummy_dev = 
		container_of(fp->private_data, struct dummy_misc_dev,
				misc_dev);

	size_t dev_buf_rem = dummy_stored_data_len(dummy_dev);
	
	char *dev_buffer = kmap(dummy_dev->dev_buffer);
	
	size_t to_copy =  0;

	if (dev_buf_rem > current_pos)
		to_copy = bufsize > dev_buf_rem - current_pos ? dev_buf_rem - current_pos : bufsize;
	
	pr_debug("Entering %s\n", __func__);

	if (copy_to_user(buf, dev_buffer + current_pos, to_copy)) {
		ret = -EACCES;
	} else
		*pos += to_copy;
	kunmap(dummy_dev->dev_buffer);
	pr_debug("Exiting %s\n", __func__);
	return !ret ? to_copy : ret;
}

static int misc_open(struct inode *inode, struct file *fp)
{
	struct dummy_misc_dev *dummy_dev =
		container_of(fp->private_data, struct dummy_misc_dev,
				misc_dev);
	pr_debug("Entering %s\n", __func__);
	if (!PageHighMem(dummy_dev->dev_buffer))
		pr_debug("In %s, page is lowmem for device %s\n",
				__func__,
				dummy_dev->misc_dev.name);
	else
		pr_debug("In %s, page is highmem for device %s\n",
				__func__,
				dummy_dev->misc_dev.name);
	pr_debug("Exiting %s\n", __func__);
	return 0;
}

static int misc_release(struct inode *inode, struct file *fp)
{
	pr_debug("Entering %s\n", __func__);
	pr_debug("Exiting %s\n", __func__);
	return 0;
}

struct file_operations misc_dev_fops = {
	.read = misc_read,
	.write = misc_write,
	.open = misc_open,
	.release = misc_release
};

static struct dummy_misc_dev dummy_dev = {
	.misc_dev = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "dummy_misc_dev",
		.fops = &misc_dev_fops
	},
};

static int __init  dummy_misc_module_init(void)
{
	int ret = 0;
	pr_debug("Entering %s\n", __func__);
	dummy_dev.dev_buffer = alloc_page(GFP_KERNEL);
#ifdef USE_LOCKING
	mutex_init(&dummy_dev.lock);
#endif
	if (!dummy_dev.dev_buffer) {
		ret = -ENOMEM;
		goto out;
	}
	ret =  misc_register(&dummy_dev.misc_dev);
	if (ret)
		put_page(dummy_dev.dev_buffer);
out:
	pr_debug("Exiting %s ret = %d\n", __func__, ret);
	return ret;
}

static void  __exit misc_module_unload(void)
{
	pr_debug("Entering %s\n", __func__);
	misc_deregister(&dummy_dev.misc_dev);
	/*
	 * These are useful when you know something can't happen
	 * and help in tracing bugs.
	 * */
	BUG_ON(page_ref_count(dummy_dev.dev_buffer) != 1);
	put_page(dummy_dev.dev_buffer);
	dummy_dev.dev_buffer = NULL;
	pr_debug("Exiting %s\n", __func__);
}

module_init(dummy_misc_module_init);
module_exit(misc_module_unload);
