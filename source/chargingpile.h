#ifndef _chargingpile_h
#define _chargingpile_h

#define SUCCESS 	0x00
#define FAIL		0x01
#define TIMEDELAY 60//���ظ�������ʱ���
#define WGSEVER_OUTTIME 15//���ط�����ͨѶ��ʱĬ��10s
#define TIME_WRESTART 3600//������������һ��Сʱ�ż�������ʱ��


#define CURVERSION	"20160103"


#define DEALEVENTSFILE "/dealevent.txt"

//extern int chargingpilenum;
extern int restartnum;
struct servercfg{
	unsigned char ip[16];//��������ַ15 bytes
	unsigned char port[6];//�˿ں�5 bytes
	unsigned char time_upload[5];//״̬�ϴ����4 bytes
	unsigned char time_out[3];//ͨѶ��ʱʱ��2 bytes
};

struct deviceinfo{
	unsigned char SN[12];//���׮SN��12 bytes
	unsigned char Serialnum[2];//���׮���ں�2 bytes
};


#endif
