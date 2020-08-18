#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/io.h>


/* 分配/设置/注册一个人platform_driver */

static int major;

static struct class *cls;
static volatile unsigned long *gpio_con;
static volatile unsigned long *gpio_dat;
static int pin;

static int led_open(struct inode *inode,struct file *file)
{
	//printk("first_drv_open\n");
	/*配置输出*/
	//清0
	*gpio_con &= ~(0x3<<(pin*2));
	//1为输出
	*gpio_con |= (0x1<<(pin*2));
	return 0;
}

static ssize_t led_write(struct file *file ,const char __user *buf, size_t count,loff_t * ppos)
{
	int val;
	//取用户程序传进来的值，用户空间到内核空间传递数据用copy_from_user()
	//内核空间传递到用户空间用copy_to_user()
	copy_from_user(&val,buf,count);//把buf拷贝到val的空间，长度为count
	
	//printk("firstdrv_write\n");

	if(val == 1)
		{
		//电灯
		*gpio_dat &= ~(1<<pin);
		}
	else
		{
		//灭灯
		*gpio_dat |= 1<<pin;
		}
	
	return 0;
}


static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,
};


static int  led_probe(struct platform_device *pdev)
{
	struct resource *res;

	/*根据platform_device 的资源进行ioremap建立地址映射*/
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);//获得资源IORESOURCE_MEM,第0个
	//建立地址映射
	gpio_con = ioremap(res->start, res->end - res->start + 1);
	gpio_dat = gpio_con + 1;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);//获得资源
	pin = res->start;
	
	/*注册字符设备驱动程序*/

	printk("led_probe , found led\n");
	/*注册一个字符设备*/
	major = register_chrdev(0,"myled",&led_fops);

	cls = class_create(THIS_MODULE,"myled");
	
	class_device_create(cls,NULL,MKDEV(major,0),NULL,"led");/* /dev/led */
	
	return 0;
}

static int  led_remove(struct platform_device *dev)
{
	/*根据platform_device 的资源进行iounmap建立地址映射*/
	/*卸载字符设备驱动程序*/
	/*iounmap*/
	
	printk("led_probe , remove led\n");

	class_device_destroy(cls,MKDEV(major,0));
	class_destroy(cls);
	unregister_chrdev(major,"myled");
	iounmap(gpio_con);
	return 0;

}



static struct platform_driver led_drv = {
	.probe  = led_probe,
	.remove = led_remove,
	.driver = {
	  .name = "myled",
	},
};



static int led_drv_init(void)
{
	platform_driver_register(&led_drv);
	return 0;
}

static void led_drv_exit(void)
{
	platform_driver_unregister(&led_drv);
}

module_init(led_drv_init);
module_exit(led_drv_exit);
MODULE_LICENSE("GPL");




