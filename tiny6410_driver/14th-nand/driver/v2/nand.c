

//�ο�drivers/mtd/s3c2410.c /drivers/mtd/nand/at91_nand.c
//NANDоƬK9GAG08U0E.pdf,������������Ϊ6410��NAND FLASHֻ֧��8-bitECC����оƬҪ��24-bit��ECC�����޷�ʹ��
//ʹ�����ECC֤���ǲ����Ե�,���ECC��Ӳ��ECC���������ֵ��ƥ�䣬���Դ��ڴ���ֻ�ò�ʹ��ECC��������ΪNAND FLASH����λ��ת����
//�����ǲ�̫�õ�.�Ժ���ʱ��;����ٸĽ�


#include <linux/module.h>
#include <linux/init.h>

#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/clk.h>
//nand controller�Ĵ�������
struct s3c_nand_regstruct{
	unsigned long		   nfconf		 ;	
	unsigned long		   nfcont		 ;	
	unsigned long		   nfcmmd		 ;	
	unsigned long		   nfaddr		 ;	
	unsigned long		   nfdata		 ;	
	unsigned long		   nfmeccd0 	 ;	
	unsigned long		   nfmeccd1 	 ;	
	unsigned long		   nfseccd		 ;	
	unsigned long		   nfsblk		 ;	
	unsigned long		   nfeblk		 ;	
	unsigned long		   nfstat		 ;	
	unsigned long		   nfeccerr0	 ;	
	unsigned long		   nfeccerr1	 ;	
	unsigned long		   nfmecc0		 ;	
	unsigned long		   nfmecc1		 ;	
	unsigned long		   nfsecc		 ;	
	unsigned long		   nfmlcbitpt	 ;	
	unsigned long		   nf8eccerr0	 ;	
	unsigned long		   nf8eccerr1	 ;	
	unsigned long		   nf8eccerr2	 ;	
	unsigned long		   nfm8ecc0 	 ;	
	unsigned long		   nfm8ecc1 	 ;	
	unsigned long		   nfm8ecc2 	 ;	
	unsigned long		   nfm8ecc3 	 ;	
	unsigned long		   nfmlc8bitpt	 ;	

};

static struct nand_chip * s3c_nandchip = NULL;
static struct mtd_info * s3c_mtdinfo = NULL;
static struct s3c_nand_regstruct * s3c_nand_reg_ptr = NULL;
static struct clk * clk = NULL;

//nand_chipоƬѡ�к���.XmnCS2
static void s3c_nand_select_chip(struct mtd_info *mtd, int chip)
{
	if(chip == -1){
		//ȡ��ѡ��: BIT1 Ϊ1
		s3c_nand_reg_ptr->nfcont |= (0x1<<1);
		
	}else{
		//ѡ��: BIT1 Ϊ0
		s3c_nand_reg_ptr->nfcont &= ~(0x1<<1);
	}
}
static void  s3c_nand_cmd_ctrl(struct mtd_info *mtd, int dat, unsigned int ctrl)
{
	if (dat == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE){
		//���� NFCMMD = data
		s3c_nand_reg_ptr->nfcmmd = dat;
	}else if(ctrl & NAND_ALE){
		//��ַ,NFADDR = data
		s3c_nand_reg_ptr->nfaddr = dat;
	}
}
int s3c_nand_dev_ready(struct mtd_info *mtd)
{
	//return "NFSTAT??bit0";
	return ( s3c_nand_reg_ptr->nfstat & (0x1<<0) );
}

static int samsung_nand_init(void)
{

	//1.����һ��nand_chip�ṹ��
	s3c_nandchip = kzalloc(sizeof(struct nand_chip),GFP_KERNEL);
	if(s3c_nandchip==NULL){
		printk("kzalloc nand_chip err.\n");
		return -1;
	}
	//ӳ��NAND FLASH Controller�������ڴ�,�õ������ַ	s3c_nand_reg_ptr
	s3c_nand_reg_ptr = ioremap(0x70200000,sizeof(struct s3c_nand_regstruct));

	if(!s3c_nand_reg_ptr){
		printk("s3c_nand_reg_ptr ioremap err.\n");
		return -1;
	}else{
		printk("s3c_nand_reg_ptr=%p",s3c_nand_reg_ptr);
	}
	

	//2.����
	s3c_nandchip->select_chip 	= s3c_nand_select_chip;//Ƭѡ����
	s3c_nandchip->cmd_ctrl 		= s3c_nand_cmd_ctrl;//�������

	s3c_nandchip->IO_ADDR_R		= &s3c_nand_reg_ptr->nfdata;//����ַ
	s3c_nandchip->IO_ADDR_W 	= &s3c_nand_reg_ptr->nfdata;//д��ַ
	s3c_nandchip->dev_ready		= s3c_nand_dev_ready;//����豸�Ƿ�æ
	
	s3c_nandchip->ecc.mode = NAND_ECC_NONE;//��ʹ��Ӳ���������ECC,֤�������ǿ��Ե�
	//3.Ӳ���������

	clk = clk_get(NULL,"nand");
	clk_enable(clk);//ʹ��ʱ��
	
	//NAND FLASHӲ������
	//HCLK=133MHZ,ʱ�� = 7.5ns
	//TACLS:CLE��ALEоƬ�����೤ʱ�����Է���nWE�ź�,��NAND FLASHоƬ�ֲ���Կ���ͬʱ����,TACLS=0
	//TWRPH0:nWE���ֵ�ʱ�� HCLKx(TWRPH0+1),��оƬ�ֲ��֪>=15ns,????TWRPH0=1
	//TWRPH1:nWE��Ϊ�ߵ�ƽ��ά�ֵ�ʱ�� HCLKX(TWRPH1+1)��оƬ�ֲ��֪>==5ns,TWRPH1=0
#define TACLS 0
#define TWRPH0 2
#define TWRPH1 0
	s3c_nand_reg_ptr->nfconf = (TACLS<<12) | (TWRPH0 << 8) | (TWRPH1 << 4);

	//NFCONT
	//BIT1 : ȡ��NAND FLASHƬѡ
	//BIT0 : ʹ��NAND FLASH������
	s3c_nand_reg_ptr->nfcont = (0x1<<1) | (0x1<<0);

	
	//4.ʹ��nand_scan����
	s3c_mtdinfo = kzalloc(sizeof(struct mtd_info),GFP_KERNEL);
	if(!s3c_mtdinfo){
		printk("kzalloc s3c_mtdinfo err.\n");
		return -1;
	}

	s3c_mtdinfo->owner = THIS_MODULE;
	s3c_mtdinfo->priv = s3c_nandchip;
	
	nand_scan(s3c_mtdinfo,1);//����NAND FLASH�豸������ҵ������
	//5.add_mtd_partions
	//add_mtd_partitions(struct mtd_info * master,const struct mtd_partition * parts,int nbparts);
	
	return 0;
}

static void samsung_nand_exit(void)
{
	iounmap(s3c_nand_reg_ptr);
	kzfree(s3c_nandchip);
	kzfree(s3c_mtdinfo);
	
}

module_init(samsung_nand_init);
module_exit(samsung_nand_exit);

MODULE_LICENSE("Dual BSD/GPL");




