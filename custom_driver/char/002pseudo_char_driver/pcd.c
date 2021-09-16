#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#define MEM_SIZE 512

#undef  pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

/*Psudo device size*/
char device_buffer[MEM_SIZE];

/* This holds the device number */
dev_t device_number;

/* Cdev variables */
struct cdev pcd_cdev;

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
	loff_t temp;
	pr_info("lseek requested\n");
	pr_info("current value of file position is %lld\n",filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:
			if((offset > MEM_SIZE) || (offset < 0))
				return -EINVAL;
			filp->f_pos = offset;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + offset;
			if((temp > MEM_SIZE) || (temp < 0))
                                return -EINVAL;
			filp->f_pos += temp;
			break;
		case SEEK_END:
			temp = filp->f_pos + offset;
			if((temp > MEM_SIZE) || (temp < 0))
                                return -EINVAL;
			filp->f_pos = temp;
			break;
		default:
			return -EINVAL;
	}
	pr_info("updated value of file position is %lld\n",filp->f_pos);
	return filp->f_pos;
}
ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("read request from user of %zu byte\n",count);
	pr_info("current file position before reading = %lld\n", *f_pos);

	/* Adjust the count  */
	if((*f_pos + count) > MEM_SIZE)
		count = MEM_SIZE - *f_pos;

	/* Copy to user */
	if(copy_to_user(buff,&device_buffer[*f_pos],count)){
	   return -EFAULT;
	}
	/* update the current pointer location */
	*f_pos += count;

	pr_info("number of Byte successfully read = %zu\n", count);
	pr_info("updated file position is = %lld\n", *f_pos);

	/* return number of byte which has been successfully read */
	return count;
}
ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{

	pr_info("write request from user of %zu byte\n",count);
        pr_info("current file position before writing = %lld\n", *f_pos);

        /* Adjust the count  */
        if((*f_pos + count) > MEM_SIZE)
                count = MEM_SIZE - *f_pos;

	if(!count)
		return -ENOMEM;

        /* Copy from user */
        if(copy_from_user(&device_buffer[*f_pos],buff,count)){
           return -EFAULT;
        }
        /* update the current pointer location */
        *f_pos += count;

        pr_info("number of Byte successfully written = %zu\n", count);
        pr_info("updated file position is = %lld\n", *f_pos);

	return count;
}
int pcd_open(struct inode *inode, struct file *filp)
{
	pr_info("Open was successful\n");

	return 0;
}
int pcd_release(struct inode *inode, struct file *filp)
{
	pr_info("Release was successful\n");

	return 0;
}

/* File operation of driver */

//struct file_operations pcd_fops;
struct file_operations pcd_fops =
{


	.open = pcd_open,
	.write = pcd_write,
	.llseek = pcd_lseek,
	.release = pcd_release,
	.owner = THIS_MODULE

};

struct class *class_pcd;
struct device *device_pcd;

static int __init pcd_driver_init(void)
{
	int ret;

	/* 1.Dynamically allocate device number */
	ret = alloc_chrdev_region(&device_number,0,1,"pcd_devices");
	if(ret != 0)
	{	pr_err("alloc chrdev failed\n");
		goto out;
	}
	pr_info("Device number <major>:<minor> = %d:%d\n",MAJOR(device_number), MINOR(device_number));

	/* 2.initialize the cdev structure with fops */
	cdev_init(&pcd_cdev, &pcd_fops);

	/* 3.registration of module with VFS  */
	pcd_cdev.owner = THIS_MODULE;
	ret = cdev_add(&pcd_cdev,device_number,1);
	if(ret < 0)
	{
		pr_err("cdev add failed\n");
		goto unreg_chrdev;
	}

	/* 4.Create device class under /sys/class  */
	class_pcd = class_create(THIS_MODULE, "pcd_class");
	if(IS_ERR(class_pcd))
	{
		pr_err("class creation failed\n");
		ret = PTR_ERR(class_pcd);
		goto cdev_del;
	}

	/* 5.Populate the sysfs with devce information */
	device_pcd = device_create(class_pcd, NULL, device_number, NULL, "pcd");

	pr_info("Module init has been successful\n");
	if(IS_ERR(device_pcd))
	{
		pr_err("device creation failed\n");
		ret = PTR_ERR(device_pcd);
		goto class_del;
	}

   return 0;
class_del:
   class_destroy(class_pcd);

cdev_del:
   cdev_del(&pcd_cdev);
unreg_chrdev:
   unregister_chrdev_region(device_number,1);

out:
   pr_err("module insertion failed\n");
   return ret;
}

static void __exit pcd_driver_clean(void)
{

	device_destroy(class_pcd,device_number);
	class_destroy(class_pcd);
	cdev_del(&pcd_cdev);
	unregister_chrdev_region(device_number,1);
	pr_info("module unloaded\n");


}


module_init(pcd_driver_init);
module_exit(pcd_driver_clean);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ABHISHEK CHAUHAN");
MODULE_DESCRIPTION("ITS A SIMPLE PSUDO CHAR DRIVER FOR SUDO MEMORY BUFFER");
MODULE_INFO(board,"BegalBone Black Rev C");
