/*
	���豸��������:����豸�����Ŀ��
	�ÿ��豸���ڴ���ģ��,��̬����1MB���ڴ���Ϊ���豸
*/


#include <linux/init.h>//module_init/exit
#include <linux/module.h>//MODULE_AUTHOR,MODULE_LICENSE��
#include <linux/genhd.h>//alloc_disk
#include <linux/blkdev.h>//blk_init_queue
#include <linux/fs.h>//register_blkdev,unregister_blkdev
#include <linux/types.h>//u_char,u_short

#define RAMBLK_SIZE (1024*1024)//���豸��С




static struct gendisk * ramblk_disk = NULL;
static struct request_queue * ramblk_request_queue = NULL;
static int major = 0;//���豸�����豸��

static DEFINE_SPINLOCK(ramblk_spinlock);//���岢��ʼ��һ��������



int ramblk_ioctl(struct block_device *blk_dev, fmode_t fm, unsigned cmd , unsigned long arg)
{
	return 0;
}

int ramblk_getgeo(struct block_device * blk_Dev, struct hd_geometry * hg)
{
	return 0;
}



static const struct block_device_operations ramblk_fops = {
	.owner	= THIS_MODULE,
	.ioctl	= ramblk_ioctl,
	.getgeo = ramblk_getgeo,
};
//���豸��������
static void ramblk_request_fn(struct request_queue *q )
{
	static int cnt = 0;
	struct request *req;
	printk("ramblk_request_fn %d.\n",cnt++);
	req = blk_fetch_request(q);//��һ����������л�ȡһ��I/O����
	while (req) {
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
	ramblk_request_queue = blk_init_queue(ramblk_request_fn,&ramblk_spinlock);
//	2.2 ����disk��������Ϣ���������������豸�ŵ�
	major = register_blkdev(0,"ramblk");//ע����豸
	if(major < 0){//����Ƿ�ɹ�����һ����Ч�����豸��
		printk(KERN_ALERT "register_blkdev err.\n");
		return -1;
	}
	//�������豸��
	ramblk_disk->major = major;
	ramblk_disk->first_minor = 0;//���õ�һ�����豸��
	sprintf(ramblk_disk->disk_name, "ramblk%c", 'a');//���ÿ��豸��
	ramblk_disk->fops = &ramblk_fops;//����fops

	//���û�б�Ҫ������ɾ��.����һ��ramdisk_info�ṹ�壬����ʼ��
	#ifdef 0
	pinfo =(struct ramblk_info*)kmalloc(sizeof(struct ramblk_info),GFP_KERNEL);
	if(!pinfo){
		printk("kmalloc pinfo err.\n");
		return -1;
	}
	ramblk_info_init(pinfo);
	ramblk_disk->private_data = pinfo;
	#endif 
	
	ramblk_disk->queue = ramblk_request_queue;//�����������
	set_capacity(ramblk_disk, RAMBLK_SIZE);//��������
	printk(" %s: CHS=%d/%d/%d\n", ramblk_disk->disk_name,pinfo->cylinders, pinfo->heads, pinfo->sectors);
//	3.ע��
	add_disk(ramblk_disk);//add partitioning information to kernel list
	printk("ramblk_init.\n");
	return 0;
}

static void ramblk_exit(void)
{
	kfree(pinfo);//�ͷ�����Ŀռ�
	unregister_blkdev(major,"ramblk");//ע���豸����
	blk_cleanup_queue(ramblk_request_queue);//�������
	del_gendisk(ramblk_disk);
	put_disk(ramblk_disk);
	printk("ramblk_exit.\n");
}


module_init(ramblk_init);//���
module_exit(ramblk_exit);//����

MODULE_AUTHOR("jefby");
MODULE_LICENSE("Dual BSD/GPL");






