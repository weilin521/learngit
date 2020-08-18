#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>



static struct class *firstdrv_class;
static struct class_device *firstdrv_class_dev;

//定义全局变量，寄存器GPFCON寄存器
volatile unsigned long *gpfcon = NULL;
volatile unsigned long *gpfdat = NULL;



static int first_drv_open(struct inode *inode,struct file *file)
{
	//printk("first_drv_open\n");
	/*配置GPF4,5,6为输出*/
	//清0
	*gpfcon &= ~((0x3<<(4*2))|(0x3<<(5*2))|(0x3<<(6*2)));
	//1为输出
	*gpfcon |= ((0x1<<(4*2))|(0x1<<(5*2))|(0x1<<(6*2)));
	return 0;
}
static ssize_t first_drv_write(struct file *file ,const char __user *buf, size_t count,loff_t * ppos)
{
	int val;
	//取用户程序传进来的值，用户空间到内核空间传递数据用copy_from_user()
	//内核空间传递到用户空间用copy_to_user()
	copy_from_user(&val,buf,count);//把buf拷贝到val的空间，长度为count
	
	//printk("firstdrv_write\n");

	switch(val)
		{
		case 1:s3c2410_gpio_setpin(S3C2410_GPF4,0);
			break;
		case 2:s3c2410_gpio_setpin(S3C2410_GPF5,0);
			break;
		case 3:s3c2410_gpio_setpin(S3C2410_GPF6,0);
			break;
		case 4:*gpfdat |= (1<<4)|(1<<5)|(1<<6);
			break;
		}
	
	return 0;
}
static struct file_operations first_drv_fops = {
	.owner = THIS_MODULE,
	.open = first_drv_open,
	.write = first_drv_write,
};

int major;
int first_drv_init(void)
{
	//写0让系统自动分配主设备号
	major = register_chrdev(0,"first_drv",&first_drv_fops);//注册，告诉内核
	
	firstdrv_class = class_create(THIS_MODULE,"firstdrv");
	
//	if(IS_ERR(firstdrv_class))
//		return PTR_ERR(firstdrv_class);
	firstdrv_class_dev = class_device_create(firstdrv_class,NULL,MKDEV(major,0),NULL,"xyz");
	
//	if(unlikely(IS_ERR(firstdrv_class_dev)))
//		return PTR_ERR(firstdrv_class_dev);
	//建立地址映射
	gpfcon = (volatile unsigned long *)ioremap(0x56000050,16);
	gpfdat = gpfcon + 1;
		

	return 0;
}

void first_drv_exit(void)
{
	unregister_chrdev(major,"first_drv");//卸载
	
	class_device_unregister(firstdrv_class_dev);
	class_destroy(firstdrv_class);
	//解除映射关系
	iounmap(gpfcon);
}

module_init(first_drv_init);

module_exit(first_drv_exit);

MODULE_LICENSE("GPL");


