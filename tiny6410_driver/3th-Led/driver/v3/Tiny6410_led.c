#include <linux/module.h>
#include <linux/init.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/io.h>

static int tiny6410_led_major = 0;//���豸��
static dev_t dev = 0;//�豸��
static int tiny6410_led_minor = 0;//���豸��
static int tiny6410_nr_devs = 1;//�豸����
static struct cdev * led_cdev = NULL;
static struct class * tiny6410_led_class = NULL;
static struct device * tiny6410_led_device = NULL;

static volatile unsigned long * gpkcon = NULL;
static volatile unsigned long * gpkdat = NULL;

void delay(volatile unsigned long n)
{
	while(n--)
		;
}
int led_open(struct inode * inode,struct file * file)
{
	volatile unsigned long temp ;
	printk(" led_open\n");
	temp = ioread32(gpkcon);
	temp = (temp & (~(0xFFFF<<16))) | (0x1111<<16);//configure LED to output
	iowrite32(temp,gpkcon);
	return 0;
}

int led_close(struct inode * node,struct file * filep)
{
	int local_led_minor = 0;
	int local_led_major =  imajor(node);
	local_led_minor = iminor(node);
	printk("open,led_major = %d,led_minor=%d\n",local_led_major,local_led_minor);
	
	return 0;	
}

ssize_t led_read (struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	volatile unsigned long temp;
	temp = ioread32(gpkcon);
	temp = (temp & (~(0xFF<<16))) ;//configure LED to output
	iowrite32(temp,gpkcon);
	temp = ioread32(gpkdat);
	temp = (temp & (0xF<<4))>>4;//get the 4~7
	printk("read temp = %lx.\n",temp);
	buf = copy_to_user((char*)buf,(char*)&temp,4);
	printk("led_read\n");
	return 0;
}

ssize_t led_write (struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	volatile unsigned long temp,r;
	copy_from_user((unsigned long*)&temp,buf,4);
	printk("write temp=%x",temp);
	//temp = 0x01;
	r=ioread32(gpkdat);
	r &= ~(0xF<<4);
	r |= (temp<<4);
	iowrite32(r,gpkdat);
	wmb();
	printk("led_write\n");
	return 0;

}

unsigned int led_poll (struct file *filp, struct poll_table_struct *pt)
{
	return 0;
}


long led_unlocked_ioctl (struct file *filp , unsigned int cmd, unsigned long arg)
{
	printk(" led_unlocked_ioctl \n");
	return 0;
}


//����file_operations�ṹ��
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_close,
	.read = led_read,
	.write = led_write,
	.poll = led_poll,
	.unlocked_ioctl = led_unlocked_ioctl
};
//�ѽṹ��file_operations�����ںˣ�ʹ���µĽӿ�cdev_add
//1.����cdev�ṹ
//2.��ʼ��cdev
//3.ע�ᵽ�ں�
static int Tiny6410_led_init(void)
{
	int result = 0;
	int err = 0;

	
	//�����豸���
	if(tiny6410_led_major){
		dev = MKDEV(tiny6410_led_major,tiny6410_led_minor);
		result = register_chrdev_region(dev,1,"6410led");//���һ�������豸���
	}else{
		result = alloc_chrdev_region(&dev,0,tiny6410_nr_devs,"6410led");//�����豸���
		tiny6410_led_major = MAJOR(dev);//��ȡ���豸��
	}
	if(result < 0){
		printk(KERN_WARNING "tiny6410_led : can't get major : %d\n",tiny6410_led_major);
		return result;
	}
	printk("init . major = %d.\n",	tiny6410_led_major );
	//��̬����cdev�ṹ��
	led_cdev = cdev_alloc();
	//��ʼ��cdev�ṹ
	cdev_init(led_cdev,&fops);
	//���cdev�ṹ���ں�
	err = cdev_add(led_cdev,dev,tiny6410_nr_devs);
	//ע�����
	if(err < 0)
	{
		printk(KERN_WARNING "tiny6410_led : can't add to kernel : %d\n",err);
		return err;
	}	
	//ע��ɹ�����ʹ��class�Զ������豸
	tiny6410_led_class = class_create(THIS_MODULE, "tiny6410led");
	if (IS_ERR(tiny6410_led_class)) {
		err = PTR_ERR(tiny6410_led_class);
		printk("create class error.\n");
	}
	tiny6410_led_device = device_create(tiny6410_led_class, NULL, MKDEV(tiny6410_led_major, 0), NULL,
			      "leds");

	gpkcon = (volatile unsigned long *)ioremap(0x7F008800,16);//��GPKCONӳ�䵽�����ַ�ռ�
	gpkdat = gpkcon + 2;

	return 0;
}

static void Tiny6410_led_exit(void)
{
	iounmap(gpkcon);//�������ӳ��
	device_destroy(tiny6410_led_class, MKDEV(tiny6410_led_major, 0));//�����豸
	class_destroy(tiny6410_led_class);//������
	cdev_del(led_cdev);//ɾ��cdev
	unregister_chrdev_region(dev,tiny6410_nr_devs);
}


module_init(Tiny6410_led_init);
module_exit(Tiny6410_led_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jefby");
MODULE_DESCRIPTION("Tiny6410 led driver frame low-level driver");


