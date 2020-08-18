#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>


static struct class *sixthdrv_class;
static struct class_device *sixthdrv_class_dev;

volatile unsigned long *gpfcon;
volatile unsigned long *gpfdat;

volatile unsigned long *gpgcon;
volatile unsigned long *gpgdat;

static struct timer_list buttons_timer;//定义结构体,定义一个定时器


static DECLARE_WAIT_QUEUE_HEAD(button_waitq);
/*中 断 事 件 标 志 ，中 断 服 务 程 序 将 它 置 1 ，sixth_drv_read 将 它 清 0 */
static volatile int ev_press = 0;

static struct fasync_struct *button_async;


struct pin_desc{
		unsigned int pin;
		unsigned int key_val;
};

/*键 值：按 下 时 ，0x01 ,0x02 ,0x03 ,0x04 */
/*键 值：松 开 时 ，0x81 ,0x82 ,0x83 ,0x84 */

static unsigned char key_val;

struct pin_desc pins_desc[4] = {
	{S3C2410_GPF0,0x01},
	{S3C2410_GPF2,0x02},
	{S3C2410_GPG3,0x03},
	{S3C2410_GPG11,0x04},
};

static struct pin_desc *iqr_pd;//发生中断时的引脚描述

/* 原 子 操 作 指 的 是 在 执 行 过 程 中 不 会 被 别 的 代 码 路 径 所 中 断 的 操 作 。*/

//static atomic_t  cnaopen = ATOMIC_INIT(1);// 定 义 原 子 变 量 cnaopen 并 初 始 化 为 1


static DECLARE_MUTEX(button_lock);     //定义互斥锁



/* 读 出 引 脚 值 *///buttons_irq中断处理程序
static irqreturn_t buttons_irq(int irq, void *dev_id)//中断号IRQ_EINT0，中断ID是&pins_desc[0]
{
	iqr_pd = (struct pin_desc *)dev_id;
	/*modify timer 修 改 定 时 器 的 超 时 时 间 , 基 于 jiffies 的 值 */
	/* 10ms 后 启 动 定 时 器 */
	mod_timer(&buttons_timer,jiffies+HZ/100);//jiffies是全局变量，每隔10ms产生一个系统时钟中断，jiffies会累加,HZ是1秒，是100，即jiffies+HZ是jiffies一秒增加100,100个系统时钟之后
	return IRQ_RETVAL(IRQ_HANDLED);
}



static int sixth_drv_open(struct inode *inode,struct file *file)
{
#if 0
	/* 自 检 ，如 果 ==0 则 表 示 没 人 打 开 过 就 跳 过 */
	/* atomic_dec_and_test 自 减 操 作 后 测 试 其 是 否 为 0 ，为 0 则 返 回 true，否 则 返 回 false*/
	if( !atomic_dec_and_test(&cnaopen))
		{
		atomic_inc(&cnaopen);//原子变量增加1
		return -EBUSY;//如果打开了，进入if语句，最后返回EBUSY繁忙
		
		}
#endif

	if(file->f_flags & O_NONBLOCK)
		{
		//试图获取信号量，如果获取不到立刻返回，且不让休眠，要判断返回值
		if(down_trylock(&button_lock))
			return -EBUSY;
		}
	else
		{
		/* 获 取 信 号 量 */
		down(&button_lock);//第一次调用open可以获得信号量，第二次则无法获得，就会进入休眠
		}



	/* 配 置 GPF 0,2为 输 入 引 脚 */
	/*配 置 GPG 3,11为 输 入 引 脚*/
	/*request_irq引入中断*/
	/*int request_irq(unsigned int irq ,irq_handler_t handler ,unsigned long flags ,const char *devname ,void *dev_id)*/
	request_irq(IRQ_EINT0,buttons_irq,IRQT_BOTHEDGE,"s2",&pins_desc[0]);
	request_irq(IRQ_EINT2,buttons_irq,IRQT_BOTHEDGE,"s3",&pins_desc[1]);
	request_irq(IRQ_EINT11,buttons_irq,IRQT_BOTHEDGE,"s4",&pins_desc[2]);
	request_irq(IRQ_EINT19,buttons_irq,IRQT_BOTHEDGE,"s5",&pins_desc[3]);
	
	return 0;
}

ssize_t sixth_drv_read(struct file *file, char __user *buf,size_t size, loff_t *ppos)
{

	if(size != 1)
		return -EINVAL;

	/* 如 果 非 租 塞 则 */
	if(file->f_flags & O_NONBLOCK)
		{
		if(!ev_press)
			return -EAGAIN;//如果没有按键发送，再次来执行
		}
	else
		{/* 如 果 阻 塞 的 话 */
		/* 如 果 没 有 按 键 动 作 ，休 眠 *///button_waitq队列
		wait_event_interruptible(button_waitq, ev_press);//如果ev_press=0，就休眠，否则直接往下运行(即唤醒才可继续执行)
		}



	/* 如 果 有 按 键 动 作 ，返 回 键 值 */
	copy_to_user(buf,&key_val,1);
	ev_press = 0;
	return 1;
}

/*释放中断*/
int sixth_drv_close(struct inode *inode,struct file *file)
{
//	atomic_inc(&cnaopen);//关闭设备时+1，原子变量增加1
	free_irq(IRQ_EINT0,&pins_desc[0]);//释放中断
	free_irq(IRQ_EINT2,&pins_desc[1]);//释放中断
	free_irq(IRQ_EINT11,&pins_desc[2]);//释放中断
	free_irq(IRQ_EINT19,&pins_desc[3]);//释放中断
	up(&button_lock);//释放信号
	return 0;
}

static int sixth_drv_fasync(int fd, struct file *filep, int on)
{

	/*fcntl(STDIN_FILENO, F_SETFL, flags)F_SETFL是设置flag,flags是要设置的flag,STDIN_FILENO设置的文件*/
	printk("driver: sixth_drv_fasync\n");

	/* fasync_helper 函 数 是 去 初 始 化 结 构 体 button_async */
	return fasync_helper(fd, filep, on, &button_async);
}



static struct file_operations sixth_drv_fops = {
	.owner = THIS_MODULE,
	.open = sixth_drv_open,
	.read = sixth_drv_read,
	.release = sixth_drv_close,
	.fasync = sixth_drv_fasync,
};

int major;

/* 定 时 器 函 数 */
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
			/* 松 开 */
			key_val = 0x80 | pindesc->key_val;
		}
		else
			{
			/* 按 下 */
			key_val = pindesc->key_val;
			}
		ev_press = 1;//    表 示 中 断 发 生 了
		wake_up_interruptible(&button_waitq);// 唤 醒 休 眠 的 进 程
		
		/* kill_fasync 函 数 发 送 信 号 */
		kill_fasync(&button_async, SIGIO, POLL_IN);//button_async包含有进程id，SIGIO发给谁，POLL_IN发什么表示有数据等待读取

}


//入口函数注册
static int sixth_drv_init(void)
{
	/*初始化定时器*/
	init_timer(&buttons_timer);//初始化buttons_timer结构体
	/*设置处理函数*/
	buttons_timer.function = buttons_timer_function;//处理函数,buttons_timer_function是一个函数，超时时间到后调用
	//buttons_timer.expires = 0;//超时时钟
	add_timer(&buttons_timer);//把定时器&buttons_timer告诉内核
	

	//写0让系统自动分配主设备号
	major = register_chrdev(0,"sixth_drv",&sixth_drv_fops);//注册，告诉内核
	//创建一个类
	sixthdrv_class = class_create(THIS_MODULE,"sixth_drv");
	//在这个类下创建一个设备
	sixthdrv_class_dev = class_device_create(sixthdrv_class,NULL,MKDEV(major,0),NULL,"buttons");/* /dev/burrons */
	//建立地址映射
	gpfcon = (volatile unsigned long *)ioremap(0x56000050,16);
	gpfdat = gpfcon + 1;

	gpgcon = (volatile unsigned long *)ioremap(0x56000060,16);
	gpgdat = gpgcon + 1;

	return 0;
}


//出口函数，卸载
static int sixth_drv_exit(void)
{
	////写0让系统自动分配主设备号
	unregister_chrdev(major,"sixth_drv");//卸载
	class_device_unregister(sixthdrv_class_dev);//卸载设备
	class_destroy(sixthdrv_class);//卸载类
	//解除映射关系
	iounmap(gpfcon);
	iounmap(gpgcon);
	return 0;
}


//修饰使之成为出/入口函数
module_init(sixth_drv_init);
module_exit(sixth_drv_exit);
MODULE_LICENSE("GPL");





