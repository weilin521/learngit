#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>



static struct input_dev *uk_dev;

static char *usb_buf;

static dma_addr_t usb_buf_phys;//dma_addr_t物理地址的意思

static int len;//长度：wMaxPacketSize最大包大小

static struct urb *uk_urb;//urb是usb请求块


/*定义一个id_table，用宏USB_INTERFACE_INFO来填充*/
static struct usb_device_id usbmouse_as_key_id_table [] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
		USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};


static void usbmouse_as_key_irq(struct urb *urb)
{

	static unsigned char pre_val;

#if 0
	int i;
	static int cnt = 0;
	printk("data cnt %d: ",++cnt);
	for(i = 0;i<len;i++)
		{
		printk("%02x ",usb_buf[i]);
		}
	printk("\n");
#endif

	/*USB鼠标数据含义
	*data[0]：bit0-左键，1-按下，0-松开
	*data[1]：bit1-右键，1-按下，0-松开
	*data[2]：bit2-中键，1-按下，0-松开
	*
	*usb_buf[0]：按键值
	*usb_buf[1]：鼠标X移动值(0x01/0xff)
	*usb_buf[2]：鼠标Y移动值(0x01/0xff)
	*/

	if((pre_val & (1<<0)) != (usb_buf[0] & (1<<0)))
		{
		/*左键发生了变化 *///上报事件
		input_event(uk_dev,EV_KEY,KEY_L,(usb_buf[0] & (1<<0)) ? 1 : 0);
		input_sync(uk_dev);
		}

	if((pre_val & (1<<1)) != (usb_buf[0] & (1<<1)))
		{
		/*右键发生了变化 *///上报事件
		input_event(uk_dev,EV_KEY,KEY_S,(usb_buf[0] & (1<<1)) ? 1 : 0);
		input_sync(uk_dev);
		}

	if((pre_val & (1<<2)) != (usb_buf[0] & (1<<2)))
		{
		/*中键发生了变化 *///上报事件
		input_event(uk_dev,EV_KEY,KEY_ENTER,(usb_buf[0] & (1<<2)) ? 1 : 0);
		input_sync(uk_dev);
		}

		pre_val = usb_buf[0];
	
	/* 重新提交urb */
	usb_submit_urb(uk_urb, GFP_KERNEL);//提交urb
}


static int usbmouse_as_key_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;//端点描述符
	int pipe;//源
	

	interface = intf->cur_altsetting;

	endpoint = &interface->endpoint[0].desc;//endpoint(端点，末端，终端)端点描述符

	/*a. 分配一个input_dev结构体*/
	uk_dev = input_allocate_device();
	
	/*b. 设置 */
	/*b1. 能产生哪类事件 */
	set_bit(EV_KEY,uk_dev->evbit);
	set_bit(EV_REP,uk_dev->evbit);
	
	/*b2. 能产生哪些事件 */
	/* 能产生这类操作里的哪类事件        L,S,ENTER,LEFTSHIT       *///L,S,回车，左边的shift按键
	set_bit(KEY_L,uk_dev->keybit);
	set_bit(KEY_S,uk_dev->keybit);
	set_bit(KEY_ENTER,uk_dev->keybit);
	
	/*c. 注册 */
	input_register_device(uk_dev);
	/*d. 硬件相关操作 */
	/* 数据传输3要素：源，目的，长度 */
	/* 源：USB设备的某个端点 *///bEndpointAddress端点地址
	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
	/* 长度： */
	len = endpoint->wMaxPacketSize;//wMaxPacketSize最大包大小
	/* 目的： */
	usb_buf = usb_buffer_alloc(dev, len, GFP_ATOMIC,&usb_buf_phys);

	/* 使用3要素 */
	/* 分配 USB request block (USB请求块)*/
	uk_urb = usb_alloc_urb(0, GFP_KERNEL);
	/* 使用3要素设置urb *///usb_fill_int_urb填充中断类型的urb，bInterval是间隔(查询间隔间隙，查询频率)
	usb_fill_int_urb(uk_urb, dev, pipe, usb_buf,len,usbmouse_as_key_irq, NULL, endpoint->bInterval);//usbmouse_as_key_irq是完成函数
	uk_urb->transfer_dma = usb_buf_phys;//告诉物理地址
	uk_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;//设置某些标志

	/* 使用urb */
	usb_submit_urb(uk_urb, GFP_KERNEL);//提交urb
	
	return 0;
}


static void usbmouse_as_key_disconnect(struct usb_interface *intf)
{
	struct usb_device *dev = interface_to_usbdev(intf);//从这个接口可以到usb device 
	//printk("disconnect usbmouse! \n");
	usb_kill_urb(uk_urb);//杀掉urb
	usb_free_urb(uk_urb);//释放urb
	usb_buffer_free(dev,len,usb_buf,usb_buf_phys);//释放buf
	input_unregister_device(uk_dev);
	input_free_device(uk_dev);
}




/* 1. 分配/设置usb_driver 结构体 */
static struct usb_driver usbmouse_as_key_driver = {
	.name		= "usbmouse_as_key",
	.probe		= usbmouse_as_key_probe,
	.disconnect	= usbmouse_as_key_disconnect,
	.id_table	= usbmouse_as_key_id_table,
};



static int usbmouse_as_key_init(void)
{
	/* 2. 注册 */
	usb_register(&usbmouse_as_key_driver);
	return 0;
}


static void usbmouse_as_key_exit(void)
{
	/* 2. 卸载 */
	usb_deregister(&usbmouse_as_key_driver);
}


module_init(usbmouse_as_key_init);
module_exit(usbmouse_as_key_exit);
MODULE_LICENSE("GPL");


