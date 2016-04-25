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


unsigned char PSN = 0;//指令帧序列号
unsigned char PSN_0b = 0;//0b指令帧序列号

//组给CMD指令数据加上头尾
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

//组充电桩数据帧
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
5.15	更新设备的文件下载和更新操作
num:传进来的帧序列号
chargsn:充电桩桩号
buff:更新的文件内容
buf_len:更新的文件内容的长度
dstframe:最终组成的数据帧

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
5.12	获取设备信息指令
num:传进来的帧序列号
chargsn:充电桩桩号
dstframe:最终组成的数据帧
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
更新文件下载指令
num:传进来的帧序列号
data:更新文件的头文件/其他下载文件
datalen:data长度
chargsn:充电桩桩号
dstframe:最终组成的数据帧
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
设置充电桩sn
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
时钟同步指令0x01  (广播指令设备无需应答)
PSN:指令帧序列号
dstframe:完整的一帧数据
cptime:充电桩时间ascii格式
返回值:dstframe长度
*/
int Frame_Clock_Sync(unsigned char myPSN,unsigned char *dstframe)
{
	unsigned char buff[16] = {0},time_str[15] = {0},time_bcd[7] = {0};
	int cmdlen = 0,len = 0;
	time_t t;
	struct tm *tm1;
	
	buff[0]=0xff;//ADDR匹配地址
	buff[1]=0x8a;//CLS充电桩指令
	buff[2]=myPSN;//PSN指令帧序列号
	buff[3]=CMD_CLOCK_SYNC;//PARM设备充电控制子指令
	buff[4]=0x08;//LC/LE指令数据长度
	t = time(NULL);
	tm1 = localtime(&t);
	snprintf((char *)time_str,15,"%04d%02d%02d%02d%02d%02d",tm1->tm_year+1900,tm1->tm_mon+1,tm1->tm_mday,tm1->tm_hour,tm1->tm_min,tm1->tm_sec);
	StrtoBCD(time_str,14,time_bcd);
	memcpy(buff+5,time_bcd,sizeof(time_bcd));//当前时间：YYYYMMDDhhmmss
	if(tm1->tm_wday == 0)
		buff[12] = 0x07;
	else
		buff[12] = tm1->tm_wday;
	
	cmdlen = 13;
	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);
	return len;
}


/*
5.5	设置设备参数指令0x05
PSN:指令帧序列号
SN:充电桩SN
dstframe:完整的一帧数据
返回值:dstframe长度
*/
int Frame_SetParameter(unsigned char PSN,unsigned char *SN,unsigned char *PVer,unsigned char *PData,unsigned char *dstframe)
{
	unsigned char buff[128] = {0};
	int cmdlen = 0,len = 0;
	
	buff[0]=0x00;//ADDR匹配地址
	buff[1]=0x8a;//CLS充电桩指令
	buff[2]=PSN;//PSN指令帧序列号
	buff[3]=CMD_SET_DEV_PARA;//PARM设备充电控制子指令
	buff[4]=0x50;//LC/LE指令数据长度

	memcpy(buff+5,SN,12);//充电桩SN
	memcpy(buff+17,PVer,4);//配置参数版本
	memcpy(buff+21,PData,64);//配置参数数据
	cmdlen = 85;
	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);
	return len;
}

/*
充电控制指令0x0c
PSN:指令帧序列号
SN:充电桩SN
EventSN:事件序号
Ctrlbyte:控制字
dstframe:完整的一帧数据
返回值:dstframe长度
*/
int Frame_ChargingControl(unsigned char myPSN,unsigned char *SN,unsigned char *EventSN,unsigned char Ctrlbyte,unsigned char *dstframe)
{
	unsigned char buff[128] = {0};
	int cmdlen = 0,len = 0;

	buff[0]=0x00;//ADDR匹配地址
	buff[1]=0x8a;//CLS充电桩指令
	buff[2]=myPSN;//PSN指令帧序列号
	buff[3]=CMD_ELECTRICIZE;//PARM设备充电控制子指令
	buff[4]=0x19;//LC/LE指令数据长度

	memcpy(buff+5,SN,12);
	memcpy(buff+17,EventSN,12);
	buff[29] = Ctrlbyte;
	cmdlen = 30;

	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);
	
	return len;
}

/*
获取待交易的交通卡、ETC卡信息0x0e
PSN:指令帧序列号
SN:充电桩SN
EventSN:事件序号
Ctrlbyte:控制字
dstframe:完整的一帧数据
返回值:dstframe长度
*/
int Frame_readcard(unsigned char PSN,unsigned char *SN,unsigned char *EventSN,unsigned char *dstframe)
{
	unsigned char buff[128] = {0};
	int cmdlen = 0,len = 0;

	buff[0]=0x00;//ADDR匹配地址
	buff[1]=0x8a;//CLS充电桩指令
	buff[2]=PSN;//PSN指令帧序列号
	buff[3]=CMD_GET_TRAN_MSG;//PARM设备充电控制子指令
	buff[4]=0x18;//LC/LE指令数据长度

	memcpy(buff+5,SN,12);
	memcpy(buff+17,EventSN,12);
	cmdlen = 29;
	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);

	return len;
}


/*
卡片扣款交易指令0x0f
PSN:指令帧序列号
SN:充电桩SN
EventSN:事件序号
Ctrlbyte:控制字
dstframe:完整的一帧数据
返回值:dstframe长度
*/
int Frame_payment(unsigned char myPSN,struct status *charg,unsigned char *dstframe)
{
	unsigned char buff[128] = {0},RFU[8] = {0};
	int cmdlen = 0,len = 0,offset = 0,sum = 0;

	buff[0]=0x00;//ADDR匹配地址
	buff[1]=0x8a;//CLS充电桩指令
	buff[2]=myPSN;//PSN指令帧序列号
	buff[3]=CMD_PAYMENT_TRAN;//卡片扣款交易指令
	buff[4]=0x28;//LC/LE指令数据长度
	offset = 5;

	memcpy(buff+offset,charg->SN,12);//充电桩SN
	offset += 12;
	memcpy(buff+offset,charg->EventSN,12);//事件序号
	offset += 12;
	memcpy(buff+offset,charg->CardID,sizeof(charg->CardID));//待交易卡ID
	offset += sizeof(charg->CardID);
	//交易金额（Hex）
	if(charg->Flag1)
	{	
		print_log(LOG_DEBUG,"Frame_payment   charg->MoneyCharg:%d\n",charg->MoneyCharg);
		memcpy(buff+offset,&(charg->MoneyCharg),sizeof(charg->MoneyCharg));
		offset += sizeof(charg->MoneyCharg);
	}
	else
	{
			sum = charg->MoneyCharg;
			print_log(LOG_DEBUG,"卡片扣款金额%d\n",sum);
			//Rev32InByte(&sum);
			memcpy(buff+offset,&sum,sizeof(sum));//应收金额(网关结算)
			offset += sizeof(sum);
	}
	
	memcpy(buff+offset,RFU,sizeof(RFU));//RFU全零
	offset += sizeof(RFU);

	cmdlen = 45;
	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);

	return len;
}


/*
交易事件清除指令0x1f
PSN:指令帧序列号
SN:充电桩SN
EventSN:事件序号
Ctrlbyte:控制字
dstframe:完整的一帧数据
返回值:dstframe长度
*/
int Frame_CleanEvent(unsigned char myPSN,struct status *charg,unsigned char CEvent,unsigned char *dstframe)
{
	unsigned char buff[128] = {0};
	int cmdlen = 0,len = 0,offset = 0;

	buff[0]=0x00;//ADDR匹配地址
	buff[1]=0x8a;//CLS充电桩指令
	buff[2]=myPSN;//PSN指令帧序列号
	buff[3]=CMD_CLEAN_TRAN;//交易事件清除指令
	buff[4]=0x0d;//LC/LE指令数据长度
	offset = 5;

	memcpy(buff+offset,charg->SN,12);//充电桩SN
	offset += 12;
	buff[offset] = CEvent;
	offset += 1;

	cmdlen = offset;
	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);

	return len;
}




/*
设备状态查询指令0x0b
PSN:指令帧序列号
SN:充电桩SN
dstframe:完整的一帧数据
返回值:dstframe长度
*/
int Create_Frame_StatusRequest(unsigned char myPSN,unsigned char *SN,unsigned char *dstframe)
{
	unsigned char buff[128] = {0};
	int cmdlen = 0,len = 0;

	buff[0]=0x00;//ADDR匹配地址
	buff[1]=0x8a;//CLS充电桩指令
	buff[2]=myPSN;//PSN指令帧序列号
	buff[3]=CMD_QUERY_STATE;//PARM设备充电控制子指令
	buff[4]=0x0c;//LC/LE指令数据长度
	memcpy(buff+5,SN,12);
	cmdlen = 17;

	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);
	return len;
}

/*
事件查询指令0x10
PSN:指令帧序列号
SN:充电桩SN
type:查询交易事件类型
dstframe:完整的一帧数据
返回值:dstframe长度
*/
int Frame_EventQuery(unsigned char myPSN,unsigned char *SN,unsigned char type,unsigned char *dstframe)
{
	unsigned char buff[128] = {0};
	int cmdlen = 0,len = 0;

	buff[0]=0x00;//ADDR匹配地址
	buff[1]=0x8a;//CLS充电桩指令
	buff[2]=myPSN;//PSN指令帧序列号
	buff[3]=CMD_EVENT_QUERY;//PARM设备充电控制子指令
	buff[4]=0x0d;//LC/LE指令数据长度
	memcpy(buff+5,SN,12);
	buff[17] = type;
	cmdlen = 18;

	len = MGTR_UART_tx_format(buff,cmdlen,dstframe);
	return len;
}


/*
设备状态查询指令0x0b回帧解析
PSN:指令帧序列号
SN:充电桩SN
dstframe:完整的一帧数据
返回值:
	正常收发数据返回dstframe长度>0
	收不到串口数据返回-2,此时通讯状态不通
	帧格式不正确返回-5
	指令执行不成功设备忙返回0

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
		if(dstframe[5] == 0x00){//指令执行成功
			stat->CStatus  = dstframe[7];
			stat->CEvent= dstframe[8];
			memcpy(stat->DT,dstframe+9,sizeof(stat->DT));
			memcpy(buff_dn,dstframe+16,4);
			memcpy(stat->ENERGY,dstframe+16,sizeof(stat->ENERGY));
			stat->Hw_Status = dstframe[20];
			//stat->Hw_Status = 0xFC;
			memcpy(stat->RFU,dstframe+21,sizeof(stat->RFU));
		}
		else//指令执行之后设备忙
			return 0;
		//状态忙就把串口数据丢掉
	}
	else if(ret == -2)//收不到数据(超时)
		return -2;
	else if(ret == -5)//帧格式不正确
		return -5;
	return ret;
}

//读取未知长度的串口数据
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
解充电桩数据帧
去掉STX  XOR  ETX
-2收不到数据，超时
-5格式不正确
>0能收到数据，需要判别执行状态位是否成功
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

