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
 *
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

MODULE_AUTHOR("jefby");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Tiny 6410 buttons with interrupt");

//buttons irq�����ṹ��
struct buttons_irq_desc{
	int irq;//�жϺ�
	unsigned long flags;//�жϴ�����ʽ
	char *name;//����
};
//irq�������ĳ�ʼ��,����ָ�����õ��ⲿ�ж������Լ��жϴ�����ʽ������
static struct buttons_irq_desc buttons_irqs[] = {
	{IRQ_EINT(0),IRQ_TYPE_EDGE_RISING,"KEY1"},//KEY1
	{IRQ_EINT(1),IRQ_TYPE_EDGE_RISING,"KEY2"},//KEY2	
	{IRQ_EINT(2),IRQ_TYPE_EDGE_RISING,"KEY3"},//KEY3	
	{IRQ_EINT(3),IRQ_TYPE_EDGE_RISING,"KEY4"},//KEY4	
/*	{IRQ_EINT(4),IRQ_TYPE_EDGE_RISING,"KEY5"},//KEY4	
	{IRQ_EINT(5),IRQ_TYPE_EDGE_RISING,"KEY6"},//KEY4	
	{IRQ_EINT(19),IRQ_TYPE_EDGE_FALLING,"KEY7"},//KEY4	
	{IRQ_EINT(20),IRQ_TYPE_EDGE_FALLING,"KEY8"},//KEY4	
*/	
};

//����һ�������ĵȴ�����
static DECLARE_WAIT_QUEUE_HEAD(buttons_waitq);
//ָʾ�Ƿ��а���������
static volatile int ev_press = 0;
//�����豸�����豸��
static int buttons_major = 0;
//�豸��
dev_t dev;
//�ַ��豸
struct cdev * buttons_cdev;
//���´���
static volatile int press_cnt[]={0,0,0,0};

//�жϴ�����򣬼�¼�������µĴ��������ñ�־λΪ1�����ѵȴ������ϵȴ��Ľ���
static irqreturn_t buttons_interrupt(int irq,void *dev_id)
{
	volatile int *press_cnt = (volatile int *)dev_id;

	*press_cnt = *press_cnt + 1;//��������ֵ��1
	ev_press = 1;//���ñ�־λ
	wake_up_interruptible(&buttons_waitq);//���ѵȴ�����

	return IRQ_RETVAL(IRQ_HANDLED);
}
//�豸�򿪲�������Ҫ���BUTTONS����Ӧ��GPIO�ĳ�ʼ����ע���û��жϴ�����
int  buttons_open(struct inode *inode,struct file *filp)
{
	int i;
	int err;
	unsigned val;

	/*����buttons��Ӧ��GPIO�ܽ�*/
	val = ioread32(S3C64XX_GPNCON);
	val = (val & ~(0xFF)) | (0xaa);//����GPIO 0��5ΪExt interrupt[0~3]���
	iowrite32(val,S3C64XX_GPNCON);
/*
	val = ioread32(S3C64XX_GPLCON1);
	val = (val & ~(0xFF<<12)) | (0x33);
	iowrite32(val,S3C64XX_GPLCON1);
*/
	/*ע���жϴ�������buttons_interrupt*/
	for(i=0;i<sizeof(buttons_irqs)/sizeof(buttons_irqs[0]);++i){
		err = request_irq(buttons_irqs[i].irq,buttons_interrupt,buttons_irqs[i].flags,buttons_irqs[i].name,(void*)&press_cnt[i]);
		if(err)
			break;
	}
	if(err){
		printk("buttons_open functions err.\n");
		i--;
		for(;i>=0;--i)
			free_irq(buttons_irqs[i].irq,(void*)&press_cnt[i]);
		return -EBUSY;
	}
	return 0;
}
//��������û�м������£���ʹ�������ߣ����а��������£��򿽱����ݵ��û��ռ䣬Ȼ������;ʹ���������ķ���
int buttons_read(struct file *filp, char __user *buf, size_t len, loff_t * pos)
{
	unsigned long err;
	wait_event_interruptible(buttons_waitq,ev_press);//���ev_press==0,������ڶ���buttons_waitq���������ߣ�ֱ��ev_press==1
	ev_press = 0;//��ʱev_press==1,���ev_press
	err = copy_to_user(buf,(const void *)press_cnt,min(sizeof(press_cnt),len));//��press_cnt��ֵ�������û��ռ�
	memset((void*)press_cnt,0,sizeof(press_cnt));//初始化press_cnt为0
	return err ? -EFAULT : 0;

}
//��Ҫ��ж���û��жϴ������
int buttons_close(struct inode *inode,struct file *filp)
{
	int i;
	for(i=0;i<sizeof(buttons_irqs)/sizeof(buttons_irqs[0]);++i)
		free_irq(buttons_irqs[i].irq,(void*)&press_cnt);
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
	
	int result;
	printk("Tiny6410 buttons module init.\n");	
	if(buttons_major){
		dev = MKDEV(buttons_major,0);
		result = register_chrdev_region(dev,1,"buttons");//֪ͨʹ���豸��dev
	}else{
		result = alloc_chrdev_region(&dev,0,1,"buttons");//��ϵͳ�Զ������豸�ţ�Ĭ�Ϸ�ʽ
		buttons_major = MAJOR(dev);
	}
	if(result < 0){
		printk(KERN_WARNING "buttons : can't get major %d\n",buttons_major);
	}

	printk("buttons major is %d",buttons_major);
	buttons_cdev = cdev_alloc();//��̬����cdev�ṹ�壬���������Ҫʹ��kzmalloc�����ڴ���cdev�ṹ�壬����ʼ��kobject����
	buttons_cdev ->ops = &buttons_fops;//����fops
	cdev_init(buttons_cdev,&buttons_fops);//��ʼ��cdev�ṹ�壬��Ҫ�ǳ�ʼ��fops�ֶ�
	cdev_add(buttons_cdev,dev,1);//ע��ýṹ�嵽�ں�,��Ҫ���豸��,cdev�ṹ��ָ��,����
	printk("buttons add ok.\n");
	return 0;
}

static void module_buttons_exit(void)
{
	cdev_del(buttons_cdev);
	unregister_chrdev_region(dev,1);
	printk("Tiny6410 buttons module exit");
}

module_init(module_buttons_init);
module_exit(module_buttons_exit);
