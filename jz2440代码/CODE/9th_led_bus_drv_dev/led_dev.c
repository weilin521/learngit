#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>



/* 分配/设置/注册一个人platform_device */

//[0]中资源start是开始地址，end结束地址，flags表示哪类资源，
//[1]中start和end是哪一个引脚，
static struct resource led_resource[] = {
	[0] = {
		.start = 0x56000050,
		.end   = 0x56000050 + 8 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = 4,
		.end   = 4,
		.flags = IORESOURCE_IRQ,
	}
};

static void led_release(struct device * dev)
{

}



//定义一个平台设备
static struct platform_device led_dev = {
	.name		  = "myled",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(led_resource),
	.resource	  = led_resource,
	.dev =
	{
		.release = led_release,
	},
};

static int led_dev_init(void)
{
	/*注册一个平台设备*/
	platform_device_register(&led_dev);
	return 0;
}


static void led_dev_exit(void)
{
	/*卸载一个平台设备*/
	platform_device_unregister(&led_dev);
}

module_init(led_dev_init);
module_exit(led_dev_exit);
MODULE_LICENSE("GPL");





