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


static struct class *fifthdrv_class;
static struct class_device *fifthdrv_class_dev;

volatile unsigned long *gpfcon;
volatile unsigned long *gpfdat;

volatile unsigned long *gpgcon;
volatile unsigned long *gpgdat;

static DECLARE_WAIT_QUEUE_HEAD(button_waitq);
/*中 断 事 件 标 志 ，中 断 服 务 程 序 将 它 置 1 ，fifth_drv_read 将 它 清 0 */
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


/* 读 出 引 脚 值 *///buttons_irq中断处理程序
static irqreturn_t buttons_irq(int irq, void *dev_id)//中断号IRQ_EINT0，中断ID是&pins_desc[0]
{
	struct pin_desc * pindesc = (struct pins_desc *)dev_id;//定义结构体指针指向dev_id，(struct pins_desc *)是强制转换xx结构体类型
	unsigned int pinval;
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
	
	return IRQ_RETVAL(IRQ_HANDLED);
}



static int fifth_drv_open(struct inode *inode,struct file *file)
{
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

ssize_t fifth_drv_read(struct file *file, char __user *buf,size_t size, loff_t *ppos)
{

	if(size != 1)
		return -EINVAL;

	/* 如 果 没 有 按 键 动 作 ，休 眠 *///button_waitq队列
	wait_event_interruptible(button_waitq, ev_press);//如果ev_press=0，就休眠，否则直接往下运行(即唤醒才可继续执行)

	/* 如 果 有 按 键 动 作 ，返 回 键 值 */
	copy_to_user(buf,&key_val,1);
	ev_press = 0;
	return 1;
}

/*释放中断*/
int fifth_drv_close(struct inode *inode,struct file *file)
{
	free_irq(IRQ_EINT0,&pins_desc[0]);//释放中断
	free_irq(IRQ_EINT2,&pins_desc[1]);//释放中断
	free_irq(IRQ_EINT11,&pins_desc[2]);//释放中断
	free_irq(IRQ_EINT19,&pins_desc[3]);//释放中断
	return 0;
}

static int fifth_drv_fasync(int fd, struct file *filep, int on)
{

	/*fcntl(STDIN_FILENO, F_SETFL, flags)F_SETFL是设置flag,flags是要设置的flag,STDIN_FILENO设置的文件*/
	printk("driver: fifth_drv_fasync\n");

	/* fasync_helper 函 数 是 去 初 始 化 结 构 体 button_async */
	return fasync_helper(fd, filep, on, &button_async);
}



static struct file_operations fifth_drv_fops = {
	.owner = THIS_MODULE,
	.open = fifth_drv_open,
	.read = fifth_drv_read,
	.release = fifth_drv_close,
	.fasync = fifth_drv_fasync,
};

int major;
//入口函数注册
static int fifth_drv_init(void)
{
	//写0让系统自动分配主设备号
	major = register_chrdev(0,"fifth_drv",&fifth_drv_fops);//注册，告诉内核
	//创建一个类
	fifthdrv_class = class_create(THIS_MODULE,"fifth_drv");
	//在这个类下创建一个设备
	fifthdrv_class_dev = class_device_create(fifthdrv_class,NULL,MKDEV(major,0),NULL,"buttons");/* /dev/burrons */
	//建立地址映射
	gpfcon = (volatile unsigned long *)ioremap(0x56000050,16);
	gpfdat = gpfcon + 1;

	gpgcon = (volatile unsigned long *)ioremap(0x56000060,16);
	gpgdat = gpgcon + 1;

	return 0;
}


//出口函数，卸载
static int fifth_drv_exit(void)
{
	////写0让系统自动分配主设备号
	unregister_chrdev(major,"fifth_drv");//卸载
	class_device_unregister(fifthdrv_class_dev);//卸载设备
	class_destroy(fifthdrv_class);//卸载类
	//解除映射关系
	iounmap(gpfcon);
	iounmap(gpgcon);
	return 0;
}


//修饰使之成为出/入口函数
module_init(fifth_drv_init);
module_exit(fifth_drv_exit);
MODULE_LICENSE("GPL");





