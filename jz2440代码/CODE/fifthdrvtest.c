#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>



/*fifthdrvtest

*/
	int fd;

	void my_signal_fun(int signum)
	{
		unsigned char key_val;
		read(fd,&key_val,1);//读取按键值
		printf("key_val: 0x%x\n",key_val);
	}
	


int main(int argc, char **argv)
{
	
	int ret;
	int Oflags;
	unsigned char key_val;
	/* 注 册 信 号 处 理 函 数 */
	signal(SIGIO,my_signal_fun);//当按键触发就会来调用信号处理函数,signal指向my_signal_fun函数
	
	fd = open("/dev/buttons",O_RDWR);
	if(fd<0)
		{
		printf("can't open!\n");
		}

	/* 告 诉 驱 动 程 序 信 号 发 给 谁 */
	/* getpid() 可 获 取 进 程 的 id 号 */
	fcntl(fd, F_SETOWN, getpid());

	/* 获 取 文 件 的 flags ，即 open 函 数 的 第 二 个 参 数(O_RDWR) */
	Oflags = fcntl(fd, F_SETFL);

	/* 改 变 flag , 设 置 为 异 步 通 信(FASYNC) 的 flag*/
	fcntl(fd, F_SETFL, Oflags | FASYNC);//改变这个的flag时驱动程序中的fifth_drv_fasync就会被调用
	
	while(1)
		{
		sleep(1000);
		}

	return 0;
}


