
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

static struct class *seconddrv_class;
static struct class_device *seconddrv_class_dev;

volatile unsigned long *gpfcon;
volatile unsigned long *gpfdat;

volatile unsigned long *gpgcon;
volatile unsigned long *gpgdat;


static int second_drv_open(struct inode *inode,struct file *file)
{
	/* 配 置 GPF 0,2为 输 入 引 脚 */
	*gpfcon &=~((0x3<<(0*2)) | (0x3<<(2*2)));
	/*配 置 GPG 3,11为 输 入 引 脚*/
	*gpgcon &=~((0x3<<(3*2))|(0x3<<(11*2)));
	return 0;
}

ssize_t second_drv_read(struct file *file, char __user *buf,size_t size, loff_t *ppos)
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


static struct file_operations second_drv_fops = {
	.owner = THIS_MODULE,
	.open = second_drv_open,
	.read = second_drv_read,
};

int major;
//入口函数注册
static int second_drv_init(void)
{
	//写0让系统自动分配主设备号
	major = register_chrdev(0,"second_drv",&second_drv_fops);//注册，告诉内核
	//创建一个类
	seconddrv_class = class_create(THIS_MODULE,"second_drv");
	//在这个类下创建一个设备
	seconddrv_class_dev = class_device_create(seconddrv_class,NULL,MKDEV(major,0),NULL,"buttons");/* /dev/burrons */
	//建立地址映射
	gpfcon = (volatile unsigned long *)ioremap(0x56000050,16);
	gpfdat = gpfcon + 1;

	gpgcon = (volatile unsigned long *)ioremap(0x56000060,16);
	gpgdat = gpgcon + 1;

	return 0;
}


//出口函数，卸载
static int second_drv_exit(void)
{
	////写0让系统自动分配主设备号
	unregister_chrdev(major,"second_drv");//卸载
	class_device_unregister(seconddrv_class_dev);//卸载设备
	class_destroy(seconddrv_class);//卸载类
	//解除映射关系
	iounmap(gpfcon);
	iounmap(gpgcon);
	return 0;
}


//修饰使之成为出/入口函数
module_init(second_drv_init);
module_exit(second_drv_exit);
MODULE_LICENSE("GPL");



