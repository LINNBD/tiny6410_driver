/*
 *
 *	buttons��������TIny6410
 *	ʹ�ñ�׼�ַ��豸������д����
 *	make��ɺ���ݴ�ӡ���ն˵�����������ַ��豸
 *	��ʽ����:
 *	mknod /dev/buttons c ���豸�� 0
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
#include <linux/device.h>//class device

MODULE_AUTHOR("jefby");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Tiny 6410 buttons with interrupt");

struct button_irq_desc {
    int irq;
    int number;
    char *name;	
};

static struct button_irq_desc button_irqs [] = {
    {IRQ_EINT( 0), 0, "KEY0"},
    {IRQ_EINT( 1), 1, "KEY1"},
    {IRQ_EINT( 2), 2, "KEY2"},
    {IRQ_EINT( 3), 3, "KEY3"},
    {IRQ_EINT( 4), 4, "KEY4"},
    {IRQ_EINT( 5), 5, "KEY5"},
    {IRQ_EINT(19), 6, "KEY6"},
    {IRQ_EINT(20), 7, "KEY7"},
};

//����һ�������ĵȴ�����
static DECLARE_WAIT_QUEUE_HEAD(buttons_waitq);
//ָʾ�Ƿ��а��������£����жϴ����������Ϊ��read��������0
static volatile int ev_press = 0;
//�����豸�����豸��
static int buttons_major = 0;
//�豸��
dev_t dev;
//�ַ��豸
struct cdev * buttons_cdev=NULL;

static volatile unsigned char key_val = 0;
static struct class * tiny6410_buttons_class = NULL;
static struct device * tiny6410_buttons_device = NULL;

//�жϴ�����򣬼�¼�������µĴ��������ñ�־λΪ1�����ѵȴ������ϵȴ��Ľ���
static irqreturn_t buttons_interrupt(int irq,void *dev_id)
{	
	int cmp = 0;
	unsigned long rval = 0;
	struct button_irq_desc *temp = (struct button_irq_desc *)dev_id;
	cmp = temp->number;
	if(cmp >0 && cmp < 6 )
	{
		rval = readl(S3C64XX_GPNDAT);
		if(rval & (0x1<<cmp))//KEY1~KEY5�ɿ�
			key_val = cmp;
		else//KEY1~KEY5����
			key_val = cmp | 0x80;
	}else{
		rval = readl(S3C64XX_GPLDAT);
		if(rval & (0x1<<(cmp-6+11)))//KEY6~7�ɿ�
			key_val = cmp;
		else//KEY6~7����
			key_val = cmp | 0x80;
	}
		
	ev_press = 1;//��ʾ�жϷ�����
	wake_up_interruptible(&buttons_waitq);
	return IRQ_RETVAL(IRQ_HANDLED);
}

//�豸�򿪲�������Ҫ���BUTTONS����Ӧ��GPIO�ĳ�ʼ����ע���û��жϴ�����,���ô�����ʽΪ˫���ش���
int  buttons_open(struct inode *inode,struct file *filp)
{ 
    int i;
    int err = 0;
   
   //�����жϺ�
    for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++) {
	if (button_irqs[i].irq < 0) {
		continue;
	}
        err = request_irq(button_irqs[i].irq, buttons_interrupt, IRQ_TYPE_EDGE_BOTH, 
                          button_irqs[i].name, (void *)&button_irqs[i]);
        if (err)
            break;
    }
    if (err) {
	    printk("err=%d.\n",err);
        i--;
        for (; i >= 0; i--) {
	    if (button_irqs[i].irq < 0) {
		continue;
	    }
	    disable_irq(button_irqs[i].irq);
            free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
        }
        return -EBUSY;
    }

    ev_press = 0;//���ó�ʼ״̬Ϊ0
    
    return 0;
}
//��������û�м������£���ʹ�������ߣ����а��������£��򿽱����ݵ��û��ռ䣬Ȼ������
int buttons_read(struct file *filp, char __user *buf, size_t len, loff_t * pos)
{
	unsigned long val = 0;
	if(len != sizeof(key_val))
		return -EINVAL;
	wait_event_interruptible(buttons_waitq,ev_press);//ev_press==0,������
	ev_press = 0;//������0
	
	if((val = copy_to_user(buf,(const void*)&key_val,sizeof(key_val))) != sizeof(key_val))
		return -EINVAL;
	return sizeof(key_val);

}
//��Ҫ��ж���û��жϴ������
int buttons_close(struct inode *inode,struct file *filp)
{
	int i=0;
	printk("buttons close.\n");
	for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++) {
		disable_irq(button_irqs[i].irq);
		free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
	}
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
	int err=0;
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
	tiny6410_buttons_device = device_create(tiny6410_buttons_class, NULL, MKDEV(buttons_major, 0), NULL,"buttons");
	printk("buttons add ok.\n");
	return 0;
}

static void module_buttons_exit(void)
{

	device_destroy(tiny6410_buttons_class, MKDEV(buttons_major, 0));
	class_destroy(tiny6410_buttons_class);
	cdev_del(buttons_cdev);
	unregister_chrdev_region(dev,1);
	printk("Tiny6410 buttons module exit");
}

module_init(module_buttons_init);
module_exit(module_buttons_exit);
