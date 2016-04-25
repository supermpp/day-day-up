#ifndef _myprotocol_h
#define _myprotocol_h
#include "linklist.h"

#define STX 0xa1
#define ETX 0x55
#define  ENCRYPIF 0x00 //0x00 ������ 0x01����

//
#define SERVER_RECONNECT 	0x01
#define SERVER_CONNET		0x00

//���ط�����ͨѶ������
#define CMDID_FILEDOWNLOAD   0x01//�ļ��´�����
#define CMDID_PASSTHROUGH     0x02//����͸������
#define CMDID_STAT_UPDATE	0x03//״̬�ϱ�������
#define CMDID_CHARGCONTROL  0x04//�����̿�������
#define CMDID_DEAL_UPDATE 	0x05//�����¼��ϴ�ָ��
#define CMDID_SERVER_GATHER 0x06//�¼���������
#define CMDID_PROCESS_QUERY  0x07 //�����̲�ѯ
#define CMDID_HEARTTAG		0x08//������
 
//�����־
#define PAYSUCCESS 	0x00//֧���ɹ�
#define NOTENOUGH 	0x01//����
#define CARDMOVE   	0x02//֧�������п�Ƭ�ƶ�������ˢһ��
#define BLACKCARD 	0x03//��������Ƭ
#define DEALNOTACK     0x04//���ײ�ȷ��������ˢ�� 
#define DEALFAIL		0x05//���ײ��ɹ�

//�����̲�ѯ�����־
#define QUERY_SUCCESS 	0x00	 //��ѯ�ɹ� 
#define QUERY_UNUSED	 	0x01	//û�д��ڳ��״̬,����������Ч
#define QUERY_CONNECTFAIL 0x02//ͨѶ�ж�����������Ч

//�ļ��´����������
#define UPDATE_SUCCESS 		0x00//�ɹ�
#define UPDATE_FORMWRONG	0x85//�ļ���ʽ����ȷ
#define UPDATE_NOSUPPORT	0x86//��֧�ִ�����

//�ļ��´�--�ļ�����
#define F_TYPE_01 0x01//���������������ļ�
#define F_TYPE_02 0x02//�����ļ�
#define F_TYPE_03 0x03//���������ļ�
#define F_TYPE_04 0x04//��ͨ��������
#define F_TYPE_05 0x05//���׮���������ļ�

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

