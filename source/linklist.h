
#ifndef _linklist_h
#define _linklist_h


#include <stdio.h>
#include <pthread.h>


#define CONNECT 0x00
#define UNCONNECT 0x01

struct node{
	unsigned char cmdid;//cmdid Э�������(���cmdid==0x04�������һ���¼�)
	unsigned char eventtype;//�¼����ͱ�־
	unsigned int DATAID;//DATAID ͨѶ��ţ��������ֲ�ͬ��ͨѶ֡
	unsigned int T_SN;//T_SN �ɼ������
	unsigned char SN[12];//SN���׮���
	unsigned char Serialnum;//���ں�,cmdid=0x02���õ�,����ͨ���ɼ����ϵ��ĸ�����ת��ֻ�Դ洢ͨ������ת�������ݱ�����Ч
	unsigned short Serialtimeout;//���ڳ�ʱʱ��,cmdid=0x02���õ�,100ms�Ϻ��ʣ�����ֵΪ0x00ʱ��������׮����Ӧ�����ݣ�����㲥����
	unsigned char controlnum;//����ָ�������cmdid=0x04���õ�
	unsigned char Charg_SN[12];//������SN ,ͬ���׮���¼����,cmdid=0x04���õ�
	unsigned int Wgevent_SN;//�¼�SN�ɲɼ���ά���Ľ������,ÿ���¼�������ͬ

	unsigned char frame_psn;
	unsigned char flag1;//Stime SDn��Ч��ʾ0x01��Ч
	unsigned char Stime[7];//���׮��ʼ���ʱ��----0x04������·�������
	unsigned char  SDn[4];//��ʼ���ʱ�ĵ���----0x04������·�������
	unsigned char flag2;//Pay ��Ч��ʾ0x01��Ч
	unsigned int Pay;//���ѽ��----0x04������·�������
	int DATA_LEN;//len of DATA DATA���� 
	
	char DATA[380];//DATA ----��ͬcmdid��Ӧ��ͬ�����ݣ�Ҳ�����Ǵ�������֡
	int sendtime;//���ķ���ʱ�䣬���ڳ�ʱ�ط�
	struct node *next;
};


//���׮״̬
struct status{
	//���׮���ص��ֶ�
	unsigned char SN[12];//SN���׮���
	unsigned char CStatus;//���׮��ǰ״̬
	unsigned char CEvent;//���׮�ڲ��¼�״̬��λ���� 1-�У�0-��B0 - ��Ƭˢ����ϢB1 - ��Ƭ�ۿ��B2 - �������¼�
	unsigned char EventSN[12];//���׮�¼����,ͬstruct node���Charg_SN
	unsigned char DT[7];//��ǰʱ�䣺YYYYMMDDhhmmss
	unsigned char ENERGY[4];//���ǰ�ܵ��ܣ�XXXXXX.XX ��λkWh
	char RFU[11];//����
	//char FlagCost;//ʵʱ������Ч��־,connect��־�����ͨ����ʵʱ������Ч
	//unsigned int TimeCharg;//�������ʱ�䵥λ�� ,=(DT- SCT)
	
	//����
	unsigned char connectfailn;//new add ʧ�ܴ���,����������10�ε�ʱ�� connect����Ϊʧ��
	unsigned char connect;//ͨѶ״̬0x00:ͨ0x01:��ͨ
	unsigned char CFlg;//�������¼�(��ѯ�¼���ʱ��ֵ)
	unsigned char Flag1;//0x01��Ч���ڱ�ʾMoneyCharg�Ƿ���Ч,���������������ص��շѽ����Ч���ô˽�����,��Ч����STARTENERGY ENDENERGY����
	unsigned int MoneyCharg;//����λ�� ,(ENDENERGY - STARTENERGY)*rate
	unsigned char SCT[7];//���׮��ʼ���ʱ��
	unsigned char STARTENERGY[4];//��ʼ���ʱ�ĵ������ڳ����ý���
	unsigned char ENDENERGY[4];//�������ʱ�ĵ������ڳ����ý���
	unsigned char Hw_Status;//Ӳ��״̬
	unsigned char FailPay;//�ۿ�ʧ�ܱ�־�������ط���һ�οۿ�ָ��
	unsigned char CardID[4];// 4�ֽ������ţ������׿�ID,ͨ��ѯ��ָ���ȡ
	unsigned char CardSN[10];// 10�ֽڿ�Ƭ���к�
	unsigned char Yue[4];//��Ƭ���
	unsigned char CPFWW[2];//���׮����汾��
	char Flag_headfile;//�·�ͷ�ļ���־��0���·���1�Ѿ��·����ˡ�
	char Flag_lastfile;//�·������ļ����һ����־��0����û�·������һ����1�����·������һ����
	char UpStatus;//0x01�ɸ���0x02������
	char FLAG_CMD11;//��ȡ�豸��Ϣָ��ı�־00δ��ѯ��(��Ҫ��ѯ)0x01�Ѿ���ѯ����
	//unsigned char carddata[241];//�洢ˢ����ۿ���Ϣ
};


extern struct node *head_tcpsendlist;
extern struct node *head_serialsendlist;
extern struct node *head_tcphavesendlist;
extern struct node *head_eventlist_old;

//new
/*ÿһ���������Լ��Ĵ�����������*/
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


