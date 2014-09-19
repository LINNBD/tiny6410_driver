/*
	ƽ̨�豸����
	platform_driver
	��������:ʵ�ּ򵥵�LED�ƿ���

*/

#include <linux/init.h>
#include <linux/module.h>

#include <linux/platform_device.h>//platform_driver 
#include <linux/fs.h>//file_operations
#include <asm/io.h>

#include <linux/device.h>//class_create,device_create

static int major = 0;
static volatile unsigned long * gpio_con = NULL;
static volatile unsigned long * gpio_dat = NULL;
static int pin = -1;//��ʾ��Ҫ�����Ǹ�LED��
static struct class * cls = NULL;
static struct device * leds_dev = NULL;


static long led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	volatile unsigned long tmp  = 0;
	if(pin < 0)
			return -1;
	tmp = ioread32(gpio_dat);
	switch(cmd){
		case 0://�ر�LED
			tmp = (tmp & ~(0x1<<pin)) | (0x1<<pin);
			break;
		case 1://��LED
			tmp = tmp & ~(0x1<<pin);
			break;
		default:
			printk("cmd err.\n");
			return -1;
	}
	iowrite32(tmp,gpio_dat);//д������
	return 0;
}
//����GPK4,5,6,7Ϊ���
int led_open (struct inode *inode , struct file *filp)
{
	volatile unsigned long tmp	= 0;

	//����pin���ŷ���Ϊ���
	tmp=ioread32(gpio_con);
	tmp &= ~(0xF<<(pin*4));
	tmp |= 0x1<<(pin*4);
	iowrite32(tmp,gpio_con);

	return 0;
}

int led_close(struct inode *inode, struct file *filp)
{
	return 0;
}


static struct file_operations led_fops = {
	.open = led_open,
	.unlocked_ioctl = led_ioctl,
	.release = led_close,
};

//����/����/ע��һ��platform_drv

int led_platdrv_probe(struct platform_device *pdev)
{
	//����I/O����
	struct resource * res = NULL;
	res = platform_get_resource(pdev,IORESOURCE_MEM,0);//number��ʾ������Դ�ĵڼ���
	gpio_con = ioremap(res->start,res->end - res->start + 1);
	gpio_dat = gpio_con + 2;
	res = platform_get_resource(pdev,IORESOURCE_IRQ,0);
	pin = res->start;//�������ֵ
	if(pin < 0)
		return -1;

	major = register_chrdev(0,"platleds",&led_fops);//ע���ַ��豸
	if(major < 0){
		printk(KERN_ALERT "register_chrdev err.\n");
		return -1;
	}
	//�Զ������豸�ڵ�
	cls = class_create(THIS_MODULE,"platleds");
	if(!cls){
		printk("class_create err.\n");
		return -1;
	}
	leds_dev = device_create(cls, NULL,MKDEV(major,0),(void *)NULL,"platleds");//�����ڵ�/dev/platleds
	if(!leds_dev){
		class_destroy(cls);
		printk("device_create err.\n");
		return -1;	
	}
	
	printk("led_platdrv_probe .found led.\n");
	return 0;
}
int led_platdrv_remove(struct platform_device * pdev)
{
	printk("led_paltdrv_remove .remove led.\n");
	iounmap(gpio_con);
	unregister_chrdev(major,"platleds");
	device_destroy(cls,MKDEV(major,0));
	class_destroy(cls);
	return 0;
}


//ƽ̨����
static struct platform_driver led_platdriver= {
	.probe = led_platdrv_probe,
	.remove = led_platdrv_remove,
	.driver = {
		.name = "platdev_led",//Ҫƥ�������
	}
};

static int led_platdriver_init(void)
{
	platform_driver_register(&led_platdriver);
	printk(KERN_ALERT "led_platdrive_init.\n");
	return 0;
}

static void led_platdriver_exit(void)
{
	platform_driver_unregister(&led_platdriver);
	printk(KERN_ALERT "led_platdrive_exit.\n");
}

module_init(led_platdriver_init);
module_exit(led_platdriver_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jefby");



