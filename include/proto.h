#ifndef __PROTO_H__
#define __PROTO_H__

#include "site_type.h"

#define DEFAULT_MGROUP 		"224.2.2.2" // 组播地址 
#define DEFAULT_RCVPORT 	"1989"		// 端口 

#define CHNNR 				100			// 频道总数量 
#define LISTCHNID 			0			// 节目单频道 
#define MINCHNID 			1			// 最小频道ID 
#define MAXCHNID 			(MINCHNID+CHNNR-1) 	// 最大频道ID 

#define MSG_CHANNEL_MAX 	(65536-20-8)		// udp包推荐长度-ip包头部-udp包头部 = 频道包的大小 
#define MAX_DATA 			(MSG_CHANNEL_MAX-sizeof(chnid_t))

#define MSG_LIST_MAX		(65536-20-8)		// 节目单包的大小
#define MAX_ENTRY 			(MSG_LIST_MAX-sizeof(chnid_t))

/* 频道包结构 */
struct msg_channel_st{ 
	chnid_t chnid;			// 必须在[MINCHNID, MAXCHNID] 
	uint8_t data[1];
}__attribute__((packed));

/* 节目单上描述频道的内容的结构 */
struct msg_listentry_st{
	chnid_t chnid;
	uint16_t len; // 这个结构体变量的长度，用字节表示
	uint8_t desc[1];
}__attribute__((packed));

/* 节目单包结构 */
struct msg_list_st{
	chnid_t chnid;			// 必须是 LISTCHNID
	struct msg_listentry_st entry[1];
}__attribute__((packed));

#endif
