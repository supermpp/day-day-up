#ifndef _chargingpile_h
#define _chargingpile_h

#define SUCCESS 	0x00
#define FAIL		0x01
#define TIMEDELAY 60//网关跟服务器时间差
#define WGSEVER_OUTTIME 15//网关服务器通讯超时默认10s
#define TIME_WRESTART 3600//程序连续运行一个小时才记下重启时间


#define CURVERSION	"20160103"


#define DEALEVENTSFILE "/dealevent.txt"

//extern int chargingpilenum;
extern int restartnum;
struct servercfg{
	unsigned char ip[16];//服务器地址15 bytes
	unsigned char port[6];//端口号5 bytes
	unsigned char time_upload[5];//状态上传间隔4 bytes
	unsigned char time_out[3];//通讯超时时间2 bytes
};

struct deviceinfo{
	unsigned char SN[12];//充电桩SN号12 bytes
	unsigned char Serialnum[2];//充电桩串口号2 bytes
};


#endif
