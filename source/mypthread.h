#ifndef _mypthread_h
#define _mypthread_h

extern unsigned int wgid;
extern unsigned  int EventIndex;
extern unsigned int WG_DATAID;

extern char *eventbuf;
extern pthread_mutex_t eventbuf_lock;

extern unsigned char FLAG_GATHER;//�¼����������־
extern  unsigned char Start_Time[4];
extern unsigned char End_Time[4];//�¼�������ʼ��������

extern char CPversion[2];//���׮�汾��
extern char Flag_Recvheadfile;
extern char Flag_Recvlastfile;
extern unsigned short count_updatefilenum,num_updatefile;

//extern struct status **chargingstat;
 //�¼���¼��������
#define MAXEVENT 40000
#define SAVESTATTIME 3

#define HeartTime 90

struct serial{
	int portnum;
	char serv_reconnect;
};




void *Mainprocess(void *arg);

void *Socketprocess(void *arg);

void *Serialprocess(void *arg);

#endif 

