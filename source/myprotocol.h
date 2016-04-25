#ifndef _myprotocol_h
#define _myprotocol_h
#include "linklist.h"

#define STX 0xa1
#define ETX 0x55
#define  ENCRYPIF 0x00 //0x00 不加密 0x01加密

//
#define SERVER_RECONNECT 	0x01
#define SERVER_CONNET		0x00

//网关服务器通讯命令码
#define CMDID_FILEDOWNLOAD   0x01//文件下传命令
#define CMDID_PASSTHROUGH     0x02//数据透传命令
#define CMDID_STAT_UPDATE	0x03//状态上报命令码
#define CMDID_CHARGCONTROL  0x04//充电过程控制命令
#define CMDID_DEAL_UPDATE 	0x05//交易事件上传指令
#define CMDID_SERVER_GATHER 0x06//事件补传命令
#define CMDID_PROCESS_QUERY  0x07 //充电过程查询
#define CMDID_HEARTTAG		0x08//心跳包
 
//错误标志
#define PAYSUCCESS 	0x00//支付成功
#define NOTENOUGH 	0x01//余额不足
#define CARDMOVE   	0x02//支付过程中卡片移动，请再刷一次
#define BLACKCARD 	0x03//黑名单卡片
#define DEALNOTACK     0x04//交易不确认请重新刷卡 
#define DEALFAIL		0x05//交易不成功

//充电过程查询结果标志
#define QUERY_SUCCESS 	0x00	 //查询成功 
#define QUERY_UNUSED	 	0x01	//没有处于充电状态,以下数据无效
#define QUERY_CONNECTFAIL 0x02//通讯中断以下数据无效

//文件下传命令错误码
#define UPDATE_SUCCESS 		0x00//成功
#define UPDATE_FORMWRONG	0x85//文件格式不正确
#define UPDATE_NOSUPPORT	0x86//不支持此命令

//文件下传--文件代号
#define F_TYPE_01 0x01//集中器参数设置文件
#define F_TYPE_02 0x02//费率文件
#define F_TYPE_03 0x03//程序升级文件
#define F_TYPE_04 0x04//交通卡黑名单
#define F_TYPE_05 0x05//充电桩更新下载文件

#define CLEANMAX  5

extern unsigned int EVENTSN ;

unsigned char XOR_SUM(const unsigned char *src,int len);
unsigned int FourbytesToInt(const unsigned char *buff);
int AnalyzePacket(const unsigned char *src,int len,unsigned char *cmdid,unsigned int *dataid,unsigned int *t_sn,unsigned char *data);
int Response_Frame(unsigned char *frame,unsigned char cmdid,unsigned int DATAID,unsigned int T_SN,unsigned char *DATA,int DATA_LEN);
int Deal_Updatefile(char *src,unsigned short *filenum,unsigned int *filesize,unsigned short *filecount);


void Deal_0x04(unsigned int dataid,unsigned int t_sn,unsigned char *data,unsigned int datalen);
int Find_Rate(unsigned char *time,int len_time,int num,unsigned char *nu,unsigned int *rate1,unsigned int *rate2,unsigned int *rate3);
void Event_transfer(unsigned char *startdate,unsigned char *enddate);
void StrtoBCD(unsigned char *src,int len,unsigned char *dst);
int Create_Data_Status(unsigned char *dstbuf,int num,struct status *p);
int Frame_readcard(unsigned char PSN,unsigned char *SN,unsigned char *EventSN,unsigned char *dstframe);
int Create_Data_History(unsigned char *dst,int offset,char flag_end,unsigned char* data,int data_len);

int Create_Data_EventUp(unsigned char *dstbuf,struct status *src,unsigned char *carddata,int *flag,unsigned char *money);
void CharToString(void *d,const void *s,int len);
void get_card_outnum(unsigned char *card_id,char *card_num);

#endif

