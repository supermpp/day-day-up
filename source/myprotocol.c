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


unsigned int EVENTSN = 0;//事件SN采集器维护的交易序号每个事件不同只在事件上传命令里用到


void CharToString(void *d,const void *s,int len)
{
	memcpy(d,s,len);
	*(((char *)d)+len)='\0';
	return;
}

/*
card_id:4字节物理卡号
card_num:10字节ascii
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
	if( (0x01&src[3]) == SAFEMODE ){//data部分加密报文
		rc4DE((char *)data,datalen);//解密data
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
   用于组网关服务器通信报文
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
	//fun 0x00不加密0x01加密
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
   组事件上报报文的data部分
carddata:刷卡及卡片扣款数据定义
flag:金额是否足够扣款，0不够1够
money:扣款金额
返回值:
>0 代表会有交易事件上传
==0 卡余额足够,立即下发扣款指令

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

	dstbuf[0] = src->CFlg;//事件类型 0x01：启动充电
	offset += 1;
	memcpy(dstbuf+offset,src->SN,sizeof(src->SN));//充电桩SN
	offset += sizeof(src->SN);
	memcpy(dstbuf+offset,src->EventSN,sizeof(src->EventSN));//充电过程SN
	offset += sizeof(src->EventSN);
	swap = EVENTSN;
	Rev32InByte(&swap);
	memcpy(dstbuf+offset,&swap,sizeof(swap));//事件SN由采集器维护的交易序号,每个事件都不相同
	offset += 4;
	//EVENTSN++;
	memcpy(dstbuf+offset,src->DT,sizeof(src->DT));//时间BCD编码格式，YYYYMMDDHHMMSS，由充电桩产生
	offset += sizeof(src->DT);
	print_log(LOG_DEBUG,"Create_Data_EventUp  CFlg:%02x\n",src->CFlg);
	if(src->CFlg == CTRLB_START)
	{
		//电表当前总电能：int单位0.01kWh
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
		memcpy(dstbuf+offset,&swap,sizeof(src->ENERGY));//电量(电表当前总电能：XXXXXX.XX 单位kWh)
		offset += sizeof(src->ENERGY);

		rate = Find_Rate(src->DT,sizeof(src->DT),atoi(ratenum),num,&rate1,&rate2,&rate3);
		print_log(LOG_DEBUG,"rate:%d\nnum:%12s\n",rate,num);
		if(rate > 0){
			memcpy(dstbuf+offset,num,sizeof(num));//费率编号
		}
		else{
			memcpy(dstbuf+offset,"000000000000",sizeof(num));//未查找到费率编号则置为全0
		}
		offset += 12;

		BCDtoStr(src->STARTENERGY,4,startdn);
		if(atoi(dn_str) < atoi(startdn))//new 电能超过最大示数
		{
			swap =  ( atoi(dn_str) +1000000 - atoi(startdn) );
		}
		else
		{
			swap = (atoi(dn_str) - atoi(startdn));
		}
		/**************************************/
		//测试测试交通卡余额不足数据消耗电量100度
		//swap = 10000;
		/**************************************/
	
		Rev32InByte(&swap);
		memcpy(dstbuf+offset,&swap,sizeof(swap));//使用电量
		offset += sizeof(tmp);

		//收到待付费指令每次都重新结算金额
		BCDtoStr(src->STARTENERGY,4,startdn);
		BCDtoStr(src->DT,7, etime_str);
		BCDtoStr(src->SCT,7, stime_str);
		etime_int = time14ToInt(etime_str);
		stime_int = time14ToInt(stime_str);

		//new
		Rev32InByte(&swap);
		sum = (swap*(rate1+rate3)/100+rate2);

		/**************************************/
		//测试交通卡余额不足数据消耗电量100度
		//sum = 10000*(rate1+rate3)/100+rate2;
		/**************************************/
		
		swap = sum;
		Rev32InByte(&swap);
		print_log(LOG_DEBUG,"待付费-----结算金额:%d  当前电能:%s 费率编号%s 使用电量:%d(0.01kwh)\n",sum,dn_str,num,(atoi(dn_str) - atoi(startdn)));
		memcpy(dstbuf+offset,&swap,sizeof(sum));//应收金额(网关结算)
		src->MoneyCharg = sum;
		offset += sizeof(sum);
	}
	else if(src->CFlg == CTRLB_CARD)
	{
		print_log(LOG_DEBUG,"--------------Create_Data_EventUp-----------CTRLB_CARD\n");
		/*判断卡片黑名单*/
		//memcpy(cardsn,carddata+5,sizeof(cardsn));
		memcpy(cardnum,carddata+11,sizeof(cardnum));//物理卡号
		get_card_outnum(cardnum,cardsn);//物理卡号转卡面号
		for(i = 0;i < atoi(blacknum);i++)
		{
			if(memcmp(cardsn,blacklist+(i*12),sizeof(cardsn)) == 0)
			{
				print_log(LOG_DEBUG,"该卡属于黑名单卡片\n");
				dstbuf[offset] = BLACKCARD;//黑名单卡片 
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
		/*判卡余额是否足够*/
		//print_hexoflen(carddata, int len);
		memcpy(balance,carddata+20,sizeof(balance));
		print_log(LOG_DEBUG,"卡号:%s\n刷卡信息:\nbalance:",cardsn);
		print_hexoflen(balance,4);
		memcpy(&yue,balance,4);
		//Rev32InByte(&yue);
		print_log(LOG_DEBUG,"余额:%d\n",yue);
		Rev32InByte(balance);

		if(src->Flag1 == 0x01)//服务端下发的充电金额有效
		{
			print_log(LOG_DEBUG,"选择交通卡付款结算金额:%d\n",src->MoneyCharg);
			//if(memcmp(&yue,&(src->MoneyCharg),sizeof(src->MoneyCharg)) >= 0)
			if(yue >= src->MoneyCharg)
			{
				print_log(LOG_DEBUG,"金额足够\n");
				swap = src->MoneyCharg;
				Rev32InByte(&swap);
				memcpy(money,&swap,sizeof(swap));
				//memcpy(money,&(src->MoneyCharg),sizeof(src->MoneyCharg));
				//金额足够返回0
				*flag = 1;
				return 0;
			}
			else
			{
				//金额不足够
				print_log(LOG_DEBUG,"金额不足\n");	
				dstbuf[offset] = NOTENOUGH;//标志金额不足
				offset += 1;
				memcpy(dstbuf+offset,cardsn,sizeof(cardsn));//卡号
				offset += sizeof(cardsn);
				memcpy(dstbuf+offset,carddata+11,4);//余额
				offset += 4;
				memcpy(dstbuf+offset,FH,sizeof(FH));
				offset += sizeof(FH);
			}
		}
		else//服务端下发的充电金额无效无需计算，直接用待付费时候的金额
		{
			sum = src->MoneyCharg;
			print_log(LOG_DEBUG,"选择交通卡付款---------------------结算金额:%d\n",sum);
			swap = sum;
			Rev32InByte(&swap);
			memcpy(money,&swap,sizeof(swap));//应收金额(网关结算)
			//if(memcmp(&yue,&sum,sizeof(sum)) >= 0)
			if(yue >= sum)
			{
				//金额足够返回0
				print_log(LOG_DEBUG,"--金额足够--\n");
				*flag = 1;
				return 0;
			}
			else
			{
				print_log(LOG_DEBUG,"--金额不足--\n");	
				dstbuf[offset] = NOTENOUGH;//标志金额不足
				offset += 1;
				memcpy(dstbuf+offset,cardsn,sizeof(cardsn));//卡号
				offset += sizeof(cardsn);
				//memcpy(dstbuf+offset,carddata+11,4);//余额
				Rev32InByte(&yue);
				memcpy(dstbuf+offset,&yue,4);//余额
				offset += 4;
				memcpy(dstbuf+offset,FH,sizeof(FH));
				offset += sizeof(FH);
			}
		}
	}
	else if(src->CFlg == CTRLB_PAYMENTEND )
	{
		/*刷卡完成之后要以刷卡事件上报*/
		dstbuf[0] = CTRLB_CARD;
		if(carddata[0] == Deal_Success)
		{
			dstbuf[offset] = PAYSUCCESS;//支付成功
			print_log(LOG_DEBUG,"*********************扣款成功*********************\n");

		}
		else if(carddata[0] == Deal_Fail)//交易未成功
		{
			dstbuf[offset] = DEALFAIL;
			print_log(LOG_DEBUG,"*********************扣款失败*********************\n");

		}
		else if(carddata[0] == Deal_NotAck)//交易不确认
		{
			dstbuf[offset] = DEALNOTACK;
			print_log(LOG_DEBUG,"*********************扣款不确认*********************\n");

			return -1;//返回-1，代表扣款不确认，由网关重新下发扣款指令
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
		print_log(LOG_DEBUG,"*********************车辆返回充电完成或充电故障*********************\n");
		BCDtoStr(src->ENERGY,sizeof(src->ENERGY), dn_str);
		swap = atoi(dn_str);
		Rev32InByte(&swap);
		memcpy(dstbuf+offset,&swap,sizeof(src->ENERGY));//电量(电表当前总电能：XXXXXX.XX 单位kWh)
		offset += sizeof(src->ENERGY);

		rate = Find_Rate(src->DT,sizeof(src->DT),atoi(ratenum),num,&rate1,&rate2,&rate3);
		print_log(LOG_DEBUG,"rate:%d\nnum:%12s\n",rate,num);
		if(rate > 0){
			memcpy(dstbuf+offset,num,sizeof(num));//费率编号
		}
		else{
			memcpy(dstbuf+offset,"000000000000",sizeof(num));//未查找到费率编号则置为全0
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
		memcpy(dstbuf+offset,&swap,sizeof(swap));//使用电量
		offset += sizeof(tmp);

		//收到待付费指令每次都重新结算金额
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
		print_log(LOG_DEBUG,"--结算金额:%d  当前电能:%s 费率编号%s 使用电量:%d(0.01kwh)\n",sum,dn_str,num,(atoi(dn_str) - atoi(startdn)));
		memcpy(dstbuf+offset,&swap,sizeof(sum));//应收金额(网关结算)
		src->MoneyCharg = sum;
		offset += sizeof(sum);
	}
	else
	{
		/*无内容*/
		return offset;
	}
	return offset;
}




/*
   组设备状态上报报文0x03的Data部分
dataid:通讯序号
t_sn:采集器编号
data:Data部分
datalen:Data部分长度
*/
int Create_Data_Status(unsigned char *dstbuf,int num,struct status *p)
{
	int len = 0,offset = 0;
	unsigned char buff_time[15] = {0},bcd_time[7] = {0},buff_num[4] = {0};

	unsigned int swap = 0;
	//unsigned int tmp1,tmp2;

	dstbuf[0] = 0x00;//采集器状态

	swap = restartnum;
	Rev32InByte(&swap);
	memcpy(dstbuf+1,&swap,4);//采集器重启次数Unsigned int 
	read_profile_string("WGID","restarttime",(char *)buff_time, 15, "00000000000000", WGINFOINI);
	StrtoBCD(buff_time,14,bcd_time);
	memcpy(dstbuf+5,bcd_time,7);//采集器最后一次重启时间BCD编码格式，YYYYMMDDHHMMSS
	//print_hexoflen(bcd_time,7);
	snprintf(buff_num,4,"%03d",num);
	print_log(LOG_DEBUG,"buff_time:%14s|SN:%12s|buff_num:%3s\n",buff_time,p->SN,buff_num);
	memcpy((char *)dstbuf+12,buff_num,3);//充电桩个数ASCII编码格式

	offset = 15+(num-1)*15;
	memcpy(dstbuf+offset,p->SN,12);//充电桩1的SN 12bytes
	dstbuf[offset+12] = p->connect;//充电桩1的通信状态
	dstbuf[offset+13] = p->CStatus;//充电桩1的当前状态
	dstbuf[offset+14] = p->Hw_Status;//充电桩1的硬件状态
	len = 15+15*num;
	return len;
}



/*
   组设备状态上报报文0x07的Data部分
dstbuf:组成的目标缓冲区
p:充电桩的状态结构体
*/
int Create_Data_Query(unsigned char *dstbuf,struct status *p)
{
	int offset = 0,charg_time = 0,rate = 0,sum = 0;
	unsigned char cur_time[14] = {0},chargstart_time[14] = {0},startdn[9] = {0},datazero[31] = {0},nu[12] = {0};
	struct tm cur_t,chargstart_t;
	int rate1 = 0,rate2 = 0,rate3 = 0;
	unsigned char stime_str[14] = {0},etime_str[14] = {0},dn_str[9] = {0};
	unsigned int stime_int =0,etime_int = 0,swap = 0;

	memcpy(dstbuf+offset,p->SN,sizeof(p->SN));//充电桩SN 12bytes
	offset +=  sizeof(p->SN);
	//采集器状态
	if(p->connect == 0x00)
	{
		dstbuf[offset] = 0x00;
		offset += 1;
		memcpy(dstbuf+offset,p->DT,sizeof(p->DT));//充电桩的状态采集时间
		offset += sizeof(p->DT);

		BCDtoStr(p->ENERGY,sizeof(p->ENERGY),dn_str);
		BCDtoStr(p->STARTENERGY,sizeof(p->STARTENERGY),startdn);
		swap = (atoi(dn_str) - atoi(startdn));
		print_log(LOG_DEBUG,"消耗电量%d (0.01kwh)\n",swap);
		Rev32InByte(&swap);
		memcpy(dstbuf+offset,&swap,sizeof(p->ENERGY));//充电桩的当前消耗电量
		offset += sizeof(p->ENERGY);

		dstbuf[offset] = 0x01;//充电桩1的实时费用有效标志
		offset += 1;

		//充电桩1的充电时间(DT-SCT)
		BCDtoStr(p->DT,7,cur_time);
		BCDtoStr(p->SCT,7,chargstart_time);
		Strtotm(cur_time, &cur_t);
		Strtotm(chargstart_time, &chargstart_t);
		charg_time = mktime(&cur_t) - mktime(&chargstart_t);
		swap = charg_time;
		Rev32InByte(&swap);
		memcpy(dstbuf+offset,&swap,4);
		offset += 4;

		//充电桩1的充电金额单位:分
		if(p->Flag1 == 0x01)
		{
			memcpy(dstbuf+offset+30,p->MoneyCharg,sizeof(p->MoneyCharg));
		}
		else{
			//费率单位:分
			rate = Find_Rate(p->DT,sizeof(p->DT),atoi(ratenum),nu,&rate1,&rate2,&rate3);
			BCDtoStr(p->STARTENERGY,4,startdn);
			BCDtoStr(p->DT,7, etime_str);
			BCDtoStr(p->SCT,7, stime_str);
			etime_int = time14ToInt(etime_str);
			stime_int = time14ToInt(stime_str);
			sum = (((atoi(dn_str) - atoi(startdn))*(rate1 + rate3)/100) + rate2);
			swap = sum;
			Rev32InByte(&swap);
			memcpy(dstbuf+offset,&swap,sizeof(swap));//应收金额(网关结算)
			print_log(LOG_DEBUG,"实时充电金额%d   cdn:%s sdn:%s totaltime:%d   rate:%d %d %d\n",sum,dn_str,startdn,charg_time,rate1,rate2,rate3);
		}
		offset += 4;
		//充电桩1的备用
		memset(p->RFU,'\0',sizeof(p->RFU));
		memcpy(dstbuf+offset,p->RFU,sizeof(p->RFU));	
		offset += sizeof(p->RFU);
	}
	else if(p->connect == 0x01)
	{
		dstbuf[offset] = 0x02;//通讯中断，无法查询，以下数据无效
		offset += 1;
		memcpy(dstbuf+offset,datazero,sizeof(datazero));
		offset += sizeof(datazero);
	}
	return offset;
}




/*
   查找费率，如果有两个以上的费率同时有效，取较小值
time:7字节BCD时间
len_time:7
num:费率个数
fatenum:查找到的费率编号
返回值:费率，大于0查找成功，等于0失败
*/
int Find_Rate(unsigned char *time,int len_time,int num,unsigned char *nu,unsigned int *rate1,unsigned int *rate2,unsigned int *rate3)
{
	int i= 0,minrate = 0,rateint1 = 0,rateint2=0,rateint3=0;
	unsigned char time_str[14] = {0},ratestr1[7] = {0},ratestr2[7] = {0},ratestr3[7] = {0};

	BCDtoStr(time,4,time_str);//只取年月日
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
处理服务器下发的固件更新文件
filenum:更新文件数量
filesize:整个更新文件的大小
返回值:
0000文件返回0
中间文件返回1
ffff文件返回2
crc校验错误返回-1
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
   充电控制指令转化为对充电桩的指令
dataid:通讯序号
t_sn:采集器编号
data:Data部分
datalen:Data部分长度
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
	memcpy(SN_piple,data+1,sizeof(SN_piple));//充电桩SN
	memcpy(SN_chariging,data+1+12,sizeof(SN_chariging));//充电过程SN
	control_data_len = datalen - 25;


	//初始化一个struct node结构体
	tmp.cmdid = 0x04;
	tmp.DATAID = dataid;
	tmp.T_SN = t_sn;
	memcpy(tmp.SN,SN_piple,sizeof(SN_piple));	

	tmp.Serialtimeout = 0x0064;//100ms
	memcpy(tmp.Charg_SN,SN_chariging,sizeof(tmp.Charg_SN));

	if(controlnum == CTRLB_END){
		tmp.flag1 = data[25];
		if(tmp.flag1 == 0x01){//充电开始时间开始电量有效，无效需采集器自己查找
			memcpy(tmp.Stime,data+26,sizeof(tmp.Stime));
			memcpy(tmp.SDn,data+33,sizeof(tmp.SDn));
		}
	}
	else if( (controlnum == CTRLB_CARD) || (controlnum == CTRLB_ETC) ){
		tmp.flag2 = data[25];
		tmp.Pay = 0;
		if(tmp.flag2 == 0x01)//付费金额有效
		{	
			print_log(LOG_DEBUG,"服务器下发付费金额有效\n");
			memcpy(&swap,data+26,sizeof(swap));
			Rev32InByte(&swap);
			tmp.Pay = swap;
		}
		else{
			print_log(LOG_DEBUG,"服务器下发付费金额无效\n");
		}
	}

	/*通过SN_piple查找该充电桩属于哪一个串口*/
	Getserianuml_SN((char *)SN_piple,(char *)serialnum);//查找充电桩SN属于哪一个串口
	tmp.Serialnum = atoi((char *)serialnum);
	print_log(LOG_DEBUG,"----serialnum:%d\n",atoi((char *)serialnum));

	print_log(LOG_DEBUG,"收到指令时间:%d\n",time(NULL));
	psn_own = PSN++;
	tmp.frame_psn = psn_own;
	tmp.controlnum = controlnum;
	switch(controlnum){
		case 1://启动充电
			print_log(LOG_INFO,"启动充电\n");
			write_log_time(LOG_INFO,"CTRLB_START:%.12s\n",SN_piple);
			//组一帧充电桩的数据帧
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_START, serialframe);
			tmp.controlnum = CTRLB_START;
			tmp.DATA_LEN = framelen;
			memcpy(tmp.DATA,serialframe,framelen);
			/*启动操作充电桩串口数据写到head_serialsendlist*/
			break;
		case 2://结束充电，进入待付费状态
			print_log(LOG_INFO,"结束充电进入待付费状态\n");
			write_log_time(LOG_INFO,"CTRLB_END:%.12s\n",SN_piple);
			/*结束充电，进入待付费状态串口数据写到head_serialsendlist*/
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_END, serialframe);
			tmp.DATA_LEN = framelen;
			tmp.controlnum = CTRLB_END;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 3://结束充电状态
			print_log(LOG_INFO,"结束充电状态释放充电枪\n");
			write_log_time(LOG_INFO,"CTRLB_FINISH:%.12s\n",SN_piple);
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_FINISH, serialframe);
			tmp.DATA_LEN = framelen;
			tmp.controlnum = CTRLB_FINISH;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 4:
			print_log(LOG_INFO,"选择交通卡付费\n");
			write_log_time(LOG_INFO,"PAYBYCARD:%.12s\n",SN_piple);
			/*预先设置好支持交通卡付费和ETC付费,先发获取交易卡片信息指令serialprocess收到该指令卡片余额足够再发扣款指令*/
			framelen = Frame_readcard(psn_own,SN_piple, SN_chariging,serialframe);
			tmp.DATA_LEN = framelen;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 5:
			print_log(LOG_INFO,"选择ETC付费\n");
			write_log_time(LOG_INFO,"PAYBYETC:%.12s\n",SN_piple);
			/*预先设置好支持交通卡付费和ETC付费,先发获取交易卡片信息指令serialprocess收到该指令卡片余额足够再发扣款指令*/
			framelen = Frame_readcard(psn_own,SN_piple, SN_chariging,serialframe);
			tmp.DATA_LEN = framelen;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 129:
			print_log(LOG_INFO,"退出启动充电状态，返回待机状态\n");
			write_log_time(LOG_INFO,"CTRLB_START_WAIT:%.12s\n",SN_piple);
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_START_WAIT, serialframe);
			tmp.DATA_LEN = framelen;
			tmp.controlnum = CTRLB_START_WAIT;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 130:
			print_log(LOG_INFO,"退出待付费状态返回充电状态\n");
			write_log_time(LOG_INFO,"CTRLB_WILLPAY_CHARG:%.12s\n",SN_piple);
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_WILLPAY_CHARG, serialframe);
			tmp.DATA_LEN = framelen;
			tmp.controlnum = CTRLB_WILLPAY_CHARG;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 132:
			print_log(LOG_INFO,"退出付费状态返回结束充电状态，等待选择支付方式\n");
			write_log_time(LOG_INFO,"CTRLB_CHANGEPAY:%.12s\n",SN_piple);
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_CHANGEPAY, serialframe);
			tmp.DATA_LEN = framelen;
			tmp.controlnum = CTRLB_CHANGEPAY;
			memcpy(tmp.DATA,serialframe,framelen);
			break;
		case 238:
			print_log(LOG_INFO,"应急停止；直接停止充电，并释放充电枪\n");
			write_log_time(LOG_INFO,"CTRLB_ALLSTOP:%.12s\n",SN_piple);
			framelen = Frame_ChargingControl(psn_own, SN_piple, SN_chariging,CTRLB_ALLSTOP, serialframe);
			tmp.DATA_LEN = framelen;
			tmp.controlnum = CTRLB_ALLSTOP;
			memcpy(tmp.DATA,serialframe,framelen);
			break;

	}
	//new 区分不同的串口写到不同的链表里
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
	/*************************待实现*******************************/	
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
			len1 = (data_len + 9);//每条事件长度
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

