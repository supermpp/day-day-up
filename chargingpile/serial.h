#ifndef _serial_h
#define _serial_h
#include "linklist.h"

extern unsigned char PSN  ;
extern unsigned char PSN_0b  ;

#define STX_CP 0x02
#define ETX_CP 0x03
#define DLE_CP 0x10

//是否下发指令
#define SEND 		0x00
#define NOSEND 	0x01

//充电桩命令码
#define CMD_CLOCK_SYNC 		0x01	//时钟同步指令
#define CMD_GET_DEV_PARA 	0x04	//获取设备参数指令
#define CMD_SET_DEV_PARA 	0x05	//设置设备参数指令
#define CMD_BEEP_CONT 		0x06	//蜂鸣器控制指令
#define CMD_QUERY_STATE 		0x0B	//设备状态查询指令
#define CMD_ELECTRICIZE 		0x0C	//设备充电控制指令
#define CMD_GET_TRAN_MSG 	0x0E	//设备获取交易卡片信息指令
#define CMD_PAYMENT_TRAN 	0x0F	//卡片扣款交易指令
#define CMD_EVENT_QUERY 		0x10	//事件查询指令
#define CMD_GET_DEVMSG	 	0x11	//获取设备信息指令
#define CMD_CLEAN_TRAN	 	0x1F	//清交易事件指令
#define CMD_DEV_RESET	 	0x20	//重置设备指令
#define CMD_GET_DEV_SN	 	0x20	//获取设备SN信息指令
#define CMD_SET_DEV_SN	 	0x20	//设置设备SN信息指令
#define CMD_UPDATECP			0x21//更新文件下载指令

#define CTRLB_START 			0x01 //启动；插入充电枪判断后开始正式充电，超时未插入退出。
#define CTRLB_END 			0x02//结束；充电结束，未结算
#define CTRLB_FINISH			0x03//完成；交易完成，可释放充电枪。
#define CTRLB_CARD			0x04//选择交通卡支付
#define CTRLB_PAYMENTEND        0x44//add
#define CTRLB_ETC				0x05//选择etc支付
#define CTRLB_START_WAIT		0x81//退出启动充电状态，返回待机状态
#define CTRLB_WILLPAY_CHARG	0x82//退出待付费状态返回充电状态
#define CTRLB_CHANGEPAY		0x84//退出付费状态返回结束充电状态，等待选择支付方式
#define CTRLB_ALLSTOP		0xee//应急停止；直接停止充电，并释放充电枪
#define CTRLB_CHARGFAULT        0xf0//车辆返回充电完成或充电故障，停止充电，进入待付费
#define CTRLB_CHARGKEYFAULT 0xf8//充电开关故障，停止充电，进入待付费


//充电桩内部事件
#define EVENT_READCARD   	0x01 //卡片刷卡信息
#define EVENT_PAYMENT    	0x02 //卡片扣款交易
#define EVENT_CHARGCTL  	0x04//充电控制事件

//充电桩指令执行结果状态
#define STATUS_SUCCESS 	0x00//执行成功
#define STATUS_BUSY		0x01//设备忙
#define STATUS_RW_F		0x02//交易读写器设备故障
#define STATUS_PSAM_F	0x03//交易PSAM故障（交通卡或ETC）
#define STATUS_RDB_F		0x04//测量电表故障
#define STATUS_CTL_F		0x05//充电控制开关故障
#define STATUS_SYS_F		0x07//系统错误
#define STATUS_NOEVENT     0x21//无事件
#define STATUS_WRONG       0x22//状态不正确
#define STATUS_HWOK         0xfc//硬件状态标准

//卡片扣款交易标志
#define Deal_Success     0x00//交易成功
#define Deal_Fail		0x01//交易未成功
#define Deal_NotAck	0x02//交易不确认，需要重新刷卡判断

#define BAUND 	57600
#define PARITY 	0	//0:None 1:Odd 2:Even 3:Mrk 4:Spc
#define DATABITS	8
#define STOPBITS 	1

#define RESPONSE0B 29//设备状态查询指令回帧长度

#define ClockDeviation 30//充电桩时钟误差


typedef struct
{
	unsigned char ADDR;
	unsigned char CLS;
	unsigned char PSN;
	unsigned char PARM;
	unsigned char LE;
	unsigned char data[256];
}CMD_DATA_S,*PCMD_DATA_S;


int MGTR_UART_tx_format  (unsigned char* cmd, unsigned int cmdlen, unsigned char* tx_buf);
int Charging_Frame(PCMD_DATA_S cmd,unsigned char *dstframe);
int MGTR_UART_rx_format( unsigned char* rcv, unsigned int rcvlen, unsigned char* rspbuf,unsigned char xpsn);
int Frame_GetDevmsg(char num,char *chargsn,char *dstframe);
int Frame_UpdateCPfile(char num,char *data,int datalen ,char *chargsn,char *dstframe);
int Frame_EventQuery(unsigned char myPSN,unsigned char *SN,unsigned char type,unsigned char *dstframe);
int Frame_ChargingControl(unsigned char myPSN,unsigned char *SN,unsigned char *EventSN,unsigned char Ctrlbyte,unsigned char *dstframe);
int Create_Frame_StatusRequest(unsigned char myPSN,unsigned char *SN,unsigned char *dstframe);
int Read_SerialData(int fd,unsigned char *data);
int Unpack_Frame_StatusRequest(unsigned char *srcframe,int srclen,struct status *stat,unsigned char *dstframe,unsigned char xpsn);
int Frame_CleanEvent(unsigned char myPSN,struct status *charg,unsigned char CEvent,unsigned char *dstframe);
int Frame_payment(unsigned char myPSN,struct status *charg,unsigned char *dstframe);
int Frame_Clock_Sync(unsigned char myPSN,unsigned char *dstframe);
//int Frame_UpdateCP(char num,char *chargsn,char *buff,int buf_len,char *dstframe);
//int Unpack_Frame_StatusRequest(unsigned char *dstframe,int len,struct status *stat);


#endif
