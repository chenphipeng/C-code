#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <errno.h>

#include "client.h"
#include <proto.h> // #include "../include/proto.h"

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

/* 坚持写够len个字节 */
static ssize_t writen(int fd, const char *buf, size_t len)
{
	int ret, pos=0;

	while(len > 0)
	{
		ret = write(fd, buf+pos, len);
		if(ret < 0)
		{
			if(errno == EINTR)
			{
				continue;
			}
			perror("write()");
			return -1;
		}
		len -= ret;
		pos += ret;
	}

	return pos;
}

/*
	-M	--mgroup	指定多播组地址
	-P	--port		指定接受端口
	-p	--player	指定播放器
	-H	--help		显示帮助

	程序初始化的级别：默认值 > 配置文件 > 环境变量 > 命令行参数
*/

int main(int argc, char **argv)
{
	int c; // 命令行参数的短格式
	int sd, pfd[2]; // sd是套接字，pfd是两个文件描述符
	int index = 0;
	int val;
	int pid;
	struct ip_mreqn mreq, mreq_d;
	struct sockaddr_in laddr, serveradd, remote_addr;
	socklen_t serveradd_len, remote_addrlen;
	struct option argarr[] = {{"port", 1, NULL, 'P'}, {"mgroup", 1, NULL, 'M'}, \
							  {"player", 1, NULL, 'p'}, {"help", 0, NULL, 'H'}, \
							  {NULL, 0, NULL, 0}};
	int recv_len; // 节目单包的实际长度
	int ret, choseid;
	
	/* 命令行参数分析 */
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

	/* 广播通信 */
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0)
	{
		perror("socket()");
		exit(1);
	}

	inet_pton(AF_INET, client_conf.mgroup, &mreq.imr_multiaddr);
	inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
	mreq.imr_ifindex = if_nametoindex("eth0");
	if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
	{
		perror("setsockopt()");
		exit(1);
	}

		/* 退出多播组 */
	inet_pton(AF_INET, client_conf.mgroup, &mreq_d.imr_multiaddr);
	inet_pton(AF_INET, "0.0.0.0", &mreq_d.imr_address);
	mreq.imr_ifindex = if_nametoindex("eth0");
	if(setsockopt(sd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq_d, sizeof(mreq_d)) < 0)
	{
		perror("setsockopt()");
		exit(1);
	}

		/* 提高局部效率 */
	val = 1;
	if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &val, sizeof(val)) < 0)
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

	if(pipe(pfd) < 0)
	{
		perror("pipe()");
		exit(1);
	}

	pid = fork();
	if(pid < 0)
	{
		perror("fork()");
		exit(1);
	}

	/* 子进程调用解码器 */
	if(pid == 0)
	{
		close(sd);
		close(pfd[1]);
		dup2(pfd[0], 0);
		if(pfd[0] > 0)
		{
			close(pfd[0]);
		}

		execl("/bin/sh", "sh", "-c", client_conf.player_cmd, NULL);
		perror("execl");
		exit(1);
	}
	
	/* 父进程从网络上收包，发送给子进程 */
	close(pfd[0]);
	// 收节目单包
	struct msg_list_st *msg_list;
	msg_list = malloc(MSG_LIST_MAX);
	if(msg_list == NULL)
	{
		perror("malloc()");
		exit(1);
	}

	while(1)
	{
		recv_len = recvfrom(sd, msg_list, MSG_LIST_MAX, 0, (void*)&serveradd, &serveradd_len);
		if(recv_len < sizeof(struct msg_list_st))
		{
			fprintf(stderr, "msg is too small!\n");
			continue;
		}
		else if(msg_list->chnid != LISTCHNID)
		{
			fprintf(stderr, "chnid is not mattch!\n");
			continue;
		}
		else
		{
			break;
		}
	}

	// 打印节目单并选择频道
	struct msg_listentry_st *pos;
	for(pos=msg_list->entry; (char*)pos<((char*)msg_list)+recv_len; pos=(void*)(((char*)pos)+pos->len))
	{
		printf("channel %d:%s\n", pos->chnid, pos->desc);
	}
	free(msg_list);

	while(1)
	{
		ret = scanf("%d", &choseid);
		if(ret != 1)
		{
			exit(1);
		}
	}

	// 收频道包，发送给子进程
	struct msg_channel_st *msg_channel;
	msg_channel = malloc(MSG_CHANNEL_MAX);
	if(msg_channel == NULL)
	{
		perror("malloc()");
		exit(0);
	}

	while(1)
	{
		recv_len = recvfrom(sd, &msg_channel, MSG_CHANNEL_MAX, 0, (void*)&remote_addr, &remote_addrlen);
		if(remote_addr.sin_addr.s_addr != serveradd.sin_addr.s_addr || remote_addr.sin_port != serveradd.sin_port)
		{
			fprintf(stderr, "Ignore:addr or port is not match!\n");
			continue;
		}
		if(recv_len < sizeof(struct msg_channel_st))
		{
			fprintf(stderr, "Ignore:message is too small!\n");
			continue;
		}
		if(msg_channel->chnid == choseid)
		{
			fprintf(stdout, "accept msg:%d recived!\n", msg_channel->chnid);
			if(writen(pfd[1], msg_channel->data, recv_len-sizeof(chnid_t)) < 0)
			{
				exit(1);
			}
		}
	}

	free(msg_channel);
	close(sd);
	exit(0);
}