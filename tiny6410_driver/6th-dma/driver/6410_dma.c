#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>//file_operations
#include <linux/device.h>//class,device_create
#include <asm/string.h>//memset,memcmp
#include <linux/irq.h>//IRQ_TYPE_EDGE_BOTH
#include <asm/io.h>//ioremap
#include <linux/wait.h>//wait_event_interruptible
#include <linux/sched.h>//request_irq,free_irq
#include <linux/irq.h>//IRQ_TYPE_EDGE_FALLING
#include <linux/interrupt.h>//request_irq,free_irq


#define BUF_SIZE (512*1024)//���忽�������ֽ���512KB
#define TRANS_NO_DMA 0//��ʹ��DMA��������
#define TRANS_USE_DMA 1//ʹ��DMA��������

//S3C6410-DMAͨ������
struct s3c6410_dma_chan{
	unsigned long DMACCxSrcAddr;//Դ��ַ
	unsigned long DMACCxDestAddr;//Ŀ�ĵ�ַ
	unsigned long DMACCxLLI;//���ӵ������ַ
	unsigned long DMACCxControl0;//ͨ��������0
	unsigned long DMACCxControl1;//ͨ��������1
	unsigned long DMACCxConfiguration;//���üĴ���
	unsigned long DMACCxConfigurationExp;//���üĴ�����չ
};
//s3c6410 dmac�������ṹ����
struct s3c6410_dmac{
	unsigned long DMACIntStatus;
	unsigned long DMACIntTCStatus;
	unsigned long DMACIntTCClear;
	unsigned long DMACIntErrorStatus;
	unsigned long DMACIntErrClr;
	unsigned long DMACRawIntTCStatus;
	unsigned long DMACRawIntErrorStatus;
	unsigned long DMACEnbldChns;
	unsigned long DMACSoftBReq;
	unsigned long DMACSoftSReq;
	unsigned long reserved[2];//����
	unsigned long DMACConfiguration;
	unsigned long DMACSync;
};


static char *src=NULL;//Դ�ڴ�������ַ
static char *dst=NULL;//Ŀ���ڴ�������ַ
static u32 src_phys=0;//Դ�ڴ�������ַ
static u32 dst_phys=0;//Ŀ���ڴ�������ַ

static int major = 0;
static volatile struct s3c6410_dma_chan *dma_chan_p = NULL;
static volatile struct s3c6410_dmac *dmac_p = NULL;
static volatile unsigned long * sdma = NULL;

static struct class * cls = NULL;
static struct deivce * dev = NULL;

//���岢��ʼ���ȴ�����
static DECLARE_WAIT_QUEUE_HEAD(sdma_waitq);
static volatile int ev_dma = 0;


//DMA�жϴ�����
irqreturn_t sdma_irq(int irq, void *devid)
{
	ev_dma = 1;
	wake_up_interruptible(&sdma_waitq);//�������߶���
	return IRQ_HANDLED;
}


int dma6410_open (struct inode *, struct file *)
{
	
	return 0;
}
long dma6410_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	int i =0;
	memset(src,0xAA,BUF_SIZE);
	memset(dst,0x55,BUF_SIZE);
	switch(cmd)
	{
		case TRANS_NO_DMA:
			for(i=0;i<BUF_SIZE;++i)
				dst[i] = src[i];//����
			//��鿽���Ƿ���ȷ
			if(memcmp(src,dst,BUF_SIZE) == 0)
			{
				printk("TRANS_NO_DMA ok.");
			}else{
				printk("TRANS_NO_DMA err.\n");
			}
			break;
		case TRANS_USE_DMA://������ʽ:�ֹ�����
			//5.����Դ��ַ
			dma_chan_p->DMACCxSrcAddr = src_phys;//Դ��ַ
			//6.����Ŀ�ĵ�ַ
			dma_chan_p->DMACCxDestAddr = dst_phys;//Ŀ�������ַ
			//7.д����һ��LLI
			dma_chan_p->DMACCxLLI = 0;//����AHB master select for loading the next LLI:LM = 0 = AHB master 1
			//8.д������Ϣ
			dma_chan_p->DMACCxControl0 = (1 << 27) \ //Destination increment
										| (1 <<26) \//Source increment
										| (0 <<25) \//Destination AHB master select:0 = AHB master 1 (AXI_SYSTEM) selected for the destination transfer
										| (0<<24) \//Source AHB master select:0 = AHB master 1 (AXI_SYSTEM) selected for the source transfer
										| (2<<21) \//Destination transfer width,32bit
										| (2<<18) \//Source transfer width
										| (0<<15) \//Destination burst size
										| (0<<12) \//Source burst size
										;
			dma_chan_p->DMACCxControl1 = BUF_SIZE;//���ô�����ֽ���
			//9.дͨ��������Ϣ������ͨ��0
			dma_chan_p->DMACCxConfiguration = (0 << 18) \ //allow DMA request
											| (0 << 11) \ //Flow control and transfer type ; Memory to Memory
											| (1 << 0);//channel enabled
			//���DMA����δ����˯��
			wait_event_interruptible(&sdma_waitq,ev_dma);

			//DMACCxConfigurationExp;���üĴ�����չ������Mem-to-Mem����ʱ��������ã���Ϊ�������裬���Բ�������
			break;
		default:
			printk(KERN_ALERT "cmd error");
			return -EINVAL;
	}
	return 0;
}


static struct file_operations fops = {
	.open = dma6410_open,
	.unlocked_ioctl = dma6410_ioctl,
};


static int dma6410_init(void)
{
	int ret = 0;
	int chx = 0;
	//�ж�����
	ret = request_irq(IRQ_SDMA0, sdma_irq,IRQ_TYPE_EDGE_BOTH,"sdma-0",1);
	if(ret){
		printk(KERN_ERR "request_irq err.\n");
		return -1;
	}
	//��������ж�����ɹ�����ʹ��DMAC3��Ҳ����SDMAC0
	dmac_p = ioremap(0x7DB00000,0x200);//ӳ��SDMAC0�������ַ�������ַ��
	if(!dmac_p){
		printk("ioremap(0x7DB00000,0x200) err.\n");
		free_irq(IRQ_SDMA0,1);//�ͷ�������ж�
		return -1;
	}
	
	dmac_p->DMACConfiguration = 0x1 | 0x0<<1;//����SDMA������0
	
	//1.���ͨ��0�Ƿ����ڱ�ʹ��
	if(!dmac_p->DMACEnbldChns&0x1){
		dmac_p->DMACEnbldChns |= 0x1;//����ͨ��0
		dma_chan_p = dmac_p + 0x100;//����Ϊͨ��0����ʼ��ַ
	}
	//2.����ʹ��SDMAC����ͨ�õ�DMAC
	sdma = ioremap(0x7E00_F110,16);//SDMA_SEL ,����ʹ��SDMA
	iowrite32(0,sdma);
	iounmap(sdma);//���ӳ��
	
	//3.ʹ�ÿ���ͨ��0
	//4.����ڸ��ж��Ϲ�����ж�
	dmac_p->DMACIntErrClr |= 0x1;
	dmac_p->DMACIntTCClear |= 0x1;
	//5.ʣ�µĵ�ioctl�м�������
	//�����������ڴ�ռ�
	src = dma_alloc_writecombine(NULL,BUF_SIZE,&src_phys,GFP_KERNEL);
	if(!src){
		printk(KERN_ERR "dma_alloc_writecombine(NULL,BUF_SIZE,&src_phys,GFP_KERNEL) error.\n");
		return -ENOMEM;
	}
	dst = dma_alloc_writecombine(NULL,BUF_SIZE,&dst_phys,GFP_KERNEL);
	if(!dst){
		printk(KERN_ERR "dma_alloc_writecombine(NULL,BUF_SIZE,&src_phys,GFP_KERNEL) error.\n");
		dma_free_writecombine(NULL,BUF_SIZE,src,src_phys);//�ͷŵ���һ�������Դ�ڴ�
		return -ENOMEM;
	}	
	//ע���ַ��豸,�������豸��
    major = register_chrdev(0,"dma-6410",fops);
	if(ret){
		printk("register_chrdev err.\n");
		return -1;
	}
	//�Զ������豸
	cls = class_create(THIS_MODULE,"dma-6410");//������dma-6410,/sys/class/dma-6410
	if(!cls){
		printk(KERN_ERR "class_create(THIS_MODULE,"dma-6410") error.\n");
		return -ENOMEM;
	}
	device_create(cls,NULL,MKDEV(major,0),NULL,"dma-6410s");//�����豸�ڵ�/dev/dma-6410s


	printk(KERN_ALERT "6410dma init.\n");
	return 0;
}
static void  dma6410_exit(void)
{
	free_irq(IRQ_SDMA0,1);//�ͷ�������ж�
	iounmap(dmac_p);//���ӳ������
	dma_free_writecombine(NULL,BUF_SIZE,src,src_phys);
	dma_free_writecombine(NULL,BUF_SIZE,dst,dst_phys);

	device_destroy(cls,MKDEV(major,0));
	class_destroy(cls);
	unregister_chrdev(major,"dma-6410");
	printk(KERN_ALERT "6410dma exit.\n");
}

module_init(dma6410_init);
module_exit(dma6410_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jef199006@gmail.com jefby");














