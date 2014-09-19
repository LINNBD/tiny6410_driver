/*
 *
  *	buttons��������TIny6410
 *	ʹ�ñ�׼�ַ��豸������д����
 *	make��ɺ���ݴ�ӡ���ն˵�����������ַ��豸
 *	��ʽ����:
 *	mknod /dev/buttons c ���豸�� 0
 *	Ŀǰ����һЩ���⣬ֻ����������K1~K4,��������K5~K7,ԭ����request_irq��������ʧ��
 *	���⣬ʹ��ctrl+c�жϺ��ٴμ���������ʧ�ܣ�ԭ�����
 *	Author:jefby
 *	Email:jef199006@gmail.com
 *	K1,K2,K3,K4 => GPN0,1,2,3
 *	������ʹ�ò�ѯ�ķ�ʽ����ȡ����ֵ: CPU���ϵĶ�ȡ����״̬.
 */
#include <linux/module.h>//MODULE_LICENSE,MODULE_AUTHOR
#include <linux/init.h>//module_init/module_exit


#include <linux/fs.h>//file_operations
#include <asm/io.h>//ioread32,iowrite32
#include <linux/cdev.h>//cdev
#include <mach/map.h>//������S3C64XX_VA_GPIO
#include <mach/regs-gpio.h>//������gpio-bank-n��ʹ�õ�S3C64XX_GPN_BASE
#include <mach/gpio-bank-n.h>//������GPNCON
#include <mach/gpio-bank-l.h>//������GPNCON
#include <linux/wait.h>//wait_event_interruptible(wait_queue_head_t q,int condition);
//wake_up_interruptible(struct wait_queue **q)
#include <linux/sched.h>//request_irq,free_irq
#include <asm/uaccess.h>//copy_to_user
#include <linux/irq.h>//IRQ_TYPE_EDGE_FALLING
#include <linux/interrupt.h>//request_irq,free_irq
#include <linux/device.h>//class device
MODULE_AUTHOR("jefby");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Tiny 6410 buttons with search");

#define GPNCON 0x7F008830
#define GPLCON0 0x7F008810

static volatile unsigned int * gpncon = NULL;
static volatile unsigned int * gpndat = NULL;
static volatile unsigned int * gplcon = NULL;
static volatile unsigned int * gpldat = NULL;
//�����豸�����豸��
static int buttons_major = 0;
//�豸��
dev_t dev;
//�ַ��豸
struct cdev * buttons_cdev;

static struct class * tiny6410_buttons_class = NULL;
static struct device * tiny6410_buttons_device = NULL;


//�豸�򿪲�������Ҫ���BUTTONS����Ӧ��GPIO�ĳ�ʼ����ע���û��жϴ�����

int  buttons_open(struct inode *inode,struct file *filp)
{
	unsigned val;

	/*����buttons��Ӧ��GPIO�ܽ�,����KEY1~KEY6*/
	gpncon = (volatile unsigned int*)ioremap(GPNCON,16);
	gpndat = gpncon + 1;
	val = ioread32(gpncon);//��ȡGPNCON��ֵ
	val = (val & ~(0xFFF));//����GPIO 0��5Ϊ����
	iowrite32(val,gpncon);

	//����KEY7,KEY8Ϊ����,gpl11,gpl12
	gplcon = (volatile unsigned int*)ioremap(GPLCON0,16);
	gpldat = gplcon + 2;//gpldat
	val = ioread32(gplcon+1);//��ȡGPNCON1��ֵ
	val = (val & ~(0xFF<<12));//����GPL11��12Ϊ����
	iowrite32(val,gplcon+1);

/*
	val = ioread32(S3C64XX_GPLCON1);
	val = (val & ~(0xFF<<12)) | (0x33);
	iowrite32(val,S3C64XX_GPLCON1);
*/
	printk("buttons open.\n");
	return 0;
}
//��������û�м������£���ʹ�������ߣ����а��������£��򿽱����ݵ��û��ռ䣬Ȼ������
int buttons_read(struct file *filp, char __user *buf, size_t len, loff_t * pos)
{
	unsigned char keyval[8]={0};
	unsigned int temp=0;
	int i=0;
	if(len != 8)
		return -1;
	temp=ioread32(gpndat);
	//��ȡKEY1~KEY6��ֵ
	for(i=0;i<6;++i){
		keyval[i] = (temp&(0x1<<i))?1 : 0;
	}
	temp = ioread32(gpldat);
	//��ȡKEY7��KEY8��ֵ
	keyval[6]=(temp&(0x1<<11))?1:0;
	keyval[7]=(temp&(0x1<<12))?1:0;
	copy_to_user(buf,keyval,sizeof(keyval));

	return 0;

}
//��Ҫ��ж���û��жϴ������
int buttons_close(struct inode *inode,struct file *filp)
{
	printk("buttons close.\n");
	return 0;
}


static struct file_operations buttons_fops = {
	.owner = THIS_MODULE,
	.read = buttons_read,
	.release = buttons_close,
	.open = buttons_open,
};
/*
	ģ���ʼ����
		1.�����豸�ţ�Ĭ��ʹ�ö�̬����ķ���
		2.���벢��ʼ��cdev�ṹ
		3.��cdevע�ᵽ�ں�
*/
static int module_buttons_init(void)
{
	int err=0;
	int result=0;
	printk("Tiny6410 buttons module init.\n");	
	if(buttons_major){
		dev = MKDEV(buttons_major,0);
		result = register_chrdev_region(dev,1,"buttons");
	}else{
		result = alloc_chrdev_region(&dev,0,1,"buttons");
		buttons_major = MAJOR(dev);
	}
	if(result < 0){
		printk(KERN_WARNING "buttons : can't get major %d\n",buttons_major);
	}

	printk("buttons major is %d",buttons_major);
	buttons_cdev = cdev_alloc();
	buttons_cdev ->ops = &buttons_fops;
	cdev_init(buttons_cdev,&buttons_fops);
	cdev_add(buttons_cdev,dev,1);

	tiny6410_buttons_class = class_create(THIS_MODULE, "tiny6410buttons");
	if (IS_ERR(tiny6410_buttons_class)) {
		err = PTR_ERR(tiny6410_buttons_class);
		printk("create class error.\n");
	}
	tiny6410_buttons_device = device_create(tiny6410_buttons_class, NULL, MKDEV(buttons_major, 0), NULL,
			      "buttons");
	printk("buttons add ok.\n");
	return 0;
}

static void module_buttons_exit(void)
{
	iounmap(gpncon);
	device_destroy(tiny6410_buttons_class, MKDEV(buttons_major, 0));
	class_destroy(tiny6410_buttons_class);
	cdev_del(buttons_cdev);
	unregister_chrdev_region(dev,1);
	printk("Tiny6410 buttons module exit");
}

module_init(module_buttons_init);
module_exit(module_buttons_exit);
