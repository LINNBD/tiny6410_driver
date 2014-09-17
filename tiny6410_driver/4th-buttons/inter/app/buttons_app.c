/*
 *
 *
 *	 buttons���û����������������ʹ��
  *  Author:jefby
  *  Email:jef199006@gmail.com

 *
 *
 * */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>


int main(int argc,char **argv)
{
	int i;
	int ret;
	int fd;
	int press_cnt[4];
	//���豸
	fd = open("/dev/buttons",0);
	if(fd < 0){
		printf("can't open /dev/buttons\n");
		return -1;
	}
	while(1){//ѭ����ȡ����read����ִ��ǰ�ް��������£����ʱ���̴�������״̬.ֻ��ʹ�ð���K1~4
		ret = read(fd,press_cnt,sizeof(press_cnt));
		if(ret < 0){
			printf("read err.\n");
			continue;
		}
		for(i=0;i<sizeof(press_cnt)/sizeof(press_cnt[0]);++i){
			if(press_cnt[i])
				printf("K%d has been pressed %d times!\n",i+1,press_cnt[i]);
		}
	}
}

