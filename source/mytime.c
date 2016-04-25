#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <unistd.h>
#include <string.h>

void BCDtoChar( char src,unsigned char *dst)
{
	dst[0] = ((src >> 4)&0x0f ) + 0x30;
	dst[1] = (src&0x0f) + 0x30;
}
void BCDtoStr(unsigned char *src,int len,unsigned char *dst)
{
	//0x20150818151300
	int i = 0;
	for(i = 0;i < len;i++){
		dst[2*i] = ((src[i] >> 4)&0x0f ) + '0';
		dst[2*i+1] = (src[i]&0x0f) + '0';
	}
}
void Rev32InByte(void *val)
{
	unsigned int v = *((unsigned int *)val);
	v = ((v & 0x000000ff) << 24) | ((v & 0x0000ff00) << 8) |((v & 0x00ff0000) >> 8) |((v & 0xff000000)>>24);
	*((unsigned int *)val) = v;
}
void Rev16InByte(void *val)
{
	unsigned short v = *((unsigned short *)val);
	v = ((v & 0x00ff) << 8) |((v & 0xff00) >> 8);
	*((unsigned short *)val) = v;

}
int CharToInt(char *s,int len)
{
	char buff[20];
	memset(buff,0,20);
	memcpy(buff,s,len);
	return atoi(buff);
}
void StrtoBCD(unsigned char *src,int len,unsigned char *dst)
{
	int i = 0,j = 0;
	for(i = 0,j = 0;i < len;i++){
		if(i %2 == 0)
			dst[j] += ((src[i] -'0')<<4);
		else if(i %2 == 1){
			dst[j] += src[i] - '0';
			j++;
		}
	}
	return ;
}
void Strtotm(unsigned char *src,struct tm *dst)
{
	char buff[5] = {0};
	memcpy(buff,src,4);//year
	dst->tm_year = (atoi(buff)-1900);

	memset(buff,'\0',sizeof(buff));
	memcpy(buff,src+4,2);//month
	dst->tm_mon = (atoi(buff)-1);
	
	memset(buff,'\0',sizeof(buff));
	memcpy(buff,src+6,2);//day
	dst->tm_mday = atoi(buff);
	
	memset(buff,'\0',sizeof(buff));
	memcpy(buff,src+8,2);//hour
	dst->tm_hour = atoi(buff);
	
	memset(buff,'\0',sizeof(buff));
	memcpy(buff,src+10,2);//min
	dst->tm_min = atoi(buff);
	
	memset(buff,'\0',sizeof(buff));
	memcpy(buff,src+12,2);//send
	dst->tm_sec = atoi(buff);
}
//20150908132200
time_t time14ToInt(char *s)
{
	time_t now;
	struct tm tm_now;
	time(&now);
	localtime_r(&now,&tm_now);
	tm_now.tm_year=CharToInt(s,4)-1900;
	tm_now.tm_mon=CharToInt(s+4,2)-1;
	tm_now.tm_mday=CharToInt(s+6,2);
	tm_now.tm_hour=CharToInt(s+8,2);
	tm_now.tm_min=CharToInt(s+10,2);
	tm_now.tm_sec=CharToInt(s+12,2);
	tm_now.tm_isdst = -1; /* Be sure to recheck dst. */
	now = mktime(&tm_now);
	return now;
}
void timet2str(char *dst)
{
	time_t t;
	struct tm *tm1;
	t = time(NULL);
	tm1 = localtime(&t);
	snprintf(dst,15,"%04d%02d%02d%02d%02d%02d",tm1->tm_year+1900,tm1->tm_mon+1,tm1->tm_mday,tm1->tm_hour,tm1->tm_min,tm1->tm_sec);
	return ;
}

void setTime(char *s)
{
	int rtc;
	time_t now;
	struct tm tm_now;
	now=time14ToInt(s);
	if(stime(&now)<0) return;
	rtc=open("/dev/rtc0",O_WRONLY);
	if(rtc<0) return;
	time(&now);
	localtime_r(&now,&tm_now);
	tm_now.tm_isdst = 0;
	ioctl ( rtc, RTC_SET_TIME, &tm_now);
	close(rtc);
	return;
}


#if 0
int main(void)
{
	unsigned char src[] = {0x20,0x15,0x08,0x18,0x15,0x33,0x00};
	int i = 0;
	unsigned char dst[14] = {0};
	memset(dst,'\0',sizeof(dst));
	BCDtoChar(0x20,dst);
	printf("%s\n",dst);
	BCDtoStr(src, 7,dst);
	printf("%.14s\n",dst);
	unsigned char buf[4] = {0};
	StrtoBCD("20150824", 8, buf);
	for(i = 0;i < 4;i++)
		printf("%02x ",buf[i]);
	
	return 0;
}
#endif
