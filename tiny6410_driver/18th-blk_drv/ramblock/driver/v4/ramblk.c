#include <linux/init.h>//module_init/exit
#include <linux/module.h>//MODULE_AUTHOR,MODULE_LICENSE��
#include <linux/genhd.h>//alloc_disk
#include <linux/blkdev.h>//blk_init_queue
#include <linux/fs.h>//register_blkdev,unregister_blkdev
#include <linux/types.h>//u_char,u_short
#include <linux/vmalloc.h>
#include <linux/hdreg.h>
//1MB��С�ռ�
#define RAMBLK_SIZE (1024*1024*32)
#define RAM_CHUNKSIZE		(0x00001000)
#define RAM_CHUNKMASK		(0x00000fff)
#define RAM_CHUNKSHIFT	(12)

/*
//��������˽�����ݽṹ
struct ramblk_info{
	u_char heads;//��ͷ��
	u_short cylinders;//������
	u_char sectors;//������
	u_char control;
	int unit;
};
*/

static struct gendisk * ramblk_disk = NULL;
static struct request_queue * ramblk_request_queue = NULL;
static int major = 0;//���豸�����豸��
//static struct ramblk_info *pinfo = NULL;
static DEFINE_SPINLOCK(ramblk_spinlock);//���岢��ʼ��һ��������
static char * ramblk_buf = NULL;//������ڴ���ʼ��ַ

//ramdisk_info��ʼ������
/*
static int ramblk_info_init(struct ramblk_info *p)
{
	if(!p){
		printk("ramblk_info_init p==NULL.\n");
		return -1;
	}
	p->heads = 4;//4����ͷ
	p->cylinders = 4;//4������
	p->sectors = 128;//128������
	return 0;
}
*/
int ramblk_getgeo(struct block_device * blk_Dev, struct hd_geometry * hg)
{
	hg->cylinders = 64;
	hg->heads = 8;
	hg->sectors = (RAMBLK_SIZE/8/64/512);
	return 0;
}



static const struct block_device_operations ramblk_fops = {
	.owner	= THIS_MODULE,
	.getgeo = ramblk_getgeo,
};

static void do_ramblk_request(struct request_queue *q )
{
	struct request *req;
//	static volatile int r_cnt = 0;
//	static volatile int w_cnt = 0;
	//printk("ramblk_request_fn %d.\n",cnt++);
	req = blk_fetch_request(q);
	while (req) {
			unsigned long start = blk_rq_pos(req) << 9;
			unsigned long len  = blk_rq_cur_bytes(req);
//			printk("len=%d.\n",len);
		
			if (start + len > RAMBLK_SIZE) {
					printk("RAMBLK_SIZE< start+len");
					goto done;
				}
			
			if (rq_data_dir(req) == READ)
				memcpy(req->buffer, (char *)(start+ramblk_buf), len);
			else
				memcpy((char *)(start+ramblk_buf), req->buffer, len);
		
			done:
				if (!__blk_end_request_cur(req, 0))
					req = blk_fetch_request(q);
	}
	
}


static int ramblk_init(void)
{
//	1.����gendisk�ṹ�壬ʹ��alloc_disk����
	ramblk_disk = alloc_disk(16);//minors=����+1
//	2.����
//	2.1 ����/���ö��У��ṩ��д����.ʹ�ú���blk_init_queue(request_fn_proc *rfn,spin_lock_t *lock)
	ramblk_request_queue = blk_init_queue(do_ramblk_request,&ramblk_spinlock);
//	2.2 ����disk��������Ϣ���������������豸�ŵ�
	major = register_blkdev(0,"ramblk");//ע�����豸
	if(major < 0){//����Ƿ�ɹ�����һ����Ч�����豸��
		printk(KERN_ALERT "register_blkdev err.\n");
		return -1;
	}
	//�������豸��
	ramblk_disk->major = major;
	ramblk_disk->first_minor = 0;//���õ�һ�����豸��
	sprintf(ramblk_disk->disk_name, "ramblk%c", 'a');//�����豸��
	ramblk_disk->fops = &ramblk_fops;//����fops
	/*
	//����һ��ramdisk_info�ṹ�壬����ʼ��
	pinfo =(struct ramblk_info*)kmalloc(sizeof(struct ramblk_info),GFP_KERNEL);
	if(!pinfo){
		printk("kmalloc pinfo err.\n");
		return -1;
	}
	ramblk_info_init(pinfo);
	ramlk_disk->private_data = pinfo;*/
	ramblk_disk->queue = ramblk_request_queue;//�����������
	set_capacity(ramblk_disk, RAMBLK_SIZE/512);//��������
	
//	3.Ӳ����صĲ���
	ramblk_buf = (char*)vmalloc(RAMBLK_SIZE);//����RAMBLK_SIZE�ڴ�
	
//	4.ע��
	add_disk(ramblk_disk);//add partitioning information to kernel list
	printk("ramblk_init.\n");
	return 0;
}

static void ramblk_exit(void)
{
	
	unregister_blkdev(major,"ramblk");//ע���豸����
	blk_cleanup_queue(ramblk_request_queue);//�������
	del_gendisk(ramblk_disk);
	put_disk(ramblk_disk);
	vfree(ramblk_buf);//�ͷ�������ڴ�
	printk("ramblk_exit.\n");
}


module_init(ramblk_init);//���
module_exit(ramblk_exit);//����

MODULE_AUTHOR("jefby");
MODULE_LICENSE("Dual BSD/GPL");






