#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "client.h"
#include <proto.h> // #include "../include/proto.h"

/*
	-M	--mgroup	指定多播组地址
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
	printf("-M	--mgroup	指定多播组地址 \n");
	printf("-p	--player	指定播放器 \n");
	printf("-H	--help		显示帮助\n");
}

int main(int argc, char **argv)
{
	int c;
	int sd;
	int index = 0;
	struct ip_mreqn mreq;
	struct sockaddr_in laddr;
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

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0)
	{
		perror("socket()");
		exit(1);
	}

	inet_pton(AF_INET, client_conf.mgroup, &mreq.imr_multiaddr);
	inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
	mreq.imr_ifindex = if_nametoindex("eth0");

	if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreqn, sizeof(mreqn)) < 0)
	{
		perror("setsockopt()");
		exit(1);
	}

	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(atoi(client_conf.rcvport));
	inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

	if(bind(AF_INET, (void*)&laddr, sizeof(laddr)) < 0)
	{
		perror("bind()");
		exit(1);
	}
	
	exit(0);
}
