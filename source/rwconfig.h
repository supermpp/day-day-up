#ifndef _rwconfig_h
#define _rwconfig_h

 #include "linklist.h"
 
#define WGCONFIGFILE "/etc/wgconfig.conf"
#define RATEFILE "/etc/rate.conf"
#define BLACKCARDFILE "/etc/blackcard.conf"
#define WGINFOINI "/etc/wginfo.ini"
#define StatusFile "/etc/piplestatus.txt"
#define EVENTFILE "/root/eventlist.txt"

#define SERIALNUM 2//串口数



extern struct servercfg remoteserver;
extern int chargingpilenum;//充电桩总数
extern unsigned char serailnum_sn[896];//所有充电桩sn和串口号
extern int serial1chargnum;//串口1充电桩数
extern unsigned char serail1SN[384];//串口1的SN号32*12=384

extern int serial2chargnum;//串口2充电桩数
extern unsigned char serail2SN[384];//串口1的SN号32*12=384

extern unsigned char AllSerialSn[SERIALNUM][384] ;//所有串口下的充电桩SN，AllSerialSn[0]代表1号串口的充电桩sn
extern int EverySerialNum[SERIALNUM] ;//每个串口下充电桩个数，EverySerialNum[0]代表1号串口的充电桩数



/*
extern struct status **chargingstat1 ;//串口1充电桩状态
extern struct status **chargingstat2 ;//串口1充电桩状态
*/
#define MAX_CHARGING_NUM 32

extern unsigned char *ratelist;
extern unsigned char ratenum[3];

extern unsigned char blacknum[6] ;
extern unsigned char *blacklist;

//充电桩更新文件类型标志
#define FILEHEAD	0x00
#define FILEIN		0x01
#define FILELAST 	0x02
#define PILEUPDATEFILE "/pileupdatefile"
//桩可更新标识
#define UPDATEFREE 	0x01
#define UPDATING 		0x02
#define UPDATFINISH 	0x03


#define UPNOT 0x02//未上传
#define UPYES 0x01//已上传
//文件记录事件最长长度305字节
struct event_str{
	unsigned char e_time[4];//事件时间0x20150902
	unsigned int e_dataid;//通讯序号，用来区分不同的通讯帧
	unsigned char e_upflag;//是否已经上传标志
	unsigned char e_sn[4];//事件sn,采集器维护
	unsigned char e_type;//事件类型,可以判断出每个事件的长度
	unsigned char e_data[291];//事件内容,最长291字节
};


//读集中器参数
int readwg_parameter(const char *filename);
//读费率文件
int readrate_parameter(const char *filename);
//读交通卡黑名单
int read_blackcard(const char *filename);
//写配置文件
int writeconfig(const char *filename);
int writeconfigfile(const char *filename,unsigned char *buff,int len,char *writetype);
void delzero(unsigned char *buff,int len);
int Getserianuml_SN(char *sn,char *serialnum);
int Record_Event(char *filepath,struct event_str *p,int index);
int Record_Status(char *filepath,struct status*p,int index,int portnum);
int Set_EventFlag(char *filepath,unsigned int Dataid);
int Record_Event_Mem(char *src,struct event_str *p,int index);
int Set_EventFlag_Mem(unsigned char *src,unsigned int Dataid);
int Read_Status(char *filepath,struct status*p,int index,int portnum);
int Read_Nbytes(char *dst,int index,int num,char *filepath);


#endif

