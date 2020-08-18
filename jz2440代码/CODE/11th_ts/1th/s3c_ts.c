#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/clk.h>
#include <linux/serio.h>

#include <linux/irq.h>

#include <asm/io.h>
#include <asm/plat-s3c24xx/ts.h>
#include <asm/plat/regs-adc.h>
#include <asm/arch/regs-gpio.h>


static struct input_dev *s3c_ts_dev;


/*  所 有 触 摸 屏 都 是 这 个 框 架  */
static int s3c_ts_init(void)
{
	/* 1. 分配一个input_dev结构体*/
	s3c_ts_dev = input_allocate_device();
	/* 2. 设置 */
	
	/*2.1 能产生哪类事件 */
	set_bit(EV_KEY,s3c_ts_dev->evbit);//按键类事件
	set_bit(EV_ABS,s3c_ts_dev->evbit);//EV_ABS绝对位移事件(屏幕)
	/*2.2 能产生这类事件的哪些事件 */
	set_bit(BTN_TOUCH,s3c_ts_dev->keybit);//BTN_TOUCH(触摸屏按键)抽象出来的按键

	input_set_abs_params(s3c_ts_dev, ABS_X, 0, 0x3FF, 0, 0);//x
	input_set_abs_params(s3c_ts_dev, ABS_Y, 0, 0x3FF, 0, 0);//y
	input_set_abs_params(s3c_ts_dev, ABS_PRESSURE, 0, 1, 0, 0);//ABS_PRESSURE压力方向
	/* 3. 注册 */
	input_register_device(s3c_ts_dev);
	/* 4. 硬件相关的操作 */

	return 0;
}

static void s3c_ts_exit(void)
{

}


module_init(s3c_ts_init);
module_exit(s3c_ts_exit);
MODULE_LICENSE("GPL");


