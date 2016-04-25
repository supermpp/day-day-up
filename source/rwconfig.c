#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "chargingpile.h"
#include "common.h"
#include "linklist.h"
#include "rwconfig.h"
#include "mypthread.h"
#include "mytime.h"

//全局变量
struct servercfg remoteserver;//远程服务器配置

//集中器参数
//int serialportnum = 0;//串口数

int chargingpilenum = 0;//充电桩总数

int serial1chargnum = 0;//串口1充电桩数
unsigned char serail1SN[384] = {0} ;//串口1的SN号32*12=384
int serial2chargnum = 0;//串口2充电桩数
unsigned char serail2SN[384] = {0} ;//串口2的SN号

//new
/*
以前是用多个变量来存储串口sn和串口号，
现在改成用二位数组来存储串口号，用一维数组来存储串口下挂的充电桩个数
*/
unsigned char AllSerialSn[SERIALNUM][384] = {{0},{0}};//所有串口下的充电桩SN，AllSerialSn[0]代表1号串口的充电桩sn
int EverySerialNum[SERIALNUM] = {0};//每个串口下充电桩个数，EverySerialNum[0]代表1号串口的充电桩数

unsigned char serailnum_sn[896] = {0};//所有充电桩sn和串口号

struct status **chargingstat1 = NULL;//串口1充电桩状态
struct status **chargingstat2 = NULL;//串口2充电桩状态


//费率文件
unsigned char ratenum[3] = {0};
unsigned char *ratelist = NULL;

//交通卡黑名单文件
unsigned char blacknum[6] = {0};
unsigned char *blacklist = NULL;

//把字符串开头的字符'0'删除
void delzero(unsigned char *buff,int len)
{
int i = 0,j = 0,begin = 0;

unsigned char *p = (unsigned char *)malloc(len*sizeof(char));
memset(p,'\0',len);
for(i = 0,j = 0;i < len;i++){
	if(begin == 0){
		if(buff[i] == '0')
			continue;
		else {
			p[j] = buff[i];
			j++;
			begin = 1;
		}
	}
	else{
		p[j] = buff[i];
		j++;
	}
}
memset(buff,'\0',len);
memcpy(buff,p,j);
free(p);
p = NULL;
}

int readwg_parameter(const char *filename)
{
char tmp[32] = {0},buff[32] = {0},sn[12] = {0},port[2] = {0};
char *p = NULL;
FILE *fp = NULL;
int  indexn[SERIALNUM] = {0},serialn = 0,num_s = 0;
fp = fopen(filename,"r");
int i = 1;
unsigned char *p1 = serail1SN,*p2 = serail2SN;
while(1){
	memset(tmp,'\0',sizeof(tmp));
	memset(buff,'\0',sizeof(buff));
	if(fgets((char *)tmp,sizeof(tmp),fp) == NULL)
		break;
	memcpy(buff,tmp,strlen((const char *)tmp));
	/*
	   if(strlen((const char *)tmp) > 1)
	   memcpy(buff,tmp,strlen((const char *)tmp) -1);
	   else 
	   memcpy(buff,tmp,strlen((const char *)tmp));
	   */
	print_hexoflen(buff, strlen((const char *)buff));
	p = buff;
	if(i == 1){
		memcpy(remoteserver.ip,p,sizeof(remoteserver.ip)-1);//ip 15bytes
		delzero(remoteserver.ip,15);
		memcpy(remoteserver.port,p+15,sizeof(remoteserver.port)-1);//port  5bytes
		delzero(remoteserver.port,5);
		memcpy(remoteserver.time_upload,p+20,sizeof(remoteserver.time_upload)-1);//状态上传间隔4bytes
		memcpy(remoteserver.time_out,p+24,sizeof(remoteserver.time_out)-1);//通讯超时时间2bytes
	}
	else if(i == 2){//充电桩个数
		chargingpilenum = atoi(buff);
	}
	else if(i > 2){//get SN
		memcpy(sn,p,sizeof(sn));//sn 12bytes
		memcpy(port,p+12,2);//port 2bytes
		memcpy(serailnum_sn+(i-3)*14,p,14);
		serialn = (atoi(port) - 1);
		print_log(LOG_DEBUG,"serialn:%d\n",serialn);
		num_s = indexn[serialn];
 		memcpy(AllSerialSn[serialn]+num_s*sizeof(sn),sn,sizeof(sn));
		indexn[serialn] += 1;
		EverySerialNum[serialn] += 1;
		if(atoi(port) == 1){
			memcpy(p1+serial1chargnum*sizeof(sn),sn,sizeof(sn));
			serial1chargnum++;
			}
		else if(atoi(port) == 2){
			memcpy(p2+serial2chargnum*sizeof(sn),sn,sizeof(sn));
			serial2chargnum++;
		}
	}

	i++;		
}
printf("充电桩总数:%d\n串口1:充电桩数%d\nSN:",chargingpilenum,serial1chargnum);
print_asciioflen(serail1SN,serial1chargnum*12);
printf("串口2:充电桩数%d\nSN:",serial2chargnum);
print_asciioflen(serail2SN,serial2chargnum*12);
printf("充电桩SN 串口号:");
print_asciioflen(serailnum_sn,chargingpilenum*14);
for(i = 0; i < SERIALNUM;i++)
{
	print_log(LOG_DEBUG,"-串口%d  充电桩个数%d:%s\n",i+1,EverySerialNum[i],AllSerialSn[i]);
}
fclose(fp);
return 1;
}

//查找充电桩，返回其串口号
int Getserianuml_SN(char *sn,char *serialnum)
{
int i = 0;
for(i = 0;i < chargingpilenum;i++){
	if(strncmp(sn,(char *)serailnum_sn+i*14,12) == 0){
		memcpy(serialnum,serailnum_sn+i*14+12,2);
		return 0;
	}
}
return -1;
}
int readrate_parameter(const char *filename)
{
	char tmp[64] = {0},buff[64] = {0};
	int i = 1;
	FILE *fp = NULL;
	fp = fopen(filename,"r");
	char *p = NULL;
	while(1){
		memset(tmp,'\0',sizeof(tmp));
		memset(buff,'\0',sizeof(buff));
		if(fgets((char *)tmp,sizeof(tmp),fp) == NULL)
			break;
		memcpy(buff,tmp,strlen((const char *)tmp));
		/*
		   if(strlen((const char *)tmp) > 1)
		   memcpy(buff,tmp,strlen((const char *)tmp) -1);
		   else 
		   memcpy(buff,tmp,strlen((const char *)tmp));
		   */
		print_hexoflen(buff, strlen((const char *)buff));
		p = buff;
		if(i == 1){//费率个数
			memcpy(ratenum,buff,2);
			ratelist = (unsigned char *)malloc(46*atoi((char *)ratenum));
			memset(ratelist,'\0',46*atoi((char *)ratenum));
		}
		else if(i > 1){//费率内容
			memcpy(ratelist+(i-2)*46 , p , 46);//起始日期
			//memcpy(ratelist+(i-2)*34 + 8, p+8 , 8);//结束日期
			//memcpy(ratelist+(i-2)*34 + 16, p+16 , 6);//费率
			//memcpy(ratelist+(i-2)*34 + 22, p+22, 12);//费率编号
		}
		i++;		
	}
	printf("费率个数:%s\n费率内容:",ratenum);
	print_asciioflen(ratelist,46*atoi((char *)ratenum) );
	fclose(fp);
	return 1;
}

int read_blackcard(const char *filename)
{
	unsigned char tmp[64] = {0},buff[64] = {0};
	int i = 1;
	FILE *fp = NULL;
	fp = fopen(filename,"r");
	unsigned char *p = NULL;
	while(1){
		memset(tmp,'\0',sizeof(tmp));
		memset(buff,'\0',sizeof(buff));
		if(fgets((char *)tmp,sizeof(tmp),fp) == NULL)
			break;
		memcpy(buff,tmp,strlen((const char *)tmp) );//如果是最后一行，则不包含0x0a，中间行包括0x0a
		/*
		   if(strlen((const char *)tmp) > 1)
		   memcpy(buff,tmp,strlen((const char *)tmp) );//如果是最后一行，则不包含0x0a，中间行包括0x0a
		   else 
		   memcpy(buff,tmp,strlen((const char *)tmp));
		   */
		print_hexoflen((char *)buff, strlen((const char *)buff));
		p = buff;
		if(i == 1){//黑名单个数
			memcpy(blacknum,buff,5);
			blacklist = (unsigned char *)malloc(12*atoi((char *)blacknum));
			memset(blacklist,'\0',12*atoi((char *)blacknum));
		}
		else if(i > 1){//黑名单卡号
			memcpy(blacklist+(i-2)*12 , p , 12);//卡号10bytes 级别2bytes
		}
		i++;		
	}
	printf("黑名单个数:%s\n黑名单卡号:",blacknum);
	print_asciioflen(blacklist,12*atoi((char *)blacknum) );
	fclose(fp);
	return 1;
}

int writeconfigfile(const char *filename,unsigned char *buff,int len,char *writetype)
{
	FILE *fp = NULL;
	fp = fopen(filename,writetype);
	int i = 0,leftsize = len,ret = 0;
	unsigned char *p = buff;
	for(;;){
		if(leftsize > 20){
			ret = fwrite(p+i*20,20,1,fp);
			leftsize -= 20;
			i++;
		}
		else{
			ret = fwrite(p+i*20,leftsize,1,fp);
			break;
		}
	}
	fclose(fp);
	return 1;
}
/*
功能:写升级文件
filename:文件名
buff:要写的内容
len:要写的长度
flag:文件类型，0头文件，非0升级文件
*/
int writeupdatefile(const char *filename,unsigned char *buff,int len,int flag)
{
	unsigned short addr = 0;
	unsigned char *p = buff;//buff+2
	int index = 0,leftsize = len,ret = 0,i = 0;
	FILE *fp = NULL;
	
	memcpy(&addr,buff,2);//buff+2
	Rev16InByte(&addr);
	if(flag == FILEHEAD){
		fp = fopen(filename,"w+");
	}
	else{
		fp = fopen(filename,"r+");
		if(flag != FILELAST){
			index = (34+(addr-1)*1034);
		}
		else{
			index = (34+(num_updatefile-1)*1034);//最后一条文件
		}
	}
	print_log(LOG_DEBUG,"flag:%d addr:%d index:%d num_updatefile:%d\n",flag,addr,index,num_updatefile);
	fseek(fp,index,SEEK_SET);
	for(;;){
		if(leftsize > 20){
			ret = fwrite(p+i*20,20,1,fp);
			leftsize -= 20;
			i++;
		}
		else{
			ret = fwrite(p+i*20,leftsize,1,fp);
			break;
		}
	}
	
	fclose(fp);
	sync();
	return 1;
}
/*
功能:记录事件到文件里
p:事件内容结构体
index:在文本文件中位置的索引
每条事件最大长度304字节，为了编辑器浏览方便，304个字节偏移
*/
int Record_Event(char *filepath,struct event_str *p,int index)
{
	int fp = 0,datalen = 0,ret = 0,n = 0;
	unsigned char buff[320] = {0};
	fp = open(filepath,O_CREAT|O_RDWR,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if(fp < 0)
	{
		printf("fopen error:%s\n",strerror(errno));
		return -1;
	}

	if(p->e_type == 0x01)
		datalen  = 40;
	else if(p->e_type == 0x02)
		datalen = 60;
	else if(p->e_type == 0x04)
		datalen = 291;
	else
		datalen = 36;
	memcpy(buff+n,p->e_time,sizeof(p->e_time));
	n += sizeof(p->e_time);
	memcpy(buff+n,&(p->e_dataid),sizeof(p->e_dataid));
	n += sizeof(p->e_dataid);
	buff[n] = p->e_upflag;
	n += 1;
	memcpy(buff+n,p->e_sn,sizeof(p->e_sn));
	n += sizeof(p->e_sn);
	buff[n] = p->e_type;
	n += sizeof(p->e_type);
	memcpy(buff+n,p->e_data,datalen);
	n += datalen;

	lseek(fp,index*320,SEEK_SET);
	print_log(LOG_DEBUG,"------------Record_Event-----------------n:%d\n",n);
	print_hexoflen(buff,n);
	ret = write(fp,buff,n);
	sync();

	close(fp);
	return 0;
}
/*
功能:设置事件的上传标志到文件
Dataid:通讯序号，用来区分不同的通讯帧，连续递增
通过该参数确定事件在文件里的偏移量
*/
int Set_EventFlag(char *filepath,unsigned int Dataid)
{
	int fp = 0,ret = 0,i = 0;
	unsigned char buff[320] = {0},flag[2] = {0};
	fp = open(filepath,O_CREAT|O_RDWR,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if(fp < 0)
	{
		printf("fopen error:%s\n",strerror(errno));
		return -1;
	}

	for(i = 0; i < MAXEVENT;i++){
		lseek(fp,i*320,SEEK_SET);
		ret = read(fp,buff,320);
		if(memcmp(&Dataid,(buff+4),sizeof(Dataid)) == 0){
			print_log(LOG_DEBUG,"Set_EventFlag success i = %d\n",i);
			flag[0] = UPYES;
			lseek(fp,(i*320+8),SEEK_SET);
			ret = write(fp,flag,1);
			print_log(LOG_DEBUG,"Set_EventFlag\n");
			break;
		}
	}
	print_log(LOG_DEBUG,"Set_EventFlag fail\n");
	sync();
	close(fp);
	return 0;
}



/*
功能:记录事件到内存空间
p:事件内容结构体
index:在文本文件中位置的索引
每条事件最大长度304字节，为了编辑器浏览方便，304个字节偏移
*/
int Record_Event_Mem(char *src,struct event_str *p,int index)
{
	int datalen = 0,n = 0;
	unsigned char buff[320] = {0};
	

	if(p->e_type == 0x01)
		datalen  = 40;
	else if(p->e_type == 0x02)
		datalen = 60;
	else if(p->e_type == 0x04)
		datalen = 291;
	else
		datalen = 36;
	memcpy(buff+n,p->e_time,sizeof(p->e_time));
	n += sizeof(p->e_time);
	memcpy(buff+n,&(p->e_dataid),sizeof(p->e_dataid));
	n += sizeof(p->e_dataid);
	buff[n] = p->e_upflag;
	n += 1;
	memcpy(buff+n,p->e_sn,sizeof(p->e_sn));
	n += sizeof(p->e_sn);
	buff[n] = p->e_type;
	n += sizeof(p->e_type);
	memcpy(buff+n,p->e_data,datalen);
	n += datalen;
	memcpy(src+(index*320),buff,n);
	sync();

	return 0;
}
/*
功能:设置事件的上传标志到内存
Dataid:通讯序号，用来区分不同的通讯帧，连续递增
通过该参数确定事件在文件里的偏移量
*/
int Set_EventFlag_Mem(unsigned char *src,unsigned int Dataid)
{
	int index = 0,i = 0;
	for(i = 0; i < MAXEVENT;i++){
		index = (i*320+8);
		if(memcmp(&Dataid,(src+index),sizeof(Dataid)) == 0){
			src[index] = UPYES;
			print_log(LOG_DEBUG,"Set_EventFlag_Mem\n");
			break;
		}
	}
	return 0;
}


/*
功能:记录充电桩状态到文件里
p:充电桩状态结构体
index:在文本文件中位置的索引
portnum:属于几号串口，每个串口偏移32*96=3072
*/
int Record_Status(char *filepath,struct status*p,int index,int portnum)
{
	int fp = 0,ret = 0,n = 0,seeknum = 0;
	
	fp = open(filepath,O_CREAT|O_RDWR,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if(fp < 0)
	{
		printf("fopen error:%s\n",strerror(errno));
		return -1;
	}
	n = sizeof(struct status);
	seeknum = ((index*sizeof(struct status))+(portnum-1)*3072);
	print_log(LOG_DEBUG,"Record_Status seeknum:%d n:%d SCT:%02x %02x %02x %02x %02x %02x %02x\n",seeknum,n,p->SCT[0],p->SCT[1],p->SCT[2],p->SCT[3],p->SCT[4],p->SCT[5],p->SCT[6]);
	lseek(fp,seeknum,SEEK_SET);
	ret = write(fp,p,n);
	sync();
	close(fp);
	return 0;
}

/*
功能:从文件里读充电桩状态到内存
p:充电桩状态结构体
index:在文本文件中位置的索引
*/
int Read_Status(char *filepath,struct status*p,int index,int portnum)
{
	int fp = 0,ret = 0,n = 0,seeknum = 0;
	char piplesn[12] = {0};
	memcpy(piplesn,p->SN,sizeof(p->SN));
	fp = open(filepath,O_RDWR,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if(fp < 0)
	{
		printf("fopen error:%s\n",strerror(errno));
		return -1;
	}
	n = sizeof(struct status);
	seeknum = ((index*sizeof(struct status))+(portnum-1)*3072);
	print_log(LOG_DEBUG,"Read_status seeknum:%d\n",seeknum);
	lseek(fp,seeknum,SEEK_SET);
	ret = read(fp,p,n);
	memcpy(p->SN,piplesn,sizeof(p->SN));
	close(fp);
	return 0;
}

/*
功能:从文件里指定位置开始读指定字节的内容到内存里
dst:目标内存
index:在文本文件中位置的索引
num:指定字节数
filepath:文件全路径名
*/
int Read_Nbytes(char *dst,int index,int num,char *filepath)
{
	int fp = 0,ret = 0;
	fp = open(filepath,O_RDWR,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if(fp < 0)
	{
		printf("fopen error:%s\n",strerror(errno));
		return -1;
	}
	//print_log(LOG_DEBUG,"Read_status seeknum:%d\n",seeknum);
	lseek(fp,index,SEEK_SET);
	ret = read(fp,dst,num);
	close(fp);
	return num;
}







