#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TRANS_NO_DMA 0//��ʹ��DMA��������
#define TRANS_USE_DMA 1//ʹ��DMA��������

static void print_usage(const char *name)
{
	printf("Usage:\n");
	printf("%s < nodma | dma >\n",name);
}
int main(int argc,char **argv)
{
	int fd = 0;
	if(argc != 2){
		print_usage(argv[0]);
		return -1;
	}
	fd = open("/dev/dma-6410s",O_RDWR);
	if(fd < 0){
		printf("can't open /dev/dma-6410s.\n");
		return -1;
	}
	if(strcmp(argv[1],"nodma") == 0){
		ioctl(fd,TRANS_NO_DMA);
	}else if(strcmp(argv[1],"dma") == 0){
		ioctl(fd,TRANS_USE_DMA);
	}else{
		print_usage(argv[0]);
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}
