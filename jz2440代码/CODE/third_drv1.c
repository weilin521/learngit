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


static struct class *thirddrv_class;
static struct class_device *thirddrv_class_dev;

volatile unsigned long *gpfcon;
volatile unsigned long *gpfdat;

volatile unsigned long *gpgcon;
volatile unsigned long *gpgdat;

static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	printk("irq = %d\n",irq);
	return IRQ_HANDLED;
}



static int third_drv_open(struct inode *inode,struct file *file)
{
	/* 配 置 GPF 0,2为 输 入 引 脚 */
	/*配 置 GPG 3,11为 输 入 引 脚*/
	/*request_irq引入中断*/
	/*int request_irq(unsigned int irq ,irq_handler_t handler ,unsigned long flags ,const char *devname ,void *dev_id)*/
	request_irq(IRQ_EINT0,buttons_irq,IRQT_BOTHEDGE,"s2",1);
	request_irq(IRQ_EINT2,buttons_irq,IRQT_BOTHEDGE,"s3",1);
	request_irq(IRQ_EINT11,buttons_irq,IRQT_BOTHEDGE,"s4",1);
	request_irq(IRQ_EINT19,buttons_irq,IRQT_BOTHEDGE,"s5",1);
	
	return 0;
}

ssize_t third_drv_read(struct file *file, char __user *buf,size_t size, loff_t *ppos)
{
	/* 返 回 4 个 引 脚 的 电 平 */
	unsigned char key_vals[4];
	int regval;
	//如果检测到的传进来的size字节不等于4的话返回错误值
	if(size != sizeof(key_vals))
		return -EINVAL;
	
	/* 读 GPF 0,2 */
	regval = *gpfdat;
	key_vals[0] = regval & (1<<0) ? 1 : 0 ;//如果GPF0为1，key_vals[0]=1,否则为0
	key_vals[1] = regval & (1<<2) ? 1 : 0 ;//如果GPF2为1，key_vals[1]=1,否则为0
	/* 读 GPG 3,11 */
	regval = *gpgdat;
	key_vals[2] = regval & (1<<3) ? 1 : 0 ;//如果GPG3为1，key_vals[2]=1,否则为0
	key_vals[3] = regval & (1<<11) ? 1 : 0 ;//如果GPG11为1，key_vals[3]=1,否则为0

	copy_to_user(buf,key_vals,sizeof(key_vals));
	
	return sizeof(key_vals);
}

/*释放中断*/
int third_drv_close(struct inode *inode,struct file *file)
{
	free_irq(IRQ_EINT0,1);//释放中断
	free_irq(IRQ_EINT2,1);//释放中断
	free_irq(IRQ_EINT11,1);//释放中断
	free_irq(IRQ_EINT19,1);//释放中断
	return 0;
}



static struct file_operations third_drv_fops = {
	.owner = THIS_MODULE,
	.open = third_drv_open,
	.read = third_drv_read,
	.release = third_drv_close,
};

int major;
//入口函数注册
static int third_drv_init(void)
{
	//写0让系统自动分配主设备号
	major = register_chrdev(0,"third_drv",&third_drv_fops);//注册，告诉内核
	//创建一个类
	thirddrv_class = class_create(THIS_MODULE,"third_drv");
	//在这个类下创建一个设备
	thirddrv_class_dev = class_device_create(thirddrv_class,NULL,MKDEV(major,0),NULL,"buttons");/* /dev/burrons */
	//建立地址映射
	gpfcon = (volatile unsigned long *)ioremap(0x56000050,16);
	gpfdat = gpfcon + 1;

	gpgcon = (volatile unsigned long *)ioremap(0x56000060,16);
	gpgdat = gpgcon + 1;

	return 0;
}


//出口函数，卸载
static int third_drv_exit(void)
{
	////写0让系统自动分配主设备号
	unregister_chrdev(major,"third_drv");//卸载
	class_device_unregister(thirddrv_class_dev);//卸载设备
	class_destroy(thirddrv_class);//卸载类
	//解除映射关系
	iounmap(gpfcon);
	iounmap(gpgcon);
	return 0;
}


//修饰使之成为出/入口函数
module_init(third_drv_init);
module_exit(third_drv_exit);
MODULE_LICENSE("GPL");





