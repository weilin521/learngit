#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>

/*thirdrvtest
所谓查询法，就是在应用程序里面执行

*/

int main(int argc, char **argv)
{
	int fd;
	int ret;
	unsigned char key_val;
	struct pollfd fds[1];
	
	fd = open("/dev/buttons",O_RDWR);
	if(fd<0)
		{
		printf("can't open!\n");
		}
	
	fds[0].fd = fd;//查询fd文件
	fds[0].events = POLLIN;//There is data to read有数据读取
	while(1)
		{
		//int poll(struct pollfd *fds, nfds_t nfds, int timeout)timeout是以ms为单位
		ret = poll(fds,1,5000);//
		if(ret == 0)
			{
			printf("time out\n");
			}
		else
			{
		//read(fd,buf,sizeof(buf))从文件fd中读取sizeof(buf)个字节放到buf指针指向的内存中。
		read(fd,&key_val,1);
		printf("key_val = 0x%x\n",key_val);
			}
		}

	return 0;
}


