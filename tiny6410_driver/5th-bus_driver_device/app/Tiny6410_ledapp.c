/*
*	ƽ̨�豸��Ӧ��Ӧ�ó��� 
*	ʹ�ø�ʽ� argv[0] <0|1>
*	�ڲ�ʵ����ioctl�������������
*	��ƽ̨�豸�������޸���Ҫ���Ƶ����ţ���ƽ̨�����б��ֲ��伴��;ֻ��Ϊ��
*	ʾ�������еķֲ�ͷ������˼��
*
*/



#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

void print_usage(char *name)
{
	printf("Usage:\n");
	printf("%s <0|1>\n",name);
}

int main(int argc,char **argv)
{
	
	int fd = 0;
	char buf[4]={0x1,0x2,0x3,0x4};
	unsigned long  rbuf = 0;

	if(argc != 2){
		print_usage(argv[0]);
		return -1;
	}
	fd = open("/dev/platleds",O_RDWR);

	if(fd < 0){
		printf("open /dev/leds error.\n");
		return -1;
	}
	
	rbuf = strtoul(argv[1],0,10);
	if(!rbuf){
		ioctl(fd,0,0);//�ر�
	}else if(rbuf == 1){
		ioctl(fd,1,0);//��
	}else{
		print_usage(argv[0]);
		return -1;
	}
	close(fd);
	return 0;
}

