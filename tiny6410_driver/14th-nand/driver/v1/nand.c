

//�ο�drivers/mtd/s3c2410.c����/drivers/mtd/nand/at91_nand.c
#include <linux/module.h>
#include <linux/init.h>

#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>


static struct nand_chip * s3c_nandchip = NULL;
static struct mtd_info * s3c_mtdinfo = NULL;


//nand_chip�ṩ��ѡ��оƬ����Xm0CSn2
static void s3c_nand_select_chip(struct mtd_info *mtd, int chip)
{
	if(chip == -1){
		//ȡ��ѡ��,NFCONT bit1����Ϊ0
	}else{

		//NFCONT��bit1����Ϊ1
	}
}
static void  s3c_nand_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE){
		//������,NFCMMD = data
	}else{

		//����ַ,NFADDR = data
	}
}
int s3c_nand_dev_ready(struct mtd_info *mtd)
{
	return "NFSTAT��bit0";
}

static int samsung_nand_init(void)
{
	//1.����һ��nand_chip�ṹ��
	s3c_nandchip = kzalloc(sizeof(struct nand_chip),GFP_KERNEL);
	if(s3c_nandchip==NULL){
		printk("kzalloc nand_chip err.\n");
		return -1;
	}

	//2.����nand_chip��nand_scan����ʹ�õ�.
	//2.1��Ҫ�ṩ� ѡ��-������-��ַ-���ݹ���
	s3c_nandchip->select_chip 	= s3c_nand_select_chip;
	s3c_nandchip->cmd_ctrl 		= s3c_nand_cmd_ctrl;
	s3c_nandchip->IO_ADDR_R		= "NFDATA�������ַ";
	s3c_nandchip->IO_ADDR_W 	= "NFDATA�������ַ";
	s3c_nandchip->dev_ready		= ;
	
	
	//3.Ӳ����ص�����
	//4.ʹ��:nand_scan
	s3c_mtdinfo = kzalloc(sizeof(struct mtd_info),GFP_KERNEL);
	if(!s3c_mtdinfo){
		printk("kzalloc s3c_mtdinfo err.\n");
		return -1;
	}

	s3c_mtdinfo->owner = THIS_MODULE;
	s3c_mtdinfo->priv = s3c_nandchip;
	
	nand_scan(s3c_mtdinfo,1);
	//5.add_mtd_partions
	add_mtd_partitions(struct mtd_info * master,const struct mtd_partition * parts,int nbparts);
	
	return 0;
}

static void samsung_nand_exit(void)
{

}

module_init(samsung_nand_init);
module_exit(samsung_nand_exit);

MODULE_LICENSE("Dual BSD/GPL");








