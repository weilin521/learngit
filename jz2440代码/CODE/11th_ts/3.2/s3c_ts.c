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
#include <asm/arch/regs-adc.h>
#include <asm/arch/regs-gpio.h>

struct s3c_ts_regs {
	unsigned long adccon;//初始化引脚
	unsigned long adctsc;//进入按下或松开模式
	unsigned long adcdly;
	unsigned long adcdat0;//判断触摸笔是按下还是松开
	unsigned long adcdat1;
	unsigned long adcupdn;
};

static struct input_dev *s3c_ts_dev;

static volatile struct s3c_ts_regs *s3c_ts_regs;
/*等待触摸笔按下*/
void enter_wait_pen_down_mode(void)
{
	/*把rADCTSC寄存器等于0xd3就可以了*/
	s3c_ts_regs->adctsc = 0xd3;
}
/*等待触摸笔松开*/
void enter_wait_pen_up_mode(void)
{
	/*把rADCTSC寄存器等于0xd3就可以了*/
	s3c_ts_regs->adctsc = 0x1d3;
}

static void enter_measure_xy_mode(void)
{
	s3c_ts_regs->adctsc = (1<<2)|(1<<3);
}

static void start_adc(void)
{
	s3c_ts_regs->adccon |= (1<<0);//启动ADC
}

static irqreturn_t pen_down_up_irq(int irq, void *dev_id)
{
	if(s3c_ts_regs->adcdat0 & (1<<15))
		{
		
		printk("pen up\n");//如果为1，松开
		enter_wait_pen_down_mode();//松开了就，进入等待按下模式
		}
	else
		{
//		printk("pen down\n");//如果为0，按下
//		enter_wait_pen_up_mode();//进入等待松开模式
		enter_measure_xy_mode();//进入测量xy坐标模式
		start_adc();//启动ADC,启动完成后会产生一个中断
		}
	return IRQ_HANDLED;
}

static irqreturn_t adc_irq(int irq, void *dev_id)
{
	static int cnt = 0;
	int adcdat0,adcdat1;
	static int x[4],y[4];
	
	/*优化措施2：如果ADC完成时，发现触摸笔以及松开，则丢弃此次结果 */
	adcdat0 = s3c_ts_regs->adcdat0;
	adcdat1 = s3c_ts_regs->adcdat1;
	if(s3c_ts_regs->adcdat0 & (1<<15))
		{
			/*如果已经松开*/
		cnt = 0;//松开后把cnt清零
		enter_wait_pen_down_mode();//松开了就，进入等待按下模式
		}
	else
		{
		//printk("adc_irq cnt = %d, x = %d,y = %d\n",++cnt,s3c_ts_regs->adcdat0 & 0x3ff,s3c_ts_regs->adcdat1 & 0x3ff);
		/*优化措施3：多次测量求平均值*/
		x[cnt] = adcdat0 & 0x3ff;//把读取到的值记录下来
		y[cnt] = adcdat1 & 0x3ff;
		++cnt;//测量一次加一
		if(cnt == 4)//测量四次打印一次
			{
			printk("x = %d,y = %d\n",(x[0]+x[1]+x[2]+x[3])/4,(y[0]+y[1]+y[2]+y[3])/4);
			cnt = 0;
			enter_wait_pen_up_mode();//进入等待松开模式
			}
		else
			{
			/*否则在测量一次*/
			enter_measure_xy_mode();//进入测量xy坐标模式
			start_adc();//启动ADC,启动完成后会产生一个中断
			}
		
		}
	
	
	return IRQ_HANDLED;

}

/*  所 有 触 摸 屏 都 是 这 个 框 架  */
static int s3c_ts_init(void)
{
	struct clk *clk;
	
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
	/* 4.1 使能时钟(CLKCON[15]) ,把这一位设置为1 */
	clk = clk_get(NULL, "adc");/*开启CLKCON寄存器时钟*/
	clk_enable(clk);
	/* 4.2 设置S3C2440的ADC/TS寄存器*/
	s3c_ts_regs = ioremap(0x58000000,sizeof(struct s3c_ts_regs));

	/*bit[14]   : 1-A/D converter prescaler enable(预分频使能,置1)
	 *bit[13:6] : A/D converter prescaler value,(预分频系数)
	 *            49，ADCCLK=PCLK/(49+1)=50MHz/(49+1)=1MHz
	 *bit[0] : A/D conversion starts by enable. 先设为0
	*/
	
	s3c_ts_regs->adccon = (1<<14)|(49<<6);//初始化

	/*注册中断*/
	request_irq(IRQ_TC,pen_down_up_irq,IRQF_SAMPLE_RANDOM,"ts_pen",NULL);
	request_irq(IRQ_ADC,adc_irq,IRQF_SAMPLE_RANDOM,"adc",NULL);

	/*优化措施1：
	*设置ADCDLY为最大值，这使得电压稳定后在发出IRQ_TC中断
	*
	*/
	
	s3c_ts_regs->adcdly = 0xffff;//设置延时值，等待按下电压稳定
	
	/*Waiting for Interrupt Mode等待中断模式*/
	enter_wait_pen_down_mode();//进入等待按下模式
	
	return 0;
}

static void s3c_ts_exit(void)
{
	free_irq(IRQ_TC,NULL);//释放中断
	free_irq(IRQ_ADC,NULL);//释放中断
	iounmap(s3c_ts_regs);//解除映射关系
	input_unregister_device(s3c_ts_dev);//卸载设备
	input_free_device(s3c_ts_dev);//释放分配的结构体空间s3c_ts_dev
}


module_init(s3c_ts_init);
module_exit(s3c_ts_exit);
MODULE_LICENSE("GPL");


