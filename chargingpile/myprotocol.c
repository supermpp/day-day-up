#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rc4.h"
#include "myprotocol.h"
#include "common.h"
#include "serial.h"
#include "linklist.h"
#include "rwconfig.h"
#include "ini-file.h"
#include "mytime.h"
#include "chargingpile.h"
#include "mypthread.h"
#include "linklist.h"
#include "myprotocol.h"
#include "rwserial.h"


unsigned int EVENTSN = 0;//�¼�SN�ɼ���ά���Ľ������ÿ���¼���ֻͬ���¼��ϴ��������õ�


void CharToString(void *d,const void *s,int len)
{
	memcpy(d,s,len);
	*(((char *)d)+len)='\0';
	return;
}

/*
card_id:4�ֽ�������
card_num:10�ֽ�ascii
*/
void get_card_outnum(unsigned char *card_id,char *card_num)
{
	char tempnum[12];
	unsigned int cardNum;
	char *pCard;
	int i,num,tmp;
	
	memset(tempnum,0,12);
	
	pCard = (char *)&cardNum;
	pCard[0] = card_id[3];
	pCard[1] = card_id[2];
	pCard[2] = card_id[1];
	pCard[3] = card_id[0];

	sprintf(tempnum,"%02d",cardNum%100);
	cardNum/=100;
	sprintf(tempnum+2,"%02d",cardNum%100);
	cardNum/=100;
	sprintf(tempnum+4,"%02d",cardNum%100);
	cardNum/=100;
	sprintf(tempnum+6,"%02d",cardNum%100);
	cardNum/=100;
	sprintf(tempnum+8,"%02d",cardNum%100);

	for(i=0,num=0;i<10;i++)
	{
		if (i%2==0) num+=tempnum[i]-'0';
		else
		{
			tmp=(tempnum[i]-'0')*2;
			num+=(tmp/10);
			num+=(tmp%10);
		}
	}
	num=num%10;
	if (num!=0) num=10-num;

	card_num[0]=num+'0';
	CharToString(card_num,tempnum,10);
	return ;
}


unsigned char XOR_SUM(const unsigned char *src,int len)
{
	unsigned char xorsum = 0;
	int i = 0; 
	for(i = 0;i < len;i++)
		xorsum ^= src[i];
	return xorsum;
}
unsigned int FourbytesToInt(const unsigned char *buff)
{
	unsigned int ret = 0;
	ret = (buff[0]<<24) + (buff[1]<<16)+(buff[2]<<8)+buff[3];
	return ret;
}
int AnalyzePacket(const unsigned char *src,int len,unsigned char *cmdid,unsigned int *dataid,unsigned int *t_sn,unsigned char *data)
{
	int datalen = 0;
	unsigned char SAFEMODE = 0x01;
	//unsigned char tmp[512] = {0};

	unsigned char *p = NULL;				
	datalen = len - 15;
	memcpy(data,src+13,datalen);
	if( (0x01&src[3]) == SAFEMODE ){//data���ּ��ܱ���
		rc4DE((char *)data,datalen);//����data
	}
	p = (unsigned char *)src+3;
	//check XOR
	if(src[len-2] != XOR_SUM(p,len-5)){
		printf("AnalyzePacket XOR error\n");
		return -1;
	}
	//check ETX
	if(src[len-1] != ETX){
		printf("AnalyzePacket ETX error\n");
		return -1;
	}
	//CmdID
	*cmdid = src[4];
	//DATAID
	memcpy(dataid,src+5,4);
	Rev32InByte(dataid);
	//T_SN
	memcpy(t_sn,src+9,4);
	Rev32InByte(t_sn);
	return datalen;
}

int  Resolve_Data(unsigned char *src,int len)
{
	return 0;	
}

/*
   ���������ط�����ͨ�ű���
   */
int Response_Frame(unsigned char *frame,unsigned char cmdid,unsigned int DATAID,unsigned int T_SN,unsigned char *DATA,int DATA_LEN)
{
	unsigned char tmp[1500] = {0};
	unsigned int swap = 0;
	unsigned char res_cmdid = 0x00;
	//STX
	frame[0] = STX;
	//Length ,include fun cmdid dataid t_sn data
	frame[1] = ((10+DATA_LEN)>>8)&0xff;
	frame[2] = (10+DATA_LEN)&0xff;
	//fun 0x00������0x01����
	frame[3] = ENCRYPIF;
	//cmdid
	if((cmdid == CMDID_STAT_UPDATE) ||(cmdid == CMDID_DEAL_UPDATE) ||(cmdid == CMDID_HEARTTAG))
		res_cmdid = cmdid;
	else 
		res_cmdid = (cmdid + 0x80);
	frame[4] = res_cmdid;
	//DATAID
	swap = DATAID;
	Rev32InByte(&swap);
	memcpy(frame+5,&swap,sizeof(swap));
	//T_SN
	swap = T_SN;
	Rev32InByte(&swap);
	memcpy(frame+9,&swap,sizeof(T_SN));
	//DATA
	if(frame[3] == 0x01){
		rc4DE((char *)DATA, DATA_LEN);
	}
	memcpy(frame+13,DATA,DATA_LEN);

	memcpy(tmp,frame+3,10+DATA_LEN);
	//XOR
	frame[DATA_LEN + 13] = XOR_SUM(tmp,10+DATA_LEN);
	//ETX
	frame[DATA_LEN + 14] = ETX;

	return (DATA_LEN + 15);
}

/*
   ���¼��ϱ����ĵ�data����
carddata:ˢ������Ƭ�ۿ����ݶ���
flag:����Ƿ��㹻�ۿ0����1��
money:�ۿ���
����ֵ:
>0 ������н����¼��ϴ�
==0 ������㹻,�����·��ۿ�ָ��

*/
int Create_Data_EventUp(unsigned char *dstbuf,struct status *src,unsigned char *carddata,int *flag,unsigned char *money)
{
	int offset = 0,rate = 0,i = 0;
	int rate1 = 0,rate2 = 0,rate3 = 0;
	unsigned int tmp = 0,sum = 0,yue = 0;
	unsigned char num[12] = {0},startdn[9] = {0},cardsn[10] = {0},cardnum[4] = {0},FH[240] = {0},dn_str[9] = {0};
	unsigned char stime_str[14] = {0},etime_str[14] = {0};
	unsigned int stime_int =0,etime_int = 0;
	unsigned char balance[4] = {0};
	unsigned int swap = 0;

	dstbuf[0] = src->CFlg;//�¼����� 0x01���������
	offset += 1;
	memcpy(dstbuf+offset,src->SN,sizeof(src->SN));//���׮SN
	offset += sizeof(src->SN);
	memcpy(dstbuf+offset,src->EventSN,sizeof(src->EventSN));//������SN
	offset += sizeof(src->EventSN);
	swap = EVENTSN;
	Rev32InByte(&swap);
	memcpy(dstbuf+offset,&swap,sizeof(swap));//�¼�SN�ɲɼ���ά���Ľ������,ÿ���¼�������ͬ
	offset += 4;
	//EVENTSN++;
	memcpy(dstbuf+offset,src->DT,sizeof(src->DT));//ʱ��BCD�����ʽ��YYYYMMDDHHMMSS���ɳ��׮����
	offset += sizeof(src->DT);
	print_log(LOG_DEBUG,"Create_Data_EventUp  CFlg:%02x\n",src->CFlg);
	if(src->CFlg == CTRLB_START)
	{
		//���ǰ�ܵ��ܣ�int��λ0.01kWh
		BCDtoStr(src->ENERGY,sizeof(src->ENERGY), dn_str);
		swap = atoi(dn_str);
		print_log(LOG_DEBUG,"-----------------------------------------------------------dn_str:%d\n",swap);
		Rev32InByte(&swap);
		memcpy(dstbuf+offset,&swap,sizeof(swap));
		offset += sizeof(src->ENERGY);
	}
	else if(src->CFlg == CTRLB_END)
	{
		BCDtoStr(src->ENERGY,sizeof(src->ENERGY), dn_str);
		swap = atoi(dn_str);
		Rev32InByte(&swap);
		memcpy(dstbuf+offset,&swap,sizeof(src->ENERGY));//����(���ǰ�ܵ��ܣ�XXXXXX.XX ��λkWh)
		offset += sizeof(src->ENERGY);

		rate = Find_Rate(src->DT,sizeof(src->DT),atoi(ratenum),num,&rate1,&rate2,&rate3);
		print_log(LOG_DEBUG,"rate:%d\nnum:%12s\n",rate,num);
		if(rate > 0){
			memcpy(dstbuf+offset,num,sizeof(num));//���ʱ��
		}
		else{
			memcpy(dstbuf+offset,"000000000000",sizeof(num));//δ���ҵ����ʱ������Ϊȫ0
		}
		offset += 12;

		BCDtoStr(src->STARTENERGY,4,startdn);
		if(atoi(dn_str) < atoi(startdn))//new ���ܳ������ʾ��
		{
			swap =  ( atoi(dn_str) +1000000 - atoi(startdn) );
		}
		else
		{
			swap = (atoi(dn_str) - atoi(startdn));
		}
		/**************************************/
		//���Բ��Խ�ͨ�������������ĵ���100��
		//swap = 10000;
		/**************************************/
	
		Rev32InByte(&swap);
		memcpy(dstbuf+offset,&swap,sizeof(swap));//ʹ�õ���
		offset += sizeof(tmp);

		//�յ�������ָ��ÿ�ζ����½�����
		BCDtoStr(src->STARTENERGY,4,startdn);
		BCDtoStr(src->DT,7, etime_str);
		BCDtoStr(src->SCT,7, stime_str);
		etime_int = time14ToInt(etime_str);
		stime_int = time14ToInt(stime_str);

		//new
		Rev32InByte(&swap);
		sum = (swap*(rate1+rate3)/100+rate2);

		/**************************************/
		//���Խ�ͨ�������������ĵ���100��
		//sum = 10000*(rate1+rate3)/100+rate2;
		/**************************************/
		
		swap = sum;
		Rev32InByte(&swap);
		print_log(LOG_DEBUG,"������-----������:%d  ��ǰ����:%s ���ʱ��%s ʹ�õ���:%d(0.01kwh)\n",sum,dn_str,num,(atoi(dn_str) - atoi(startdn)));
		memcpy(dstbuf+offset,&swap,sizeof(sum));//Ӧ�ս��(���ؽ���)
		src->MoneyCharg = sum;
		offset += sizeof(sum);
	}
	else if(src->CFlg == CTRLB_CARD)
	{
		print_log(LOG_DEBUG,"--------------Create_Data_EventUp-----------CTRLB_CARD\n");
		/*�жϿ�Ƭ������*/
		//memcpy(cardsn,carddata+5,sizeof(cardsn));
		memcpy(cardnum,carddata+11,sizeof(cardnum));//������
		get_card_outnum(cardnum,cardsn);//������ת�����
		for(i = 0;i < atoi(blacknum);i++)
		{
			if(memcmp(cardsn,blacklist+(i*12),sizeof(cardsn)) == 0)
			{
				print_log(LOG_DEBUG,"�ÿ����ں�������Ƭ\n");
				dstbuf[offset] = BLACKCARD;//��������Ƭ 
				offset += 1;
				memcpy(dstbuf+offset,cardsn,sizeof(cardsn));
				offset += sizeof(cardsn);
				memcpy(dstbuf+offset,"0000",4);
				offset += 4;
				memcpy(dstbuf+offset,FH,sizeof(FH));
				offset += sizeof(FH);
				return offset;
			}
		}
		/*�п�����Ƿ��㹻*/
		//print_hexoflen(carddata, int len);
		memcpy(balance,carddata+20,sizeof(balance));
		print_log(LOG_DEBUG,"����:%s\nˢ����Ϣ:\nbalance:",cardsn);
		print_hexoflen(balance,4);
		memcpy(&yue,balance,4);
		//Rev32InByte(&yue);
		print_log(LOG_DEBUG,"���:%d\n",yue);
		Rev32InByte(balance);

		if(src->Flag1 == 0x01)//������·��ĳ������Ч
		{
			print_log(LOG_DEBUG,"ѡ��ͨ�����������:%d\n",src->MoneyCharg);
			//if(memcmp(&yue,&(src->MoneyCharg),sizeof(src->MoneyCharg)) >= 0)
			if(yue >= src->MoneyCharg)
			{
				print_log(LOG_DEBUG,"����㹻\n");
				swap = src->MoneyCharg;
				Rev32InByte(&swap);
				memcpy(money,&swap,sizeof(swap));
				//memcpy(money,&(src->MoneyCharg),sizeof(src->MoneyCharg));
				//����㹻����0
				*flag = 1;
				return 0;
			}
			else
			{
				//���㹻
				print_log(LOG_DEBUG,"����\n");	
				dstbuf[offset] = NOTENOUGH;//��־����
				offset += 1;
				memcpy(dstbuf+offset,cardsn,sizeof(cardsn));//����
				offset += sizeof(cardsn);
				memcpy(dstbuf+offset,carddata+11,4);//���
				offset += 4;
				memcpy(dstbuf+offset,FH,sizeof(FH));
				offset += sizeof(FH);
			}
		}
		else//������·��ĳ������Ч������㣬ֱ���ô�����ʱ��Ľ��
		{
			sum = src->MoneyCharg;
			print_log(LOG_DEBUG,"ѡ��ͨ������---------------------������:%d\n",sum);
			swap = sum;
			Rev32InByte(&swap);
			memcpy(money,&swap,sizeof(swap));//Ӧ�ս��(���ؽ���)
			//if(memcmp(&yue,&sum,sizeof(sum)) >= 0)
			if(yue >= sum)
			{
				//����㹻����0
				print_log(LOG_DEBUG,"--����㹻--\n");
				*flag = 1;
				return 0;
			}
			else
			{
				print_log(LOG_DEBUG,"--����--\n");	
				dstbuf[offset] = NOTENOUGH;//��־����
				offset += 1;
				memcpy(dstbuf+offset,cardsn,sizeof(cardsn));//����
				offset += sizeof(cardsn);
				//memcpy(dstbuf+offset,carddata+11,4);//���
				Rev32InByte(&yue);
				memcpy(dstbuf+offset,&yue,4);//���
				offset += 4;
				memcpy(dstbuf+offset,FH,sizeof(FH));
				offset += sizeof(FH);
			}
		}
	}
	else if(src->CFlg == CTRLB_PAYMENTEND )
	{
		/*ˢ�����֮��Ҫ��ˢ���¼��ϱ�*/
		dstbuf[0] = CTRLB_CARD;
		if(carddata[0] == Deal_Success)
		{
			dstbuf[offset] = PAYSUCCESS;//֧���ɹ�
			print_log(LOG_DEBUG,"*********************�ۿ�ɹ�*********************\n");

		}
		else if(carddata[0] == Deal_Fail)//����δ�ɹ�
		{
			dstbuf[offset] = DEALFAIL;
			print_log(LOG_DEBUG,"*********************�ۿ�ʧ��*********************\n");

		}
		else if(carddata[0] == Deal_NotAck)//���ײ�ȷ��
		{
			dstbuf[offset] = DEALNOTACK;
			print_log(LOG_DEBUG,"*********************�ۿȷ��*********************\n");

			return -1;//����-1������ۿȷ�ϣ������������·��ۿ�ָ��
		}
		offset += 1;
		memcpy(dstbuf+offset,src->CardSN,sizeof(src->CardSN));
		offset += sizeof(src->CardSN);
		memcpy(&swap,src->Yue,4);
		Rev32InByte(&swap);
		//Rev32InByte(src->Yue);
		memcpy(dstbuf+offset,&swap,sizeof(src->Yue));
		offset += sizeof(src->Yue);
		memcpy(dstbuf+offset,carddata+1,240);
		offset += 240;
	}
	else if( (src->CFlg == CTRLB_CHARGFAULT)||(src->CFlg == CTRLB_CHARGKEYFAULT) )
	{
		print_log(LOG_DEBUG,"*********************�������س����ɻ������*********************\n");
		BCDtoStr(src->ENERGY,sizeof(src->ENERGY), dn_str);
		swap = atoi(dn_str);
		Rev32InByte(&swap);
		memcpy(dstbuf+offset,&swap,sizeof(src->ENERGY));//����(���ǰ�ܵ��ܣ�XXXXXX.XX ��λkWh)
		offset += sizeof(src->ENERGY);

		rate = Find_Rate(src->DT,sizeof(src->DT),atoi(ratenum),num,&rate1,&rate2,&rate3);
		print_log(LOG_DEBUG,"rate:%d\nnum:%12s\n",rate,num);
		if(rate > 0){
			memcpy(dstbuf+offset,num,sizeof(num));//���ʱ��
		}
		else{
			memcpy(dstbuf+offset,"000000000000",sizeof(num));//δ���ҵ����ʱ������Ϊȫ0
		}
		offset += 12;

		BCDtoStr(src->STARTENERGY,4,startdn);
		if(atoi(dn_str) < atoi(startdn))
		{
			swap = (atoi(dn_str) + 1000000 - atoi(startdn));
		}
		else
		{
			swap = (atoi(dn_str) - atoi(startdn));
		}
		Rev32InByte(&swap);
		memcpy(dstbuf+offset,&swap,sizeof(swap));//ʹ�õ���
		offset += sizeof(tmp);

		//�յ�������ָ��ÿ�ζ����½�����
		BCDtoStr(src->STARTENERGY,4,startdn);
		BCDtoStr(src->DT,7, etime_str);
		BCDtoStr(src->SCT,7, stime_str);
		etime_int = time14ToInt(etime_str);
		stime_int = time14ToInt(stime_str);

		//new
		Rev32InByte(&swap);
		sum = (swap*(rate1+rate3)/100+rate2) ;
		swap = sum;
		Rev32InByte(&swap);
		print_log(LOG_DEBUG,"--������:%d  ��ǰ����:%s ���ʱ��%s ʹ�õ���:%d(0.01kwh)\n",sum,dn_str,num,(atoi(dn_str) - atoi(startdn)));
		memcpy(dstbuf+offset,&swap,sizeof(sum));//Ӧ�ս��(���ؽ���)
		src->MoneyCharg = sum;
		offset += sizeof(sum);
	}
	else
	{
		/*������*/
		return offset;
	}
	return offset;
}




/*
   ���豸״̬�ϱ�����0x03��Data����
dataid:ͨѶ���
t_sn:�ɼ������
data:Data����
datalen:Data���ֳ���
*/
int Create_Data_Status(unsigned char *dstbuf,int num,struct status *p)
{
	int len = 0,offset = 0;
	unsigned char buff_time[15] = {0},bcd_time[7] = {0},buff_num[4] = {0};

	unsigned int swap = 0;
	//unsigned int tmp1,tmp2;

	dstbuf[0] = 0x00;//�ɼ���״̬

	swap = restartnum;
	Rev32InByte(&swap);
	memcpy(dstbuf+1,&swap,4);//�ɼ�����������Unsigned int 
	read_profile_string("WGID","restarttime",(char *)buff_time, 15, "00000000000000", WGINFOINI);
	StrtoBCD(buff_time,14,bcd_time);
	memcpy(dstbuf+5,bcd_time,7);//�ɼ������һ������ʱ��BCD�����ʽ��YYYYMMDDHHMMSS
	//print_hexoflen(bcd_time,7);
	snprintf(buff_num,4,"%03d",num);
	print_log(LOG_DEBUG,"buff_time:%14s|SN:%12s|buff_num:%3s\n",buff_time,p->SN,buff_num);
	memcpy((char *)dstbuf+12,buff_num,3);//���׮����ASCII�����ʽ

	offset = 15+(num-1)*15;
	memcpy(dstbuf+offset,p->SN,12);//���׮1��SN 12bytes
	dstbuf[offset+12] = p->connect;//���׮1��ͨ��״̬
	dstbuf[offset+13] = p->CStatus;//���׮1�ĵ�ǰ״̬
	dstbuf[offset+14] = p->Hw_Status;//���׮1��Ӳ��״̬
	len = 15+15*num;
	return len;
}



/*
   ���豸״̬�ϱ�����0x07��Data����
dstbuf:��ɵ�Ŀ�껺����
p:���׮��״̬�ṹ��
*/
int Create_Data_Query(unsigned char *dstbuf,struct status *p)
{
	int offset = 0,charg_time = 0,rate = 0,sum = 0;
	unsigned char cur_time[14] = {0},chargstart_time[14] = {0},startdn[9] = {0},datazero[31] = {0},nu[12] = {0};
	struct tm cur_t,chargstart_t;
	int rate1 = 0,rate2 = 0,rate3 = 0;
	unsigned char stime_str[14] = {0},etime_str[14] = {0},dn_str[9] = {0};
	unsigned int stime_int =0,etime_int = 0,swap = 0;

	memcpy(dstbuf+offset,p->SN,sizeof(p->SN));//���׮SN 12bytes
	offset +=  sizeof(p->SN);
	//�ɼ���״̬
	if(p->connect == 0x00)
	{
		dstbuf[offset] = 0x00;
		offset += 1;
		memcpy(dstbuf+offset,p->DT,sizeof(p->DT));//���׮��״̬�ɼ�ʱ��
		offset += sizeof(p->DT);

		BCDtoStr(p->ENERGY,sizeof(p->ENERGY),dn_str);
		BCDtoStr(p->STARTENERGY,sizeof(p->STARTENERGY),startdn);
		swap = (atoi(dn_str) - atoi(startdn));
		print_log(LOG_DEBUG,"���ĵ���%d (0.01kwh)\n",swap);
		Rev32InByte(&swap);
		memcpy(dstbuf+offset,&swap,sizeof(p->ENERGY));//���׮�ĵ�ǰ���ĵ���
		offset += sizeof(p->ENERGY);

		dstbuf[offset] = 0x01;//���׮1��ʵʱ������Ч��־
		offset += 1;

		//���׮1�ĳ��ʱ��(DT-SCT)
		BCDtoStr(p->DT,7,cur_time);
		BCDtoStr(p->SCT,7,chargstart_time);
		Strtotm(cur_time, &cur_t);
		Strtotm(chargstart_time, &chargstart_t);
		charg_time = mktime(&cur_t) - mktime(&chargstart_t);
		swap = charg_time;
		Rev32InByte(&swap);
		memcpy(dstbuf+offset,&swap,4);
		offset += 4;

		//���׮1�ĳ���λ:��
		if(p->Flag1 == 0x01)
		{
			memcpy(dstbuf+offset+30,p->MoneyCharg,sizeof(p->MoneyCharg));
		}
		else{
			//���ʵ�λ:��
			rate = Find_Rate(p->DT,sizeof(p->DT),atoi(ratenum),nu,&rate1,&rate2,&rate3);
			BCDtoStr(p->STARTENERGY,4,startdn);
			BCDtoStr(p->DT,7, etime_str);
			BCDtoStr(p->SCT,7, stime_str);
			etime_int = time14ToInt(etime_str);
			stime_int = time14ToInt(stime_str);
			sum = (((atoi(dn_str) - atoi(startdn))*(rate1 + rate3)/100) + rate2);
			swap = sum;
			Rev32InByte(&swap);
			memcpy(dstbuf+offset,&swap,sizeof(swap));//Ӧ�ս��(���ؽ���)
			print_log(LOG_DEBUG,"ʵʱ�����%d   cdn:%s sdn:%s totaltime:%d   rate:%d %d %d\n",sum,dn_str,startdn,charg_time,rate1,rate2,rate3);
		}
		offset += 4;
		//���׮1�ı���
		memset(p->RFU,'\0',sizeof(p->RFU));
		memcpy(dstbuf+offset,p->RFU,sizeof(p->RFU));	
		offset += sizeof(p->RFU);
	}
	else if(p->connect == 0x01)
	{
		dstbuf[offset] = 0x02;//ͨѶ�жϣ��޷���ѯ������������Ч
		offset += 1;
		memcpy(dstbuf+offset,datazero,sizeof(datazero));
		offset += sizeof(datazero);
	}
	return offset;
}




/*
   ���ҷ��ʣ�������������ϵķ���ͬʱ��Ч��ȡ��Сֵ
time:7�ֽ�BCDʱ��
len_time:7
num:���ʸ���
fatenum:���ҵ��ķ��ʱ��
����ֵ:���ʣ�����0���ҳɹ�������0ʧ��
*/
int Find_Rate(unsigned char *time,int len_time,int num,unsigned char *nu,unsigned int *rate1,unsigned int *rate2,unsigned int *rate3)
{
	int i= 0,minrate = 0,rateint1 = 0,rateint2=0,rateint3=0;
	unsigned char time_str[14] = {0},ratestr1[7] = {0},ratestr2[7] = {0},ratestr3[7] = {0};

	BCDtoStr(time,4,time_str);//ֻȡ������
	print_log(LOG_DEBUG,"time_str:%8s\n",time_str);
	for(i = 0;i < num;i++){
		if( (memcmp((char *)time_str,(char *)ratelist+(i*46),8) >= 0) && (memcmp((char *)time_str,(char *)ratelist+(i*46 + 8),8) <= 0) ){
			memcpy(ratestr1,ratelist+(i*46+16),sizeof(ratestr1)-1);
			memcpy(ratestr2,ratelist+(i*46+22),sizeof(ratestr2)-1);
			memcpy(ratestr3,ratelist+(i*46+28),sizeof(ratestr3)-1);
			if(minrate == 0){
				minrate = atoi((char *)ratestr1);

				rateint1 = atoi((char *)ratestr1);
				rateint2 = atoi((char *)ratestr2);
				rateint3 = atoi((char *)ratestr3);
				memcpy(nu,ratelist+(i*46+34),12);
			}
			else{
				if(atoi((char *)ratestr1) < minrate){
					minrate = atoi((char *)ratestr1);

					rateint1 = atoi((char *)ratestr1);
					rateint2 = atoi((char *)ratestr2);
					rateint3 = atoi((char *)ratestr3);
					memcpy(nu,ratelist+(i*46+34),12);
				}
			}	
		}
	}
	memcpy(rate1,&rateint1,sizeof(rateint1));
	memcpy(rate2,&rateint2,sizeof(rateint2));
	memcpy(rate3,&rateint3,sizeof(rateint3));
	return minrate;
}

int Data_Response01(unsigned char *buff)
{
	return 0;

}
/*
����������·��Ĺ̼������ļ�
filenum:�����ļ�����
filesize:���������ļ��Ĵ�С
����ֵ:
0000�ļ�����0
�м��ļ�����1
ffff�ļ�����2
crcУ����󷵻�-1
*/
int Deal_Updatefile(char *src,unsigned short *filenum,unsigned int *filesize,unsigned short *filecount)
{
	char crcfile[2] = {0},crc[2] = {0},buff[1030] = {0};
	unsigned short addr = 0,crc16 = 0;
	
	memcpy(&addr,src,sizeof(addr));
	Rev16InByte(&addr);
	if(addr == 0){
		/*
		memcpy(buff,src+2,30);
		crc16 = CRC16(buff,30);
		*/
		memcpy(CPversion,src+6,sizeof(CPversion));
		memcpy(filenum,src+8,sizeof(unsigned short));
		Rev16InByte(filenum);
		memcpy(filesize,src+22,sizeof(unsigned int));
		Rev32InByte(filesize);
		memcpy(crcfile,src+26,sizeof(crcfile));
		memcpy(crc,src+32,sizeof(crc));
		*filecount = 1;
		print_log(LOG_DEBUG,"%02x %02x = filenum:%d %02x %02x %02x %02x = filesize:%d CPversion:%02x%02x crcfile:%02x%02x\n",\
			src[8],src[9],*filenum,src[22],src[23],src[24],src[25],*filesize,CPversion[0],CPversion[1],crcfile[0],crcfile[1]);

		/*
		if(src[len-2] != XOR_SUM(p,len-5))
			return -1;
		*/
		return 0;
	}
	else{
		memcpy(buff,src+2,sizeof(buff));
		memcpy(crc,src+1030,sizeof(crc));
		crc16 = CRC16(buff,sizeof(buff));
		print_log(LOG_DEBUG,"%02x %02x = filenum:%d %02x %02x %02x %02x = filesize:%d CPversion:%02x%02x crcfile:%02x%02x\n",\
			src[8],src[9],*filenum,src[22],src[23],src[24],src[25],*filesize,CPversion[0],CPversion[1],crcfile[0],crcfile[1]);

		/*
		if(memcmp(&crc16,crc,sizeof(crc)) != 0)
			return -1;
		*/
		*filecount += 1;
		if(addr == 0xffff){	
			return 2;
		}
		else{
			return 1;
		}
	}
}
/*
   ������ָ��ת��Ϊ�Գ��׮��ָ��
dataid:ͨѶ���
t_sn:�ɼ������
data:Data����
datalen:Data���ֳ���
*/
void Deal_0x04(unsigned int dataid,unsigned int t_sn,unsigned char *data,unsigned int datalen)
{
	unsigned char controlnum = 0,psn_own = 0;
	unsigned char SN_piple[12] = {0},SN_chariging[12],serialframe[128] = {0},serialnum[3] = {0};
	unsigned int control_data_len = 0,swap = 0;
	struct node tmp;
	memset(&tmp,'\0',sizeof(tmp));
	int framelen = 0;
	controlnum = data[0];
	memcpy(SN_piple,data+1,sizeof(SN_piple));//���׮SN
	memcpy(SN_chariging,data+1+12,sizeof(SN_chariging));//������SN
	control_data_len = datalen - 25;


	//��ʼ��һ��struct node�ṹ��
	tmp.cmdid = 0x04;
	tmp.DATAID = dataid;
	tmp.T_SN = t_sn;
	memcpy(tmp.SN,SN_piple,sizeof(SN_piple));	

	tmp.Serialtimeout = 0x0064;//100ms
	memcpy(tmp.Charg_SN,SN_chariging,sizeof(tmp.Charg_SN));

	if(controlnum == CTRLB_END){
		tmp.flag1 = data[25];
		if(tmp.flag1 == 0x01){//��翪ʼʱ�俪ʼ������Ч����Ч��ɼ����Լ�����
			memcpy(tmp.Stime,data+26,sizeof(tmp.Stime));
			memcpy(tmp.SDn,data+33,sizeof(tmp.SDn));
		}
	}
	else if( (controlnum == CTRLB_CARD) || (controlnum == CTRLB_ETC) ){
		tmp.flag2 = data[25];
		tmp.Pay = 0;
		if(tmp.flag2 == 0x01)//���ѽ����Ч
		{	
			print_log(LOG_DEBUG,"�������·����ѽ����Ч\n");
			memcpy(&swap,data+26,sizeof(swap));
			Rev32InByte(&swap);
			tmp.Pay = swap;
		}
		else{
			print_log(LOG_DEBUG,"�������·����ѽ����Ч\n");
		}
	}

	/*ͨ��SN_piple���Ҹó��׮������һ������*/
	Getserianuml_SN((char *)SN_piple,(char *)serialnum);//���ҳ��׮SN������һ������
	tmp.Serialnum = atoi((char *)serialnum);
	print_log(LOG_DEBUG,"----serialnum:%d\n",atoi((char *)serialnum));

	print_log(LOG_DEBUG,"�յ�ָ��ʱ��:%d\n",time(NULL));
	psn_own = PSN++;
	tmp.frame_psn = psn_own;
	tmp.controlnum = controlnum;
	switch(controlnum){
		case 1://�������
			print_log(LOG_INFO,"�������\n");
			write_log_time(LOG_INFO,"CTRLB_START:%.12s\n",SN_piple);
			//��һ֡���׮������֡
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_START, serialframe);
			tmp.controlnum = CTRLB_START;
			tmp.DATA_LEN = framelen;
			memcpy(tmp.DATA,serialframe,framelen);
			/*�����������׮��������д��head_serialsendlist*/
			break;
		case 2://������磬���������״̬
			print_log(LOG_INFO,"���������������״̬\n");
			write_log_time(LOG_INFO,"CTRLB_END:%.12s\n",SN_piple);
			/*������磬���������״̬��������д��head_serialsendlist*/
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_END, serialframe);
			tmp.DATA_LEN = framelen;
			tmp.controlnum = CTRLB_END;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 3://�������״̬
			print_log(LOG_INFO,"�������״̬�ͷų��ǹ\n");
			write_log_time(LOG_INFO,"CTRLB_FINISH:%.12s\n",SN_piple);
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_FINISH, serialframe);
			tmp.DATA_LEN = framelen;
			tmp.controlnum = CTRLB_FINISH;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 4:
			print_log(LOG_INFO,"ѡ��ͨ������\n");
			write_log_time(LOG_INFO,"PAYBYCARD:%.12s\n",SN_piple);
			/*Ԥ�����ú�֧�ֽ�ͨ�����Ѻ�ETC����,�ȷ���ȡ���׿�Ƭ��Ϣָ��serialprocess�յ���ָ�Ƭ����㹻�ٷ��ۿ�ָ��*/
			framelen = Frame_readcard(psn_own,SN_piple, SN_chariging,serialframe);
			tmp.DATA_LEN = framelen;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 5:
			print_log(LOG_INFO,"ѡ��ETC����\n");
			write_log_time(LOG_INFO,"PAYBYETC:%.12s\n",SN_piple);
			/*Ԥ�����ú�֧�ֽ�ͨ�����Ѻ�ETC����,�ȷ���ȡ���׿�Ƭ��Ϣָ��serialprocess�յ���ָ�Ƭ����㹻�ٷ��ۿ�ָ��*/
			framelen = Frame_readcard(psn_own,SN_piple, SN_chariging,serialframe);
			tmp.DATA_LEN = framelen;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 129:
			print_log(LOG_INFO,"�˳��������״̬�����ش���״̬\n");
			write_log_time(LOG_INFO,"CTRLB_START_WAIT:%.12s\n",SN_piple);
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_START_WAIT, serialframe);
			tmp.DATA_LEN = framelen;
			tmp.controlnum = CTRLB_START_WAIT;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 130:
			print_log(LOG_INFO,"�˳�������״̬���س��״̬\n");
			write_log_time(LOG_INFO,"CTRLB_WILLPAY_CHARG:%.12s\n",SN_piple);
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_WILLPAY_CHARG, serialframe);
			tmp.DATA_LEN = framelen;
			tmp.controlnum = CTRLB_WILLPAY_CHARG;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 132:
			print_log(LOG_INFO,"�˳�����״̬���ؽ������״̬���ȴ�ѡ��֧����ʽ\n");
			write_log_time(LOG_INFO,"CTRLB_CHANGEPAY:%.12s\n",SN_piple);
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_CHANGEPAY, serialframe);
			tmp.DATA_LEN = framelen;
			tmp.controlnum = CTRLB_CHANGEPAY;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 238:
			print_log(LOG_INFO,"Ӧ��ֹͣ��ֱ��ֹͣ��磬���ͷų��ǹ\n");
			write_log_time(LOG_INFO,"CTRLB_ALLSTOP:%.12s\n",SN_piple);
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_ALLSTOP, serialframe);
			tmp.DATA_LEN = framelen;
			tmp.controlnum = CTRLB_ALLSTOP;
			memcpy(tmp.DATA,serialframe,framelen);
			break;

	}
	//new ���ֲ�ͬ�Ĵ���д����ͬ��������
	while(pthread_mutex_lock(&serialsendlist_lock)!=0);
	if(atoi(serialnum) == 1){
		AddFromEnd(head_serialsendlist1,&tmp);
	}
	else if(atoi(serialnum) == 2){
		AddFromEnd(head_serialsendlist2,&tmp);
	}
	else if(atoi(serialnum) == 3){
		AddFromEnd(head_serialsendlist3,&tmp);
	}
	else if(atoi(serialnum) == 4){
		AddFromEnd(head_serialsendlist4,&tmp);
	}
	else if(atoi(serialnum) == 5){
		AddFromEnd(head_serialsendlist5,&tmp);
	}
	else if(atoi(serialnum) == 6){
		AddFromEnd(head_serialsendlist6,&tmp);
	}
	while(pthread_mutex_unlock(&serialsendlist_lock)!=0);

	PSN += 1;


}

int Create_Data_History(unsigned char *dst,int offset,char flag_end,unsigned char* data,int data_len)
{
	unsigned short num;
	int n = 0;
	dst[0] = flag_end;
	n += 1;
	memcpy(&num,dst+1,sizeof(num));
	num += 1;
	Rev16InByte(&num);
	memcpy(dst+n,&num,sizeof(num));
	n += 2;
	n += offset;
	memcpy(dst+n,data,data_len);
	n += data_len;
	return n;
}

void Event_transfer(unsigned char *startdate,unsigned char *enddate)
{	
	/*************************��ʵ��*******************************/	
	unsigned char buff[1500] = {0},res_frame[1500] = {0},stime[4] = {0},data[300] = {0};
	int i = 0,data_len = 0,offset = 0,len = 0,res_len = 0,len1 = 0,num = 0,n = 0;
	unsigned char e_type = 0,end = 0x01,noend = 0x00;
	struct node p;

	for(i = 0;i < MAXEVENT;i++)
	{
		offset = i*300;
		while(pthread_mutex_lock(&eventbuf_lock)!=0);
		memcpy(stime,eventbuf+offset,sizeof(stime));
		if( (memcmp(stime,Start_Time,sizeof(stime))>=0) && (memcmp(stime,End_Time,sizeof(stime)) <= 0) )
		{
			num++;
			offset += 8;
			e_type = eventbuf[offset];
			if(e_type == 0x01)
				data_len  = 40;
			else if(e_type == 0x02)
				data_len = 60;
			else if(e_type == 0x04)
				data_len = 291;
			else
				data_len = 36;
			len1 = (data_len + 9);//ÿ���¼�����
			memcpy(data,eventbuf+(i*300),len1);
			if(num == 4){
				len = Create_Data_History(buff,n,end,data,len1);
				res_len = Response_Frame(res_frame,CMDID_SERVER_GATHER,WG_DATAID,wgid,buff,len);
				WG_DATAID++;
				num = 0;
				n = 0;
				memset(&p,'\0',sizeof(struct node));
				p.DATA_LEN = res_len;
				memcpy(p.DATA,res_frame,res_len);
				AddFromEnd(head_tcpsendlist, &p);
			}
			else
				len = Create_Data_History(buff,n,noend,data,len1);
			n += len1;
		}
		while(pthread_mutex_unlock(&eventbuf_lock)!=0);
	}
	return ;
}

