#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

/*thirdrvtest
所谓查询法，就是在应用程序里面执行

*/

int main(int argc, char **argv)
{
	int fd;
	int cnt=0;
	unsigned char key_val;
	
	
	fd = open("/dev/buttons",O_RDWR);
	if(fd<0)
		{
		printf("can't open!\n");
		}

	while(1)
		{
		//read(fd,buf,sizeof(buf))从文件fd中读取sizeof(buf)个字节放到buf指针指向的内存中。
		read(fd,&key_val,1);
		printf("key_val = 0x%x\n",key_val);
		}

	return 0;
}


