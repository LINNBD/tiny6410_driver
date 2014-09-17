/*
	��USB��굱��������ʹ�ã�ʹ�䷢��L,S,��ENTER����ֵ
	��������ȫʵ����USB�����Ϊ�����¼��Ĺ���.
	
*/


#include <linux/module.h>
#include <linux/init.h>


#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/input.h>//input_dev

static struct input_dev * uk_dev = NULL;
static dma_addr_t uk_buf_phys ;//usb DMA�����������ַ
static char * uk_buf = NULL;//usb DMA�����������ַ
static int len = 0;
static struct urb * uk_urb= NULL;//usb�����


//usb��ɺ���
void usbmouse_as_key_irq(struct urb * urb)
{
#if 0
	int i=0 ;
	static int cnt = 0;
	printk("data cnt %d :",cnt++);
	for(i=0;i<len;++i){
		printk("%02x ",uk_buf[i]);
	}
	printk("\n");
#endif
//usb���ݵ�����
//uk_buf�ĵ�һ���ֽڵ�bit0 ��ʾ��� 0 �ɿ� 1 ����
//					  bit1 ��ʾ�Ҽ� 0 �ɿ� 1 ����
//					  bit2 ��ʾ�м� 0 �ɿ� 1 ����
	static volatile unsigned char prev_val;
	if((prev_val & 0x1) != (uk_buf[0]&0x1)){
		//��������˱仯
		input_event(uk_dev,EV_KEY, KEY_L, (uk_buf[0]&0x1) ? 1: 0);
		input_sync(uk_dev);
	}
	if((prev_val & (0x1<<1)) != (uk_buf[0]&0x2)){
		//�Ҽ������˱仯
		input_event(uk_dev,EV_KEY, KEY_S, (uk_buf[0]&0x2) ? 1: 0);
		input_sync(uk_dev);
	}
	if((prev_val & (0x1<<2)) != (uk_buf[0]&0x4)){
		//�м������˱仯
		input_event(uk_dev,EV_KEY, KEY_ENTER, (uk_buf[0]&0x4) ? 1: 0);
		input_sync(uk_dev);
	}
	prev_val = uk_buf[0];
	
	usb_submit_urb (urb, GFP_ATOMIC);//���ύ�� USB ���������͵�USB�豸
}

static int usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface = NULL;
	struct usb_endpoint_descriptor *endpoint = NULL;
	int pipe =0;
	
	interface = intf->cur_altsetting;
	
	if (interface->desc.bNumEndpoints != 1)
			return -ENODEV;
	
	endpoint = &interface->endpoint[0].desc;
	if (!usb_endpoint_is_int_in(endpoint))
			return -ENODEV;
	
	
	printk("found usbmouse.\n");
	printk("bcdUSB= %x\n",dev->descriptor.bcdUSB);
	printk("idProduct=%x\n",dev->descriptor.idProduct);
	printk("idVendor=%x\n",dev->descriptor.idVendor);

//1.����һ��input_device�ṹ��
	uk_dev = input_allocate_device();
	if(uk_dev == NULL){
		printk(KERN_ALERT "input_allocate_device err.\n");
		return -1;
	}
//2.����
	//2.1 �����ܲ����������¼�
	set_bit(EV_KEY,uk_dev->evbit);
	//2.2 �����ܲ�����Щ����
	set_bit(KEY_L,uk_dev->keybit);
	set_bit(KEY_S,uk_dev->keybit);
	set_bit(KEY_ENTER,uk_dev->keybit);

//3.ע��
	input_register_device(uk_dev);

//4.Ӳ����صĲ���
	//���ݴ������Ҫ��:Դ,Ŀ��,����
	//Դ-�豸�����ĳ���˵�
	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
	//����
	len = endpoint->wMaxPacketSize;
	//Ŀ��
	uk_buf = usb_alloc_coherent(dev, len, GFP_ATOMIC, &uk_buf_phys);
	//ʹ����Ҫ��,
	//(1)����һ��urb
	uk_urb = usb_alloc_urb(0,GFP_ATOMIC);
	//(2)����urb
	usb_fill_int_urb(uk_urb,dev,pipe,uk_buf,len,usbmouse_as_key_irq,NULL,endpoint->bInterval);
	uk_urb->transfer_dma = uk_buf_phys;
	uk_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	//(3)ʹ��urb
	usb_submit_urb(uk_urb, GFP_ATOMIC);
	
	return 0;
}


static void usb_mouse_disconnect(struct usb_interface *intf)
{
	struct usb_device *dev = interface_to_usbdev(intf);

	usb_kill_urb(uk_urb);
	usb_free_urb(uk_urb);
	
	usb_free_coherent(dev,len,uk_buf,uk_buf_phys);

	input_unregister_device(uk_dev);
	input_free_device(uk_dev);
	
	printk("usbmouse disconnect.\n");
}

/*����������֧�ֵ��豸�б�*/
static struct usb_device_id usb_mouse_id_table [] = {
	//USB_INTERFACE_INFO(class,subclass,protocol),����һ��struct usb_deivce_id�ṹ��,������USB�ӿڵ�ָ��������ƥ��.
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};

//usb����������Ҫ�ṹ��
static struct usb_driver usb_mouse_as_key_driver = {
	.name		= "usbmouse_as_key",
	.probe		= usb_mouse_probe,
	.disconnect	= usb_mouse_disconnect,
	.id_table	= usb_mouse_id_table,
};


static int usbmouse_as_key_init(void)
{
	usb_register(&usb_mouse_as_key_driver);
	return 0;
}

static void usbmouse_as_key_exit(void)
{
	usb_deregister(&usb_mouse_as_key_driver);
}

module_init(usbmouse_as_key_init);
module_exit(usbmouse_as_key_exit);

MODULE_LICENSE("Dual BSD/GPL");










