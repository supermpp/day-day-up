#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "rwserial.h"
#include "serial.h"
#include "linklist.h"
#include "rwconfig.h"
#include "common.h"
#include "mytime.h"


unsigned char PSN = 0;//ָ��֡���к�
unsigned char PSN_0b = 0;//0bָ��֡���к�

//���CMDָ�����ݼ���ͷβ
int MGTR_UART_tx_format(unsigned char* cmd, unsigned int cmdlen, unsigned char* tx_buf)
{
	unsigned int tx_point = 0;
	unsigned char c = 0;
	unsigned char xor_num = 0;

	if(cmdlen == 0) return 0;
	//STX_CP
	tx_buf[tx_point++] = STX_CP;
	//Length
	c = (cmdlen /0x100) % 0x100;
	xor_num ^= c;
	if((c==STX_CP) || (c== ETX_CP) || (c==DLE_CP))
	{
		tx_buf[tx_point++] = DLE_CP;
		tx_buf[tx_point++] = c;
	}
	else
	{
		tx_buf[tx_point++] = c;
	}
	c = cmdlen % 0x100;
	xor_num ^= c;
	if((c==STX_CP) || (c== ETX_CP) || (c==DLE_CP))
	{
		tx_buf[tx_point++] = DLE_CP;
		tx_buf[tx_point++] = c;
	}
	else
	{
		tx_buf[tx_point++] = c;
	}
	//Data
	while(cmdlen)
	{
		if( (*cmd == STX_CP) || (*cmd == ETX_CP) || (*cmd ==DLE_CP))
		{
			tx_buf[tx_point++] = DLE_CP;
			tx_buf[tx_point++] = *cmd;
		}
		else
		{
			tx_buf[tx_point++] = *cmd;
		}
		xor_num ^= *cmd;
		cmd++;
		cmdlen--;
	}
	if((xor_num == STX_CP) || (xor_num == ETX_CP) || (xor_num == DLE_CP))
	{
		tx_buf[tx_point++] = DLE_CP;
		tx_buf[tx_point++] = xor_num;
	}
	else
	{
		tx_buf[tx_point++] = xor_num;
	}
	tx_buf[tx_point++] = ETX_CP;
	return tx_point;
}

//����׮����֡
int Charging_Frame(PCMD_DATA_S cmd,unsigned char *dstframe)
{
	unsigned char buff[300]={0};
	int len;
	if(cmd==NULL) return 0;
	if(cmd->CLS != 0x8A) return 0;
	if(cmd->LE >250) return 0;
	
	buff[0]=cmd->ADDR;
	buff[1]=cmd->CLS;
	buff[2]=cmd->PSN;
	buff[3]=cmd->PARM;
	buff[4]=cmd->LE;
	memcpy(buff+5,cmd->data,cmd->LE);
	
	len=MGTR_UART_tx_format(buff, buff[4]+5, dstframe);
	
	return len;
}

/*
5.15	�����豸���ļ����غ͸��²���
num:��������֡���к�
chargsn:���׮׮��
buff:���µ��ļ�����
buf_len:���µ��ļ����ݵĳ���
dstframe:������ɵ�����֡

int Frame_UpdateCP(char num,char *chargsn,char *buff,int buf_len,char *dstframe)
{
	int cmdlen = 0,len = 0;
	char buff[1100] = {0};
	buff[0] = 0x00;
	buff[1] = 0x8a;
	buff[2] = num;
	buff[3] = CMD_UPDATECP;
	if(buf_len == 34)
		buff[4] = 0x2e;
	else if(buf_len == 1034)
		buff[4] = 0x16;
	memcpy(buff+5,chargsn,12);
	memcpy(buff+17,buff,buf_len);
	cmdlen = (17+buf_len);
	
	 len = MGTR_UART_tx_format(buff,cmdlen,dstframe);

	 return len;
}
*/

/*
5.12	��ȡ�豸��Ϣָ��
num:��������֡���к�
chargsn:���׮׮��
dstframe:������ɵ�����֡
*/
int Frame_GetDevmsg(char num,char *chargsn,char *dstframe)
{
	int cmdlen = 0,len = 0;
	char buff[64] = {0};
	buff[0] = 0x00;
	buff[1] = 0x8a;
	buff[2] = num;
	buff[3] = CMD_GET_DEVMSG;
	buff[4] = 0x0c;
	memcpy(buff+5,chargsn,12);
	cmdlen = 17;
	
	 len = MGTR_UART_tx_format(buff,cmdlen,dstframe);

	 return len;
}
/*
�����ļ�����ָ��
num:��������֡���к�
data:�����ļ���ͷ�ļ�/���������ļ�
datalen:data����
chargsn:���׮׮��
dstframe:������ɵ�����֡
*/
int Frame_UpdateCPfile(char num,char *data,int datalen ,char *chargsn,char *dstframe)
{
	int cmdlen = 0,len = 0;
	char buff[1200] = {0};
	buff[0] = 0x00;
	buff[1] = 0x8a;
	buff[2] = num;
	buff[3] = CMD_UPDATECP;
	if(datalen == 34)
		buff[4] = 0x2e;
	else 
		buff[4] = 0x16;
	memcpy(buff+5,chargsn,12);
	memcpy(buff+17,data,datalen);
	cmdlen = 17+datalen;

	 len = MGTR_UART_tx_format(buff,cmdlen,dstframe);
	 return len;
}
/*
���ó��׮sn
*/
int Frame_Set_PipleSN(char num,char *sn,unsigned char *dstframe)
{
	int cmdlen = 0,len = 0;
	unsigned char buff[64] = {0};
	buff[0] = 0xFE;
	buff[1] = 0x8a;
	buff[2] = num;
	buff[3] = 0xF2;
	buff[4] = 0x0C;
	memcpy(buff+5,sn,12);
	cmdlen = 17;

	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);
	
	return len;
}

/*
ʱ��ͬ��ָ��0x01  (�㲥ָ���豸����Ӧ��)
PSN:ָ��֡���к�
dstframe:������һ֡����
cptime:���׮ʱ��ascii��ʽ
����ֵ:dstframe����
*/
int Frame_Clock_Sync(unsigned char myPSN,unsigned char *dstframe)
{
	unsigned char buff[16] = {0},time_str[15] = {0},time_bcd[7] = {0};
	int cmdlen = 0,len = 0;
	time_t t;
	struct tm *tm1;
	
	buff[0]=0xff;//ADDRƥ���ַ
	buff[1]=0x8a;//CLS���׮ָ��
	buff[2]=myPSN;//PSNָ��֡���к�
	buff[3]=CMD_CLOCK_SYNC;//PARM�豸��������ָ��
	buff[4]=0x08;//LC/LEָ�����ݳ���
	t = time(NULL);
	tm1 = localtime(&t);
	snprintf((char *)time_str,15,"%04d%02d%02d%02d%02d%02d",tm1->tm_year+1900,tm1->tm_mon+1,tm1->tm_mday,tm1->tm_hour,tm1->tm_min,tm1->tm_sec);
	StrtoBCD(time_str,14,time_bcd);
	memcpy(buff+5,time_bcd,sizeof(time_bcd));//��ǰʱ�䣺YYYYMMDDhhmmss
	if(tm1->tm_wday == 0)
		buff[12] = 0x07;
	else
		buff[12] = tm1->tm_wday;
	
	cmdlen = 13;
	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);
	return len;
}


/*
5.5	�����豸����ָ��0x05
PSN:ָ��֡���к�
SN:���׮SN
dstframe:������һ֡����
����ֵ:dstframe����
*/
int Frame_SetParameter(unsigned char PSN,unsigned char *SN,unsigned char *PVer,unsigned char *PData,unsigned char *dstframe)
{
	unsigned char buff[128] = {0};
	int cmdlen = 0,len = 0;
	
	buff[0]=0x00;//ADDRƥ���ַ
	buff[1]=0x8a;//CLS���׮ָ��
	buff[2]=PSN;//PSNָ��֡���к�
	buff[3]=CMD_SET_DEV_PARA;//PARM�豸��������ָ��
	buff[4]=0x50;//LC/LEָ�����ݳ���

	memcpy(buff+5,SN,12);//���׮SN
	memcpy(buff+17,PVer,4);//���ò����汾
	memcpy(buff+21,PData,64);//���ò�������
	cmdlen = 85;
	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);
	return len;
}

/*
������ָ��0x0c
PSN:ָ��֡���к�
SN:���׮SN
EventSN:�¼����
Ctrlbyte:������
dstframe:������һ֡����
����ֵ:dstframe����
*/
int Frame_ChargingControl(unsigned char myPSN,unsigned char *SN,unsigned char *EventSN,unsigned char Ctrlbyte,unsigned char *dstframe)
{
	unsigned char buff[128] = {0};
	int cmdlen = 0,len = 0;

	buff[0]=0x00;//ADDRƥ���ַ
	buff[1]=0x8a;//CLS���׮ָ��
	buff[2]=myPSN;//PSNָ��֡���к�
	buff[3]=CMD_ELECTRICIZE;//PARM�豸��������ָ��
	buff[4]=0x19;//LC/LEָ�����ݳ���

	memcpy(buff+5,SN,12);
	memcpy(buff+17,EventSN,12);
	buff[29] = Ctrlbyte;
	cmdlen = 30;

	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);
	
	return len;
}

/*
��ȡ�����׵Ľ�ͨ����ETC����Ϣ0x0e
PSN:ָ��֡���к�
SN:���׮SN
EventSN:�¼����
Ctrlbyte:������
dstframe:������һ֡����
����ֵ:dstframe����
*/
int Frame_readcard(unsigned char PSN,unsigned char *SN,unsigned char *EventSN,unsigned char *dstframe)
{
	unsigned char buff[128] = {0};
	int cmdlen = 0,len = 0;

	buff[0]=0x00;//ADDRƥ���ַ
	buff[1]=0x8a;//CLS���׮ָ��
	buff[2]=PSN;//PSNָ��֡���к�
	buff[3]=CMD_GET_TRAN_MSG;//PARM�豸��������ָ��
	buff[4]=0x18;//LC/LEָ�����ݳ���

	memcpy(buff+5,SN,12);
	memcpy(buff+17,EventSN,12);
	cmdlen = 29;
	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);

	return len;
}


/*
��Ƭ�ۿ��ָ��0x0f
PSN:ָ��֡���к�
SN:���׮SN
EventSN:�¼����
Ctrlbyte:������
dstframe:������һ֡����
����ֵ:dstframe����
*/
int Frame_payment(unsigned char myPSN,struct status *charg,unsigned char *dstframe)
{
	unsigned char buff[128] = {0},RFU[8] = {0};
	int cmdlen = 0,len = 0,offset = 0,sum = 0;

	buff[0]=0x00;//ADDRƥ���ַ
	buff[1]=0x8a;//CLS���׮ָ��
	buff[2]=myPSN;//PSNָ��֡���к�
	buff[3]=CMD_PAYMENT_TRAN;//��Ƭ�ۿ��ָ��
	buff[4]=0x28;//LC/LEָ�����ݳ���
	offset = 5;

	memcpy(buff+offset,charg->SN,12);//���׮SN
	offset += 12;
	memcpy(buff+offset,charg->EventSN,12);//�¼����
	offset += 12;
	memcpy(buff+offset,charg->CardID,sizeof(charg->CardID));//�����׿�ID
	offset += sizeof(charg->CardID);
	//���׽�Hex��
	if(charg->Flag1)
	{	
		print_log(LOG_DEBUG,"Frame_payment   charg->MoneyCharg:%d\n",charg->MoneyCharg);
		memcpy(buff+offset,&(charg->MoneyCharg),sizeof(charg->MoneyCharg));
		offset += sizeof(charg->MoneyCharg);
	}
	else
	{
			sum = charg->MoneyCharg;
			print_log(LOG_DEBUG,"��Ƭ�ۿ���%d\n",sum);
			//Rev32InByte(&sum);
			memcpy(buff+offset,&sum,sizeof(sum));//Ӧ�ս��(���ؽ���)
			offset += sizeof(sum);
	}
	
	memcpy(buff+offset,RFU,sizeof(RFU));//RFUȫ��
	offset += sizeof(RFU);

	cmdlen = 45;
	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);

	return len;
}


/*
�����¼����ָ��0x1f
PSN:ָ��֡���к�
SN:���׮SN
EventSN:�¼����
Ctrlbyte:������
dstframe:������һ֡����
����ֵ:dstframe����
*/
int Frame_CleanEvent(unsigned char myPSN,struct status *charg,unsigned char CEvent,unsigned char *dstframe)
{
	unsigned char buff[128] = {0};
	int cmdlen = 0,len = 0,offset = 0;

	buff[0]=0x00;//ADDRƥ���ַ
	buff[1]=0x8a;//CLS���׮ָ��
	buff[2]=myPSN;//PSNָ��֡���к�
	buff[3]=CMD_CLEAN_TRAN;//�����¼����ָ��
	buff[4]=0x0d;//LC/LEָ�����ݳ���
	offset = 5;

	memcpy(buff+offset,charg->SN,12);//���׮SN
	offset += 12;
	buff[offset] = CEvent;
	offset += 1;

	cmdlen = offset;
	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);

	return len;
}




/*
�豸״̬��ѯָ��0x0b
PSN:ָ��֡���к�
SN:���׮SN
dstframe:������һ֡����
����ֵ:dstframe����
*/
int Create_Frame_StatusRequest(unsigned char myPSN,unsigned char *SN,unsigned char *dstframe)
{
	unsigned char buff[128] = {0};
	int cmdlen = 0,len = 0;

	buff[0]=0x00;//ADDRƥ���ַ
	buff[1]=0x8a;//CLS���׮ָ��
	buff[2]=myPSN;//PSNָ��֡���к�
	buff[3]=CMD_QUERY_STATE;//PARM�豸��������ָ��
	buff[4]=0x0c;//LC/LEָ�����ݳ���
	memcpy(buff+5,SN,12);
	cmdlen = 17;

	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);
	return len;
}

/*
�¼���ѯָ��0x10
PSN:ָ��֡���к�
SN:���׮SN
type:��ѯ�����¼�����
dstframe:������һ֡����
����ֵ:dstframe����
*/
int Frame_EventQuery(unsigned char myPSN,unsigned char *SN,unsigned char type,unsigned char *dstframe)
{
	unsigned char buff[128] = {0};
	int cmdlen = 0,len = 0;

	buff[0]=0x00;//ADDRƥ���ַ
	buff[1]=0x8a;//CLS���׮ָ��
	buff[2]=myPSN;//PSNָ��֡���к�
	buff[3]=CMD_EVENT_QUERY;//PARM�豸��������ָ��
	buff[4]=0x0d;//LC/LEָ�����ݳ���
	memcpy(buff+5,SN,12);
	buff[17] = type;
	cmdlen = 18;

	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);
	return len;
}


/*
�豸״̬��ѯָ��0x0b��֡����
PSN:ָ��֡���к�
SN:���׮SN
dstframe:������һ֡����
����ֵ:
	�����շ����ݷ���dstframe����>0
	�ղ����������ݷ���-2,��ʱͨѶ״̬��ͨ
	֡��ʽ����ȷ����-5
	ָ��ִ�в��ɹ��豸æ����0

*/
int Unpack_Frame_StatusRequest(unsigned char *srcframe,int srclen,struct status *stat,unsigned char *dstframe,unsigned char xpsn)
{
	int ret = 0,i = 0;
	char buff_dn[5] = {0},buff[4] = {0};
	for(i = 0;i < 4;i++)
		buff[i] = 0x00;
	
	ret = MGTR_UART_rx_format(srcframe,srclen,dstframe,xpsn);
	
	print_log(LOG_DEBUG,"ret:%d  dstframe[5]:%02x\n",ret,dstframe[5]);
	
	if(ret > 0){
		if(dstframe[5] == 0x00){//ָ��ִ�гɹ�
			stat->CStatus  = dstframe[7];
			stat->CEvent= dstframe[8];
			memcpy(stat->DT,dstframe+9,sizeof(stat->DT));
			memcpy(buff_dn,dstframe+16,4);
			memcpy(stat->ENERGY,dstframe+16,sizeof(stat->ENERGY));
			stat->Hw_Status = dstframe[20];
			//stat->Hw_Status = 0xFC;
			memcpy(stat->RFU,dstframe+21,sizeof(stat->RFU));
		}
		else//ָ��ִ��֮���豸æ
			return 0;
		//״̬æ�ͰѴ������ݶ���
	}
	else if(ret == -2)//�ղ�������(��ʱ)
		return -2;
	else if(ret == -5)//֡��ʽ����ȷ
		return -5;
	return ret;
}

//��ȡδ֪���ȵĴ�������
int Read_SerialData(int fd,unsigned char *data)
{
	int len = 0,ret = 0;
	
	for( ; ;){
		if(len == 0){
			len = serial_read(fd,data,1,150);
			if(ret == 1)
				len++;
			else 
				break;
		}
		else{
			ret = serial_read(fd,data+len,1,30);
			if(ret == 1)
				len++;
			else 
				break;
		}
	}

	return len;
}
/*
����׮����֡
ȥ��STX  XOR  ETX
-2�ղ������ݣ���ʱ
-5��ʽ����ȷ
>0���յ����ݣ���Ҫ�б�ִ��״̬λ�Ƿ�ɹ�
*/

int MGTR_UART_rx_format( unsigned char* rcv, unsigned int rcvlen, unsigned char* rspbuf,unsigned char xpsn)
{
	unsigned int rx_point = 0;
	unsigned int rsplen = 0;
	unsigned int xor_num = 0;

	if( (rcvlen == 0) || ( rcv == NULL) || (rspbuf == NULL)) return -2;

	if(rcv[rx_point++] != STX_CP) return -5;
	rcvlen--;
	if(rcvlen < 4) return -5;
	rcvlen--;
	while(rcvlen)
	{
		if(rcv[rx_point] == DLE_CP)
		{
			rx_point++;
			rcvlen--;
			if(rcvlen == 0) return -5;
			if( (rcv[rx_point] != STX_CP) && (rcv[rx_point] != ETX_CP) && (rcv[rx_point] != DLE_CP)) 
				return -5;
			xor_num ^= rcv[rx_point];
			rspbuf[rsplen++] = rcv[rx_point++];
			rcvlen--;
		}
		else if( (rcv[rx_point] == STX_CP) || (rcv[rx_point] == ETX_CP))
		{
			return -5;
		}
		else
		{
			xor_num ^=rcv[rx_point];
			rspbuf[rsplen++] = rcv[rx_point++];
			rcvlen--;
		}
	}
	if(xor_num != 0) return -5;
	if(rcv[rx_point] != ETX_CP)
	{
		return -5;
	}
	if(rspbuf[4] != xpsn)
	{
		print_log(LOG_DEBUG,"framenum error [%02x] [%02x]\n",rspbuf[4],xpsn);
		sleep(1);
		return -5;
	}
	return (rsplen-1);
}

