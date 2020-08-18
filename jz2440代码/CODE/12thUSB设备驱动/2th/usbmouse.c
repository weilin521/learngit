#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>




/*定义一个id_table，用宏USB_INTERFACE_INFO来填充*/
static struct usb_device_id usbmouse_as_key_id_table [] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
		USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};


static int usbmouse_as_key_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);

	printk("found usbmouse! \n");

	printk("bcdUSB = %x\n",dev->descriptor.bcdUSB);//bcdUSB是版本号

	printk("VID = 0x%x\n",dev->descriptor.idVendor);//

	printk("PID = 0x%x\n",dev->descriptor.iProduct);//
	return 0;
}


static void usbmouse_as_key_disconnect(struct usb_interface *intf)
{
	printk("disconnect usbmouse! \n");
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


