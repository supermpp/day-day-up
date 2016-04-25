#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>   
#include <sys/ioctl.h>
#include <sys/io.h>
#include <sys/user.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <time.h>
#include <linux/rtc.h>
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>   
#include <sys/ioctl.h>
#include <sys/io.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <dirent.h>
#include <time.h>
#include <linux/rtc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/watchdog.h>
#include <linux/ethtool.h>
#include <pthread.h>

#include "common.h"

static int default_log_level;


void getmac(const char *filename,char *mac)
{
	char buff[256] = {0},*p = NULL;
	FILE *fd = NULL;
	fd = fopen(filename,"r");
	while(fgets(buff,sizeof(buff),fd) != NULL)
	{
		if((p = strstr(buff,"hwaddr=")) != NULL)
		{
			memcpy(mac,p+7,17);
			
		}
		memset(buff,'\0',sizeof(buff));
	}
	fclose(fd);
}

void s2bcd(void *d, void *s,const int len) 
{
	int i;
	unsigned char a,b;
	unsigned char *dest,*src;
	dest=(unsigned char *)d;
	src=(unsigned char *)s;
	for(i=0; i<len;i++)              /*"65"-->0x65*/
	{
		a=src[2*i]<='9'?src[2*i]:src[2*i]-'A'+0x0a;
		b=src[2*i+1]<='9'?src[2*i+1]:src[2*i+1]-'A'+0x0a;
		dest[i]   = ((a&0x0f)<<4)|(b&0x0f);
	}
	return;
}

void mactoint(char *dst,char *mac)
{
	int i = 0,j = 0;
	char buff[8] = {0};
	for(i = 0,j = 0;i < 11;i++)
	{
		if(mac[i] != ':')
			{
			buff[j++] = mac[i+6];
			}
	}
	s2bcd(dst,buff,8);
}


int getTime(void *d,void *t,struct tm *mytm)
{
	char buff[10];
	time_t now;
	struct tm tm_now;
	if (mytm!=NULL) tm_now=*mytm;
	else
	{
		time(&now);
		localtime_r(&now,&tm_now);
	}

	if (d!=NULL) strftime((char *)d,9,"%Y%m%d",&tm_now);
	if (t!=NULL)  strftime((char *)t,7,"%H%M%S",&tm_now);
	strftime(buff,2,"%u",&tm_now);
	return atoi(buff);
}

char *stringTrim(char *source)
{
	char *pBegin;
	char *pEnd;
	int  len;

	if (NULL == source || strlen(source) == 0)
	{
		return NULL;
	}

	pBegin = source;
	pEnd = source + strlen(source) - 1;
	while (isspace(*pBegin) && (*pBegin != '\0'))
	{
		pBegin ++;
	}
	while (isspace(*pEnd) && (pEnd > pBegin))
	{
		pEnd --;
	}

	len = (int)(pEnd - pBegin + 1);
	strncpy(source,pBegin,len);
	source[len] = '\0';

	return source;
}

int stringAna( char s_buffer[] )
{
	char ch;
	int ret = 0;

	while(*s_buffer!=0)
	{
		ch = *s_buffer;
		s_buffer++;
		if ( ch!='_')
		{
			if ( isalnum(ch)==0 )
			{
				ret = -1;
				break;
			}
		}
	}
	return ret;
}

unsigned char bin2ascii(unsigned char bin,char high)
{
	unsigned char h = (bin & 0xf0) >> 4, l = bin & 0x0f;

	if (h<=9) h += 0x30; else h = 0x41 + h - 0x0a;
	if (l<=9) l += 0x30; else l = 0x41 + l - 0x0a;

	if(high=='H') return h;
	else return l;
}

unsigned char twoAsc2Byte(unsigned char str1,unsigned char str2)
{
	unsigned char s_Ret=0x00;

	if(str1<=0x39) str1=str1-0x30;
	else str1=str1-0x41+0x0A;

	if(str2<=0x39) str2=str2-0x30;
	else str2=str2-0x41+0x0A;

	s_Ret=((str1& 0x0f)<<4)+(str2 & 0x0f);
	return s_Ret;
}

float eightAsc2Float(unsigned char str1,unsigned char str2,unsigned char str3,unsigned char str4,
		unsigned char str5,unsigned char str6,unsigned char str7,unsigned char str8)
{
	float ret;
	unsigned char *p;

	p=(unsigned char *)&ret;

	p[0]=twoAsc2Byte(str1,str2);
	p[1]=twoAsc2Byte(str3,str4);
	p[2]=twoAsc2Byte(str5,str6);
	p[3]=twoAsc2Byte(str7,str8);

	return ret;
}

int Write_Disk(int fd,void *buff,int len)
{
	int m,n,k;
	char *p;
	k=0;
	n=len;
	p=(char *)buff;
	while(1)
	{
		m=write(fd,p+k,n);
		if(m==n) return (k+n);
		else
		{
			if(m>0)
			{
				//print_log_wg104(LOG_INFO,"Write_Disk will write %d bytes,but writed %d\n",n,m);
				k+=m;
				n=n-m;
			}
			else
			{
				if(errno==EINTR) 
				{
					//print_log_wg104(LOG_INFO,"Write_Disk is interrupted \n");
					continue;
				}
				//print_log_wg104(LOG_ERR,"Write_Disk Error--%m\n");
				if(k==0) return -1;
				else return k;
			}
		}

	}
}

int Read_Disk(int fd,void *buff,int len)
{
	int m,n,k;
	char *p;
	k=0;
	n=len;
	p=(char *)buff;
	while(1)
	{
		m=read(fd,p+k,n);
		if(m==n) return (k+n);
		else
		{
			if(m>0)
			{
				//print_log_wg104(LOG_INFO,"Read_Disk will read %d bytes,but read %d\n",n,m);
				k+=m;
				n=n-m;
			}
			else
			{
				if(m==0 && k==0) return 0;
				if(errno==EINTR)
				{
					//print_log_wg104(LOG_INFO,"Read_Disk is interrupted \n");
					continue;
				}
				//print_log_wg104(LOG_ERR,"Read_Disk Error--%m\n");
				if(k==0) return -1;
				else return k;
			}
		}

	}
}



void set_log_level(int level)
{
	default_log_level=level;
	return;
}

void print_log(int level,const char * format, ... )
{
	va_list ap;
	char d[10],t[10];
	struct timeval time_1;
	struct timezone time_zone;
	struct tm tm_now;
	//static pthread_mutex_t log_lock=PTHREAD_MUTEX_INITIALIZER;

	if (default_log_level<level) return;

	//	while(pthread_mutex_lock(&log_lock)!=0);
	gettimeofday(&time_1,&time_zone);
	localtime_r(&time_1.tv_sec,&tm_now);
	strftime(d,9,"%Y%m%d",&tm_now);
	strftime(t,7,"%H%M%S",&tm_now);
	//给打印的信息添加时间戳
	//printf("[%.4s-%.2s-%s %.2s:%.2s:%s.%03ld]  ",d,d+4,d+6,t,t+2,t+4,time_1.tv_usec/1000L);

	va_start(ap,format);
	vprintf(format,ap);
	va_end(ap);
	//	while(pthread_mutex_unlock(&log_lock)!=0);
	return;
}

void write_log_time(int level,const char * format, ... )
{
	if(level > default_log_level)
		return;
	va_list ap;
	char d[10],t[10];
	struct timeval time_1;
	struct timezone time_zone;
	struct tm tm_now;

	gettimeofday(&time_1,&time_zone);
	localtime_r(&time_1.tv_sec,&tm_now);
	strftime(d,9,"%Y%m%d",&tm_now);
	strftime(t,7,"%H%M%S",&tm_now);
	char buff[1024*10] = {0};
	sprintf(buff,"[%.4s-%.2s-%s %.2s:%.2s:%s.%03ld] ",d,d+4,d+6,t,t+2,t+4,time_1.tv_usec/1000L);

	FILE *fp;
	fp = fopen(LOGDEBUGPATH,"a+");
	if(fp == NULL){
		printf("error:%s",strerror(errno));
	}
	fwrite(buff,strlen(buff),1,fp);	

	va_start(ap,format);
	memset(buff,0,sizeof(buff));
	vsprintf(buff,format,ap);
	fwrite(buff,strlen(buff),1,fp);	
	va_end(ap);
	fclose(fp);
	return;
}


void record_syslog(const char *path,const char *file1,const char *file2,int maxsize,const char * format, ... )
{
	va_list ap;
	char d[10],t[10];
	struct timeval time_1;
	struct timezone time_zone;
	struct tm tm_now;
	struct stat f_stat1,f_stat2;
	static int lognum = 1,logaccess = 0;
	int ret;
	if(logaccess == 0){
		if (file1 !=NULL && file2 !=NULL){
			if( (access(file1,F_OK) == -1)&&(access(file2,F_OK) == -1)){
				printf("%s %s is not exist\n",file1,file2);
				ret = mkdir(path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
				logaccess = 1;
			}
		}
	}
	gettimeofday(&time_1,&time_zone);
	localtime_r(&time_1.tv_sec,&tm_now);
	strftime(d,9,"%Y%m%d",&tm_now);
	strftime(t,7,"%H%M%S",&tm_now);
	char buff[1024*10] = {0};
	sprintf(buff,"[%.4s-%.2s-%s %.2s:%.2s:%s.%03ld] ",d,d+4,d+6,t,t+2,t+4,time_1.tv_usec/1000L);

	FILE *fp = NULL;
	f_stat1.st_size = 0;
	f_stat2.st_size = 0;
	stat(file1,&f_stat1);
	stat(file2,&f_stat2);
	if(f_stat1.st_size > maxsize){
		if(f_stat2.st_size > maxsize){
			if(lognum == 1){
				fp = fopen(file1,"w");
				if(fp == NULL){
					printf("error:%s",strerror(errno));
				}
				lognum = 2;
			}
			else if(lognum == 2){
				fp = fopen(file2,"w");
				if(fp == NULL){
					printf("error:%s",strerror(errno));
				}
				lognum = 1;
			}
		}
		else{
			fp = fopen(file2,"a");
			if(fp == NULL){
				printf("error:%s",strerror(errno));
			}
		}
	}
	else{
		fp = fopen(file1,"a");
		if(fp == NULL){
			printf("error:%s",strerror(errno));
		}
	}
	fwrite(buff,strlen(buff),1,fp);	
	va_start(ap,format);
	memset(buff,0,sizeof(buff));
	vsprintf(buff,format,ap);
	fwrite(buff,strlen(buff),1,fp);	
	va_end(ap);
	fclose(fp);
	return;
}

void print_time(void)
{
	char d[10]= {0},t[10]= {0};
	if(debug_strq== 0)
		return ;
	struct timeval time_1;
	struct timezone time_zone;
	struct tm tm_now;
	gettimeofday(&time_1,&time_zone);
	localtime_r(&time_1.tv_sec,&tm_now);
	strftime(d,9,"%Y%m%d",&tm_now);	
	strftime(t,7,"%H%M%S",&tm_now);	
	//给打印的信息添加时间戳	
	printf("[%.4s-%.2s-%s %.2s:%.2s:%s.%03ld]\n",d,d+4,d+6,t,t+2,t+4,time_1.tv_usec/1000L);
	return ;
}

void Packetrecv_debug(const unsigned char *src,int len)
{
	int i = 0;
	if(debug_packet == 0)
		return;
	
	printf("socket recv %d bytes [cmdid:%02x][Hex]:",len,src[4]);
	for(i = 0; i < len;i++)
		printf("%02x ",src[i]);
	printf("\n");
	
	return ;
}
void Packetsend_debug(const unsigned char *src,int len)
{
	int i = 0;
	if(debug_packet == 0)
		return;
	
	printf("socket send %d bytes [cmdid:%02x][Hex]",len,src[4]);
	for(i = 0; i < len;i++)
		printf("%02x ",src[i]);
	printf("\n");
	
	return ;
}

void StatusRequest_senddebug(const unsigned char *src,int len,char parm)
{
	int i = 0;
	if(debug_strq == 0)
		return;
	
	printf("StatusRequest_debug send %d bytes [parm:%02x][Hex]:",len,parm);
	for(i = 0; i < len;i++)
		printf("%02x ",src[i]);
	printf("\n");
	
	return ;
}	
void StatusRequest_recvdebug(const unsigned char *src,int len,char parm)
{
	int i = 0;
	if(debug_strq == 0)
		return;
	
	printf("StatusRequest_debug recv %d bytes [parm:%02x][Hex]:",len,parm);
	for(i = 0; i < len;i++)
		printf("%02x ",src[i]);
	printf("\n");
	
	return ;
}	
void Serialrecv_debug(const unsigned char *src,int len,char parm)
{
	int i = 0;
	if(debug_packet == 0)
		return;
	
	printf("serial recv %d bytes [parm:%02x][Hex]:",len,parm);
	for(i = 0; i < len;i++)
		printf("%02x ",src[i]);
	printf("\n");
	
	return ;
}

void Serialsend_debug(const unsigned char *src,int len,char parm)
{
	int i = 0;
	if(debug_packet == 0)
		return;
	
	printf("serial send %d bytes [parm:%02x][Hex]",len,parm);
	for(i = 0; i < len;i++)
		printf("%02x ",src[i]);
	printf("\n");
	
	return ;
}
void print_hexoflen(const char *src,int len)
{
	int i = 0;
	if(debug_packet == 0)
		return;
	printf("Hex:");
	for(i = 0;i <  len;i++)
		printf("%02x ",src[i]);
	printf("\n");
	return ;
}
//用于打印一些定长的字符串
void print_asciioflen(const unsigned char *src,int len)
{
	int i = 0;
	if(debug_packet == 0)
		return;
	printf("ascii:");
	for(i = 0;i <  len;i++)
		printf("%c",src[i]);
	printf("\n");
	return ;
}


// if_name like "ath0", "eth0". Notice: call this function

// need root privilege.

// return value:

// -1 -- error , details can check errno

// 1 -- interface link up

// 0 -- interface link down.

int get_netlink_status(const char *if_name)

{

	int skfd;

	struct ifreq ifr;

	struct ethtool_value edata;

	edata.cmd = ETHTOOL_GLINK;

	edata.data = 0;

	memset(&ifr, 0, sizeof(ifr));

	strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);

	ifr.ifr_data = (char *) &edata;

	if (( skfd = socket( AF_INET, SOCK_DGRAM, 0 )) == 0)

		return -1;

	if(ioctl( skfd, SIOCETHTOOL, &ifr ) == -1)

	{

		close(skfd);

		return -1;

	}

	close(skfd);

	return edata.data;

}
void getversion(char *filename,char *version)
{
	//v1.0_201504201031.tar
	strncpy(version,filename,18);
}
/*
   ret 0 update  
   -1 no update
   */
int checkversion(char *oldversion,char *newversion)
{
	//v1.00_201504161340
	write_log_time(LOG_ERR, "old:%s\nnew:%s\n",oldversion,newversion);
	char o_branch[10] = {0},o_timeflag[15] = {0}, n_branch[10] = {0},n_timeflag[15] = {0};
	int i = 0;
	char *ptr = oldversion,*ptr1 = newversion;
	int flag = 0;
	while(*ptr != '\0'){
		if(*ptr != '_'){
			if(flag == 0){
				o_branch[i++] = *ptr;
			}
			else{
				o_timeflag[i++] = *ptr;
			}
			ptr++;	
		}
		else{
			if(flag == 0){
				o_branch[i++] = '\0';
				ptr++;
				i = 0;
				flag = 1;
			}
		}
	}
	o_timeflag[i] = '\0';
	i = 0;
	flag = 0;
	while(*ptr1 != '\0'){
		if(*ptr1 != '_'){
			if(flag == 0){
				n_branch[i++] = *ptr1;
			}
			else{
				n_timeflag[i++] = *ptr1;
			}
			ptr1++;	
		}
		else{
			if(flag == 0){
				n_branch[i++] = '\0';
				ptr1++;
				i = 0;
				flag = 1;
			}
		}

	}
	n_timeflag[i] = '\0';
	write_log_time(LOG_ERR,"%s-%s-%s-%s\n",o_branch,n_branch,o_timeflag,n_timeflag);
	if((strcmp(o_branch,n_branch) == 0)&&(strcmp(o_timeflag,n_timeflag) < 0))
		return 0;
	return -1;
}
/*
   return 	0 更新成功
   2无需更新程序
   -1更新失败

*/
#if 0
int recvupdatetag(int *fd,char *str,char *outfilename,char *mac)
{
	char versionstr[64] = {0},filename[32] = {0},tmp[32] = {0},newversion[32] = {0},oldversion[32] = {0},buff[1024] = {0};
	int ret,flag  = 0,countsize = 0;
	int tcpsockfd = *fd;
	FILE *nfd = NULL;
	write_log_time(LOG_ERR,"str:%s\n",str);
	strncpy(versionstr,str,18);
	strncat(versionstr,"_",1);
	strncat(versionstr,mac,17);
	ret = tcp_send(tcpsockfd,versionstr,strlen(versionstr));
	write_log_time(LOG_ERR,"versionstr:%s\ntcp_send ret:%d want send:%d\n",versionstr,ret,strlen(versionstr));

	if(ret == strlen(versionstr)){
		while(1){
			if(flag == 0){//接收更新包文件名
				ret = tcp_recv(tcpsockfd,filename,22,1000000);//v1.00_201504201031.tar
				write_log_time(LOG_ERR,"-filename:%s\n",filename);
				if(ret == 22){
					getversion(filename,newversion);
					write_log_time(LOG_ERR,"filename:%s\nnewversion:%s\nversionstr:%s\n",filename,newversion,versionstr);
					strncpy(oldversion,versionstr,18);
					if(checkversion(oldversion,newversion) == 0){
						flag = 1;
						strncpy(tmp,filename,22);
						memset(filename,'\0',sizeof(filename));
						sprintf(filename,"/%s",tmp);
						strcpy(outfilename,filename);
						write_log_time(LOG_ERR,"filename:%s\n",filename);
						nfd = fopen(filename,"wb+");
						if(fd == NULL){
							close(tcpsockfd);
							return -1;
						}
					}
					else{
						write_log_time(LOG_ERR,"no update\n");
						close(tcpsockfd);
						return 2;
					}
				}
				else{
					close(tcpsockfd);
					return -1;
				}
			}
			else{//接收文件内容
				memset(buff,'\0',sizeof(buff));
				ret = tcp_recv(tcpsockfd, buff, sizeof(buff), 1000000);
				if(ret >= 0){
					write_log_time(LOG_ERR,"tcp_recv:%d ",ret);
					ret = fwrite(buff,sizeof(char),ret,nfd);
					write_log_time(LOG_ERR,"fwrite:%d \n",ret);
					sync();
					countsize += ret;
				}
				else{//接收完毕
					fclose(nfd);
					//close(tcpsockfd);
					sync();
					write_log_time(LOG_ERR,"recv ok\n");
					break;
				}
			}
		}
	}
	else{
		close(tcpsockfd);
		return -1;
	}
	write_log_time(LOG_ERR,"countsize:%d\n",countsize);
	//exit(-1);
	//close(tcpsockfd);
	return 0;

}
#endif
#if 0
void *checkupdate(void *arg)
{
	int sockfd = 0,ret = 0;
	struct mytcpser *server;
	server = (struct mytcpser *)arg;
	char filename[32] = {0};
	char shellcmd[128] = {0};
	//server.ip  server.version 
	while(1){
		sockfd = 0;
		ret = NonblockTcpNetconnect(&sockfd,server->serverip,server->serverport+ 10000,0);
		printf("checkupdate------%d------%s------%d--------%s-----%s\n",server->serverport,server->serverip,server->serverport+10000,server->version,server->wgmac);
		for( ; ;){
			if(ret < 0){
				ret =  NonblockTcpNetconnect(&sockfd,server->serverip,server->serverport + 10000,0);
				usleep(2000*1000);
			}
			else
				break;
		}
		ret = recvupdatetag(&sockfd,server->version,filename,server->wgmac);
		write_log_time(LOG_ERR,"recvupdatetag ret:%d\n",ret);
		if(ret == 0){//成功，重启网关
			ret = close(sockfd);
			write_log_time(LOG_ERR,"close ret:%d\n",ret);
			sprintf(shellcmd,"tar -xvf %s -C /",filename);
			write_log_time(LOG_ERR,"shellcmd:%s\n",shellcmd);
			system(shellcmd);
			write_log_time(LOG_ERR,"shellcmd:%s\n",shellcmd);
			sync();
			sleep(1000);
			system("reboot");
		}
		else{//失败，过30s  重连重新建立连接
			sleep(10);
			continue;
		}


	}
}
#endif

