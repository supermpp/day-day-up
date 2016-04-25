#ifndef _rwconfig_h
#define _rwconfig_h

 #include "linklist.h"
 
#define WGCONFIGFILE "/etc/wgconfig.conf"
#define RATEFILE "/etc/rate.conf"
#define BLACKCARDFILE "/etc/blackcard.conf"
#define WGINFOINI "/etc/wginfo.ini"
#define StatusFile "/etc/piplestatus.txt"
#define EVENTFILE "/root/eventlist.txt"

#define SERIALNUM 2//������



extern struct servercfg remoteserver;
extern int chargingpilenum;//���׮����
extern unsigned char serailnum_sn[896];//���г��׮sn�ʹ��ں�
extern int serial1chargnum;//����1���׮��
extern unsigned char serail1SN[384];//����1��SN��32*12=384

extern int serial2chargnum;//����2���׮��
extern unsigned char serail2SN[384];//����1��SN��32*12=384

extern unsigned char AllSerialSn[SERIALNUM][384] ;//���д����µĳ��׮SN��AllSerialSn[0]����1�Ŵ��ڵĳ��׮sn
extern int EverySerialNum[SERIALNUM] ;//ÿ�������³��׮������EverySerialNum[0]����1�Ŵ��ڵĳ��׮��



/*
extern struct status **chargingstat1 ;//����1���׮״̬
extern struct status **chargingstat2 ;//����1���׮״̬
*/
#define MAX_CHARGING_NUM 32

extern unsigned char *ratelist;
extern unsigned char ratenum[3];

extern unsigned char blacknum[6] ;
extern unsigned char *blacklist;

//���׮�����ļ����ͱ�־
#define FILEHEAD	0x00
#define FILEIN		0x01
#define FILELAST 	0x02
#define PILEUPDATEFILE "/pileupdatefile"
//׮�ɸ��±�ʶ
#define UPDATEFREE 	0x01
#define UPDATING 		0x02
#define UPDATFINISH 	0x03


#define UPNOT 0x02//δ�ϴ�
#define UPYES 0x01//���ϴ�
//�ļ���¼�¼������305�ֽ�
struct event_str{
	unsigned char e_time[4];//�¼�ʱ��0x20150902
	unsigned int e_dataid;//ͨѶ��ţ��������ֲ�ͬ��ͨѶ֡
	unsigned char e_upflag;//�Ƿ��Ѿ��ϴ���־
	unsigned char e_sn[4];//�¼�sn,�ɼ���ά��
	unsigned char e_type;//�¼�����,�����жϳ�ÿ���¼��ĳ���
	unsigned char e_data[291];//�¼�����,�291�ֽ�
};


//������������
int readwg_parameter(const char *filename);
//�������ļ�
int readrate_parameter(const char *filename);
//����ͨ��������
int read_blackcard(const char *filename);
//д�����ļ�
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

