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
#include <linux/gpio_keys.h>

#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>


struct pin_desc{
		int irq;
		char *name;
		unsigned int pin;
		unsigned int key_val;
};

/*定义四个按键*/
struct pin_desc pins_desc[4] = {
	{IRQ_EINT0,"s2",S3C2410_GPF0,KEY_L},
	{IRQ_EINT2,"s3",S3C2410_GPF2,KEY_S},
	{IRQ_EINT11,"s4",S3C2410_GPG3,KEY_ENTER},
	{IRQ_EINT19,"s5",S3C2410_GPG11,KEY_LEFTSHIFT},
};

/**/
static struct input_dev *buttons_dev;
static struct pin_desc *iqr_pd;//发生中断时的引脚描述
static struct timer_list buttons_timer;//定义结构体,定义一个定时器




/* 读 出 引 脚 值 *///buttons_irq中断处理程序
static irqreturn_t buttons_irq(int irq, void *dev_id)//中断号IRQ_EINT0，中断ID是&pins_desc[0]
{
	iqr_pd = (struct pin_desc *)dev_id;//把按键dev_id记录下来
	/*modify timer 修 改 定 时 器 的 超 时 时 间 , 基 于 jiffies 的 值 */
	/* 10ms 后 启 动 定 时 器 *///mod_timer是修改定时器的超时时间
	mod_timer(&buttons_timer,jiffies+HZ/100);//jiffies是全局变量，每隔10ms产生一个系统时钟中断，jiffies会累加,HZ是1秒，是100，即jiffies+HZ是jiffies一秒增加100,100个系统时钟之后
	return IRQ_RETVAL(IRQ_HANDLED);
}


static void buttons_timer_function(unsigned long data)
{
	struct pin_desc * pindesc = iqr_pd;//定义结构体指针指向dev_id，(struct pins_desc *)是强制转换xx结构体类型
	unsigned int pinval;
	
		if(!pindesc)
			return;
			
			/* 读 取 引 脚 值 ( 按 键 值 ) */
		pinval = s3c2410_gpio_getpin(pindesc->pin);//参数是引脚pindesc->pin
			/* 确 定 按 键 值 */
			if(pinval)
				{
				/* 松 开 : 最后一个参数 ：0-松开 1-按下*/
				input_event(buttons_dev,EV_KEY,pindesc->key_val,0);
				/*上报一个同步事件，表示这个事件已经上报完了*/
				input_sync(buttons_dev);
				}
			else
				{
				/* 按 下 */
				input_event(buttons_dev,EV_KEY,pindesc->key_val,1);
				input_sync(buttons_dev);
				}

}



static int buttons_init(void)
{
	int i;
	/*1、分配一个input_dev结构体*/
	buttons_dev = input_allocate_device();
	/* 2、设置 */
	/* 2.1 设置能产生哪类事件 */
	set_bit(EV_KEY,buttons_dev->evbit);
	/*设置能产生重复类事件*/
	set_bit(EV_REP,buttons_dev->evbit);
	/* 2.2 能产生这类操作里的哪类事件        L,S,ENTER,LEFTSHIT       *///L,S,回车，左边的shift按键
	set_bit(KEY_L,buttons_dev->keybit);
	set_bit(KEY_S,buttons_dev->keybit);
	set_bit(KEY_ENTER,buttons_dev->keybit);
	set_bit(KEY_LEFTSHIFT,buttons_dev->keybit);
	
	/*3、注册*/
	input_register_device(buttons_dev);
	/*4、硬 件 相 关 的 操 作 */
	/*初始化定时器,定时器是为了防抖动*/
	init_timer(&buttons_timer);//初始化buttons_timer结构体
	/*设置处理函数*/
	buttons_timer.function = buttons_timer_function;//处理函数,buttons_timer_function是一个函数，超时时间到后调用
	add_timer(&buttons_timer);//把定时器&buttons_timer告诉内核
	
	
	for(i = 0;i < 4;i++)
		{
		/*request_irq引入中断 , 注册中断*/
		request_irq(pins_desc[i].irq,buttons_irq,IRQT_BOTHEDGE,pins_desc[i].name,&pins_desc[i]);
		}
	
	return 0;

}


static void buttons_exit(void)
{
	int i;
	for(i = 0;i<4;i++)
		{
		free_irq(pins_desc[i].irq,&pins_desc[i]);
		}

	del_timer(&buttons_timer);
	input_unregister_device(buttons_dev);//卸载设备
	input_free_device(buttons_dev);//释放分配的空间
}



//修饰使之成为出/入口函数
module_init(buttons_init);
module_exit(buttons_exit);
MODULE_LICENSE("GPL");

