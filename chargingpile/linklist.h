
#ifndef _linklist_h
#define _linklist_h


#include <stdio.h>
#include <pthread.h>


#define CONNECT 0x00
#define UNCONNECT 0x01

struct node{
	unsigned char cmdid;//cmdid 协议命令号(如果cmdid==0x04将会产生一条事件)
	unsigned char eventtype;//事件类型标志
	unsigned int DATAID;//DATAID 通讯序号，用于区分不同的通讯帧
	unsigned int T_SN;//T_SN 采集器编号
	unsigned char SN[12];//SN充电桩编号
	unsigned char Serialnum;//串口号,cmdid=0x02有用到,报文通过采集器上的哪个串口转发只对存储通过串口转发的数据报文有效
	unsigned short Serialtimeout;//串口超时时间,cmdid=0x02有用到,100ms较合适，当此值为0x00时，代表充电桩不会应答数据，例如广播报文
	unsigned char controlnum;//控制指令控制码cmdid=0x04有用到
	unsigned char Charg_SN[12];//充电过程SN ,同充电桩的事件序号,cmdid=0x04有用到
	unsigned int Wgevent_SN;//事件SN由采集器维护的交易序号,每个事件都不相同

	unsigned char frame_psn;
	unsigned char flag1;//Stime SDn有效标示0x01有效
	unsigned char Stime[7];//充电桩开始充电时间----0x04服务端下发下来的
	unsigned char  SDn[4];//开始充电时的电能----0x04服务端下发下来的
	unsigned char flag2;//Pay 有效标示0x01有效
	unsigned int Pay;//付费金额----0x04服务端下发下来的
	int DATA_LEN;//len of DATA DATA长度 
	
	char DATA[380];//DATA ----不同cmdid对应不同的内容，也可能是串口数据帧
	int sendtime;//报文发送时间，用于超时重发
	struct node *next;
};


//充电桩状态
struct status{
	//充电桩返回的字段
	unsigned char SN[12];//SN充电桩编号
	unsigned char CStatus;//充电桩当前状态
	unsigned char CEvent;//充电桩内部事件状态，位设置 1-有，0-无B0 - 卡片刷卡信息B1 - 卡片扣款交易B2 - 充电控制事件
	unsigned char EventSN[12];//充电桩事件序号,同struct node里的Charg_SN
	unsigned char DT[7];//当前时间：YYYYMMDDhhmmss
	unsigned char ENERGY[4];//电表当前总电能：XXXXXX.XX 单位kWh
	char RFU[11];//备用
	//char FlagCost;//实时费用有效标志,connect标志如果不通，则实时费用无效
	//unsigned int TimeCharg;//持续充电时间单位秒 ,=(DT- SCT)
	
	//补充
	unsigned char connectfailn;//new add 失败次数,当连续超过10次的时候 connect才置为失败
	unsigned char connect;//通讯状态0x00:通0x01:不通
	unsigned char CFlg;//充电控制事件(查询事件的时候赋值)
	unsigned char Flag1;//0x01有效用于标示MoneyCharg是否有效,若服务器传给网关的收费金额有效就用此金额结算,无效则由STARTENERGY ENDENERGY计算
	unsigned int MoneyCharg;//充电金额单位分 ,(ENDENERGY - STARTENERGY)*rate
	unsigned char SCT[7];//充电桩开始充电时间
	unsigned char STARTENERGY[4];//开始充电时的电能用于充电费用结算
	unsigned char ENDENERGY[4];//结束充电时的电能用于充电费用结算
	unsigned char Hw_Status;//硬件状态
	unsigned char FailPay;//扣款失败标志，用于重发下一次扣款指令
	unsigned char CardID[4];// 4字节物理卡号，待交易卡ID,通过询卡指令获取
	unsigned char CardSN[10];// 10字节卡片序列号
	unsigned char Yue[4];//卡片余额
	unsigned char CPFWW[2];//充电桩软件版本号
	char Flag_headfile;//下发头文件标志，0可下发，1已经下发过了。
	char Flag_lastfile;//下发更新文件最后一条标志，0代表没下发到最后一条，1代表下发到最后一条。
	char UpStatus;//0x01可更新0x02更新中
	char FLAG_CMD11;//获取设备信息指令的标志00未查询过(需要查询)0x01已经查询过了
	//unsigned char carddata[241];//存储刷卡或扣款信息
};


extern struct node *head_tcpsendlist;
extern struct node *head_serialsendlist;
extern struct node *head_tcphavesendlist;
extern struct node *head_eventlist_old;

//new
/*每一个串口有自己的串口数据链表*/
extern struct node *head_serialsendlist1;
extern struct node *head_serialsendlist2;
extern struct node *head_serialsendlist3;
extern struct node *head_serialsendlist4;
extern struct node *head_serialsendlist5;
extern struct node *head_serialsendlist6;


extern pthread_mutex_t tcpsendlist_lock;
extern pthread_mutex_t serialhavesendlist_lock;
extern pthread_mutex_t serialsendlist_lock;

struct  node *AddFromEnd(struct node *head,struct node *new );
void print_list(struct node*head);
void InitList(struct node **p);
struct node *DelFromHead(struct node *head,struct node *deldata);
int Findlist(struct node *head,struct node *dstnode,unsigned int key);
int DelDesignedlist(struct node *head,struct node *dstnode,int key);
int GetStatus_SN(struct status *dst,struct status **src,int num,struct node *p);
int GetStatus_BySN(struct status **p,int num,unsigned char *SN,struct status *dst);
int SetStatuslist_EventSN(struct status **p,int num,struct node *src);
int SetStatuslist_SCTDN(struct status **p,int num,struct node *src);
int SetStatuslist_Pay(struct status **p,int num,struct node *src);
int getIndex_BySN(struct status **src,int num,struct node *p);

#endif


