/*

	�USB�豸����������,����Ŀ����Ҫдһ��USB�豸��������,��һ��USB��굱��������ʹ��.�������"L",�һ�����"s",�м��������"enter"
	Author:jefby
	Email:jef199006@gmail.com

*/
#include <linux/module.h>
#include <linux/init.h>

#include <linux/usb/input.h>
#include <linux/hid.h>

static int usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);//�ҵ�USB�豸������
	
	printk("found usbmouse.\n");
	printk("manufacture is %s,produce id is %s.\n",dev->manufacturer,dev->product);//��ӡ��USB�豸����ID�Ͳ�ƷID��
	return 0;
}


static void usb_mouse_disconnect(struct usb_interface *intf)
{
	printk("usbmouse disconnect.\n");
}


static struct usb_device_id usb_mouse_id_table [] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};
//usb_driver�ṹ��
static struct usb_driver usb_mouse_as_key_driver = {
	.name		= "usbmouse_as_key",
	.probe		= usb_mouse_probe,
	.disconnect	= usb_mouse_disconnect,
	.id_table	= usb_mouse_id_table,
};

//��ں���
static int usbmouse_as_key_init(void)
{
	usb_register(&usb_mouse_as_key_driver);
	return 0;
}
//���ں���
static void usbmouse_as_key_exit(void)
{
	usb_deregister(&usb_mouse_as_key_driver);
}

module_init(usbmouse_as_key_init);
module_exit(usbmouse_as_key_exit);

MODULE_LICENSE("Dual BSD/GPL");










