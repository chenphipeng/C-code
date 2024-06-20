#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include "client.h"

/*
	-M	--mgroup	指定多播组
	-P	--port		指定接受端口
	-p	--player	指定播放器
	-H	--help		显示帮助
*/

// C99标准引入的一种结构体初始化方式，称为“指定初始化器”(designated initializer)
struct client_conf_st client_conf = {.rcvport = DEFAULT_RCVPORT, \
									 .mgroup = DEFAULT_MGROUP, \
									 .player_cmd = DEFAULT_PLAYERCMD};

static void printhelp()
{
	printf("-P	--port		指定接受端 \n");
	printf("-M	--mgroup	指定多播组 \n");
	printf("-p	--player	指定播放器 \n");
	printf("-H	--help		显示帮助\n");
}

int main(int argc, char **argv)
{
	int c;
	int index = 0;
	struct option argarr[] = {{"port", 1, NULL, 'P'}, {"mgroup", 1, NULL, 'M'}, \
							  {"player", 1, NULL, 'p'}, {"help", 0, NULL, 'H'}, \
							  {NULL, 0, NULL, 0}};

	/*
		初始化
		级别：默认值、配置文件、环境变量、命令行参数
	*/
	
	while(1)
	{
		c = getopt_long(argc, argv, "P:M:p:H", argarr, &index);
		if(c < 0)
		{
			break;
		}

		switch(c)
		{
			case 'P':
				client_conf.rcvport = optarg;
				break;
			case 'M':
				client_conf.mgroup = optarg;
				break;
			case 'p':
				client_conf.player_cmd = optarg;
				break;
			case 'H':
				printhelp();
				exit(0);
				break;
			default:
				abort();
				break;
		}
	}

	socket();
	
	exit(0);
}
