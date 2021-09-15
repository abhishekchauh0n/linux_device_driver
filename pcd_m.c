#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

#define MEM_SIZE_PCD1 1024
#define MEM_SIZE_PCD2 512
#define MEM_SIZE_PCD3 1024
#define MEM_SIZE_PCD4 512

#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR   0x11

#define NO_OF_DEVICES 4

#undef  pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

/*Psudo device size*/
char device_buffer1[MEM_SIZE_PCD1];
char device_buffer2[MEM_SIZE_PCD2];
char device_buffer3[MEM_SIZE_PCD3];
char device_buffer4[MEM_SIZE_PCD4];

/* Device private data  */
struct pcdev_private_data
{
	char *buffer;
	unsigned size;
	const char *serial_number;
	int perm;
	struct cdev cdev;
};

/* Drivers private data  */
struct pcdrv_private_data
{
	int total_devices;
	/*This holds device number*/
	dev_t device_number;
	struct class *class_pcd;
	struct device *device_pcd;
	struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};

/*Device data initialization*/
struct pcdrv_private_data pcdrv_data =
{
	.total_devices = NO_OF_DEVICES,
	.pcdev_data = {
		[0] = {
			.buffer = device_buffer1,
			.size = MEM_SIZE_PCD1,
			.serial_number = "ABHI1995KNIT",
			.perm = RDONLY/* Read Only */
		},
		[1] = {
                        .buffer = device_buffer2,
                        .size = MEM_SIZE_PCD2,
                        .serial_number = "CHAU1995KNIT",
                        .perm = WRONLY/* Write Only */
                },
		[2] = {
                        .buffer = device_buffer3,
                        .size = MEM_SIZE_PCD3,
                        .serial_number = "HAN1995KNIT",
                        .perm = RDWR/* Read and write */
                },
		[0] = {
                        .buffer = device_buffer4,
                        .size = MEM_SIZE_PCD4,
                        .serial_number = "BHA1995KNIT",
                        .perm = RDWR/* Read and write */
                },
	}
};

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
	struct pcdev_private_data *pcdev_data = filp->private_data;
        int max_size = pcdev_data->size;

	loff_t temp;
	pr_info("lseek requested\n");
	pr_info("current value of file position is %lld\n",filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:
			if((offset > max_size) || (offset < 0))
				return -EINVAL;
			filp->f_pos = offset;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + offset;
			if((temp > max_size) || (temp < 0))
                                return -EINVAL;
			filp->f_pos += temp;
			break;
		case SEEK_END:
			temp = filp->f_pos + offset;
			if((temp > max_size) || (temp < 0))
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

	struct pcdev_private_data *pcdev_data = filp->private_data;
	int max_size = pcdev_data->size;
	pr_info("read request from user of %zu byte\n",count);
	pr_info("current file position before reading = %lld\n", *f_pos);

	/* Adjust the count  */
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;

	/* Copy to user */
	if(copy_to_user(buff,pcdev_data->buffer+(*f_pos),count)){
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

	struct pcdev_private_data *pcdev_data = filp->private_data;
        int max_size = pcdev_data->size;
	pr_info("write request from user of %zu byte\n",count);
        pr_info("current file position before writing = %lld\n", *f_pos);

        /* Adjust the count  */
        if((*f_pos + count) > max_size)
                count = max_size - *f_pos;

	if(!count)
		return -ENOMEM;

        /* Copy from user */
        if(copy_from_user(pcdev_data->buffer+(*f_pos),buff,count)){
           return -EFAULT;
        }
        /* update the current pointer location */
        *f_pos += count;

        pr_info("number of Byte successfully written = %zu\n", count);
        pr_info("updated file position is = %lld\n", *f_pos);

	return count;
}

int check_permission(int dev_perm, int acc_mode)
{
	// To ensure read and write access
	if(dev_perm == RDWR)
		return 0;
	// To ensure read access only
	if((dev_perm == RDONLY) && ((acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)))
		return 0;
	// To ensure write access only
	if((dev_perm == WRONLY) && ((acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ)))
                return 0;
	return -EPERM;
}

int pcd_open(struct inode *inode, struct file *filp)
{
	int ret;
	int minor_n;

	struct pcdev_private_data *pcdev_data;
	/*find out on which device file open was attempted by user space*/

	minor_n = MINOR(inode->i_rdev);
	pr_info("minor access: %d\n",minor_n);
	
	/*Get device's privte data structure*/
	//container_of(ptr, type, member);
	//ptr---> should point to the member
	//type---> type of container struct in which member is embedded in
	//member---> name of the member ptr refers to
	pcdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);

	/*to supply device private data to other method of driver*/
	filp->private_data = pcdev_data;

	/*Check permission*/
	ret = check_permission(pcdev_data->perm, filp->f_mode);
	(!ret)?pr_info("module was successful\n"):pr_info("module was unsuccessful\n");
	return 0;
}
int pcd_release(struct inode *inode, struct file *filp)
{

	return 0;
}

/* File operation of driver */

//struct file_operations pcd_fops;
struct file_operations pcd_fops=
{


	.open = pcd_open,
	.write = pcd_write,
	.llseek = pcd_lseek,
	.release = pcd_release,
	.owner = THIS_MODULE

};

static int __init pcd_driver_init(void)
{
	int ret;
	int i;

	/* 1.Dynamically allocate device number */
	ret = alloc_chrdev_region(&pcdrv_data.device_number,0,NO_OF_DEVICES,"pcd_devices");
	if(ret != 0){
		pr_err("alloc chrdev failed\n");
		goto out;
	}
	/* Create device class under /sys/class  */
        pcdrv_data.class_pcd = class_create(THIS_MODULE, "pcd_class");
        if(IS_ERR(pcdrv_data.class_pcd))
        {
                pr_err("class creation failed\n");
                ret = PTR_ERR(pcdrv_data.class_pcd);
                goto unreg_chrdev;
        }

	for(i=0;i<NO_OF_DEVICES;i++){
		pr_info("Device number <major>:<minor> = %d:%d\n",MAJOR(pcdrv_data.device_number+i), MINOR(pcdrv_data.device_number+i));

		/* initialize the cdev structure with fops */
		cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);

		/* registration of module with VFS  */
		pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
		ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev,pcdrv_data.device_number,1);
		if(ret < 0){
			pr_err("cdev add failed\n");
			goto cdev_del;
		}
	}

	/* Populate the sysfs with devce information */
	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, pcdrv_data.device_number, NULL, "pcd-%d",i+1);
	pr_info("Module init has been successful\n");
	if(IS_ERR(pcdrv_data.device_pcd))
	{
		pr_err("device creation failed\n");
		ret = PTR_ERR(pcdrv_data.device_pcd);
		goto class_del;
	}

   return 0;

cdev_del:
class_del:
   for(;i>=0;i--)
   {
	   device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i);
	   cdev_del(&pcdrv_data.pcdev_data[i].cdev);
   }
   class_destroy(pcdrv_data.class_pcd);
unreg_chrdev:
   unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);

out:
   pr_err("module insertion failed\n");
   return ret;
}

static void __exit pcd_driver_clean(void)
{
	int i;
	for(i=0;i<NO_OF_DEVICES;i++)
  	 {
           device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i);
           cdev_del(&pcdrv_data.pcdev_data[i].cdev);
  	 }
  	 class_destroy(pcdrv_data.class_pcd);
	 pr_info("module has been unloaded successfully!");
}


module_init(pcd_driver_init);
module_exit(pcd_driver_clean);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ABHISHEK CHAUHAN");
MODULE_DESCRIPTION("ITS A SIMPLE PSUDO CHAR DRIVER FOR MULTIPLE SUDO MEMORY BUFFER DEVICES");
MODULE_INFO(board,"BegalBone Black Rev C");
