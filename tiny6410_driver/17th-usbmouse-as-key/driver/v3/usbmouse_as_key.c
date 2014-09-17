/*
	��USB��굱��������ʹ�ã�ʹ�䷢��L,S,��ENTER����ֵ
	��û��ʵ��,ֻ�ǽ������¼����ݴ�ӡ����
	һ�����ݸ�ʽ:
	��һ���ֽ� 
	0λ��ʾ�������Ƿ񱻰���,0��ʾδ����,1��ʾ����
	1λ��ʾ����Ҽ��Ƿ񱻰���,0��ʾδ����,1��ʾ����
	2λ��ʾ����м��Ƿ񱻰���,0��ʾδ����,1��ʾ����
*/


#include <linux/module.h>
#include <linux/init.h>


#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/input.h>//input_dev
#include <linux/usb.h>

static struct input_dev * uk_dev = NULL;
static dma_addr_t uk_buf_phys ;//�����usb�����������ַ
static char * uk_buf = NULL;//�����usb�����������ַ
static int len = 0;
static struct urb * uk_urb= NULL;//usb�����


//usb��ɺ���
void usbmouse_as_key_irq(struct urb * urb)
{
	int i=0 ;
	static int cnt = 0;
	printk("data cnt %d :",cnt++);
	for(i=0;i<len;++i){
		printk("%02x ",uk_buf[i]);
	}
	printk("\n");
	usb_submit_urb (urb, GFP_ATOMIC);
}

static int usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);//��ȡUSB�豸������ָ��
	struct usb_host_interface *interface = NULL;
	struct usb_endpoint_descriptor *endpoint = NULL;//usb�˿�������
	int pipe =0;
	
	interface = intf->cur_altsetting;
	
	if (interface->desc.bNumEndpoints != 1)
			return -ENODEV;
	
	endpoint = &interface->endpoint[0].desc;//�˵�0���ڿ��ƴ������ͺʹ��䷽��
	if (!usb_endpoint_is_int_in(endpoint))//��������жϴ����ҷ�����IN,���ش���
			return -ENODEV;
	
	
	printk("found usbmouse.\n");
	printk("bcdUSB= %x\n",dev->descriptor.bcdUSB);
	printk("idProduct=%x\n",dev->descriptor.idProduct);
	printk("idVendor=%x\n",dev->descriptor.idVendor);

//1.����һ��input_device�ṹ��
	uk_dev = input_allocate_device();//���䲢��ʼ��һ�������豸
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

//3.ע���豸��������ϵͳ��
	input_register_device(uk_dev);

//4.Ӳ����صĲ���
	//���ݴ������Ҫ��:Դ,Ŀ��,����
	//Դ-�豸�����ĳ���˵�
	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);//�˵�ͷ���
	//����
	len = endpoint->wMaxPacketSize;
	//Ŀ��:������ջ�����
	uk_buf = usb_alloc_coherent(dev, len, GFP_ATOMIC, &uk_buf_phys);
	//ʹ����Ҫ��,urb(usb request block)
	//(1)����һ��urb
	uk_urb = usb_alloc_urb(0,GFP_KERNEL);
	//(2)����urb
	usb_fill_int_urb(uk_urb,dev,pipe,uk_buf,len,usbmouse_as_key_irq,NULL,endpoint->bInterval);
	uk_urb->transfer_dma = uk_buf_phys;//Ҫ���͵�DMA��������ַ
	uk_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP; //urb->transfer_dma valid on submit,Ӧ������λ, �� urb ����һ��Ҫ�����͵� DMA ����. USB ����ʹ������� transfer_dma ����ָ��Ļ���, ���Ǳ� transfer_buffer ����ָ��Ļ���.

	//(3)ʹ��urb
	usb_submit_urb(uk_urb, GFP_ATOMIC);//���ύ�� USB ���������ͳ��� USB �豸
	
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


static struct usb_device_id usb_mouse_id_table [] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};

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










