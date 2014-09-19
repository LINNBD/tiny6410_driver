#include <linux/init.h>
#include <linux/module.h>

#include <linux/platform_device.h>


//����/����/ע��һ��platform_device�豸
void	led_platdev_release(struct device *dev)
{
	printk(KERN_ALERT "led_platdev_release.\n");
}

//��Դ
static struct resource tiny6410_led_resource[] = {
	[0] = {
		.start	= 0x7F008800,//gpkcon����ʼ��ַ
		.end	= 0x7F008800 + 16 -1,//������ַ
		.flags	= IORESOURCE_MEM
	},
	[1] = {
		.start	= 5,//��ʾ��4λ
		.end	= 5,
		.flags	= IORESOURCE_IRQ
	}
};

//ƽ̨�豸
static struct platform_device led_platdevice = {
	.name		= "platdev_led",//����
	.id		= -1,
	.num_resources	= ARRAY_SIZE(tiny6410_led_resource),//��Դ����
	.resource	= tiny6410_led_resource,//��Դ
	.dev = {//struct device dev�ֶ�
		.release = led_platdev_release,//����release����ΪNULL
	}
};

static int led_platdevice_init(void)
{
	platform_device_register(&led_platdevice);
	printk(KERN_ALERT "led_platdevice_init.\n");
	return 0;
}

static void led_platdevice_exit(void)
{
	platform_device_unregister(&led_platdevice);
	printk(KERN_ALERT "led_platdevice_exit.\n");
}

module_init(led_platdevice_init);
module_exit(led_platdevice_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jefby");



