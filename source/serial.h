#ifndef _serial_h
#define _serial_h
#include "linklist.h"

extern unsigned char PSN  ;
extern unsigned char PSN_0b  ;

#define STX_CP 0x02
#define ETX_CP 0x03
#define DLE_CP 0x10

//�Ƿ��·�ָ��
#define SEND 		0x00
#define NOSEND 	0x01

//���׮������
#define CMD_CLOCK_SYNC 		0x01	//ʱ��ͬ��ָ��
#define CMD_GET_DEV_PARA 	0x04	//��ȡ�豸����ָ��
#define CMD_SET_DEV_PARA 	0x05	//�����豸����ָ��
#define CMD_BEEP_CONT 		0x06	//����������ָ��
#define CMD_QUERY_STATE 		0x0B	//�豸״̬��ѯָ��
#define CMD_ELECTRICIZE 		0x0C	//�豸������ָ��
#define CMD_GET_TRAN_MSG 	0x0E	//�豸��ȡ���׿�Ƭ��Ϣָ��
#define CMD_PAYMENT_TRAN 	0x0F	//��Ƭ�ۿ��ָ��
#define CMD_EVENT_QUERY 		0x10	//�¼���ѯָ��
#define CMD_GET_DEVMSG	 	0x11	//��ȡ�豸��Ϣָ��
#define CMD_CLEAN_TRAN	 	0x1F	//�彻���¼�ָ��
#define CMD_DEV_RESET	 	0x20	//�����豸ָ��
#define CMD_GET_DEV_SN	 	0x20	//��ȡ�豸SN��Ϣָ��
#define CMD_SET_DEV_SN	 	0x20	//�����豸SN��Ϣָ��
#define CMD_UPDATECP			0x21//�����ļ�����ָ��

#define CTRLB_START 			0x01 //������������ǹ�жϺ�ʼ��ʽ��磬��ʱδ�����˳���
#define CTRLB_END 			0x02//��������������δ����
#define CTRLB_FINISH			0x03//��ɣ�������ɣ����ͷų��ǹ��
#define CTRLB_CARD			0x04//ѡ��ͨ��֧��
#define CTRLB_PAYMENTEND        0x44//add
#define CTRLB_ETC				0x05//ѡ��etc֧��
#define CTRLB_START_WAIT		0x81//�˳��������״̬�����ش���״̬
#define CTRLB_WILLPAY_CHARG	0x82//�˳�������״̬���س��״̬
#define CTRLB_CHANGEPAY		0x84//�˳�����״̬���ؽ������״̬���ȴ�ѡ��֧����ʽ
#define CTRLB_ALLSTOP		0xee//Ӧ��ֹͣ��ֱ��ֹͣ��磬���ͷų��ǹ
#define CTRLB_CHARGFAULT        0xf0//�������س����ɻ�����ϣ�ֹͣ��磬���������
#define CTRLB_CHARGKEYFAULT 0xf8//��翪�ع��ϣ�ֹͣ��磬���������


//���׮�ڲ��¼�
#define EVENT_READCARD   	0x01 //��Ƭˢ����Ϣ
#define EVENT_PAYMENT    	0x02 //��Ƭ�ۿ��
#define EVENT_CHARGCTL  	0x04//�������¼�

//���׮ָ��ִ�н��״̬
#define STATUS_SUCCESS 	0x00//ִ�гɹ�
#define STATUS_BUSY		0x01//�豸æ
#define STATUS_RW_F		0x02//���׶�д���豸����
#define STATUS_PSAM_F	0x03//����PSAM���ϣ���ͨ����ETC��
#define STATUS_RDB_F		0x04//����������
#define STATUS_CTL_F		0x05//�����ƿ��ع���
#define STATUS_SYS_F		0x07//ϵͳ����
#define STATUS_NOEVENT     0x21//���¼�
#define STATUS_WRONG       0x22//״̬����ȷ
#define STATUS_HWOK         0xfc//Ӳ��״̬��׼

//��Ƭ�ۿ�ױ�־
#define Deal_Success     0x00//���׳ɹ�
#define Deal_Fail		0x01//����δ�ɹ�
#define Deal_NotAck	0x02//���ײ�ȷ�ϣ���Ҫ����ˢ���ж�

#define BAUND 	57600
#define PARITY 	0	//0:None 1:Odd 2:Even 3:Mrk 4:Spc
#define DATABITS	8
#define STOPBITS 	1

#define RESPONSE0B 29//�豸״̬��ѯָ���֡����

#define ClockDeviation 30//���׮ʱ�����


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
