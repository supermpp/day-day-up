#ifndef _common_h
#define _common_h
#include <stdio.h>
#include <time.h>

#define LOG_OFF      0
#define LOG_ERR	  1
#define LOG_INFO    2
#define LOG_DEBUG 3

#define LOGDEBUGPATH "/debuglog.txt"

#define LOGSYSPATH "/log"
#define LOGSYSFILE1 "/log/log1"
#define LOGSYSFILE2 "/log/log2"
#define MAXSIZESYSLOG (2*1024*1024)

extern int debug_packet ;
extern int debug_strq;

void mactoint(char *dst,char *mac);
void s2bcd(void *d, void *s,const int len);
void getmac(const char *filename,char *mac);
void set_log_level(int level);
void print_log(int level,const char * format, ... );
void write_log_time(int level,const char * format, ... );
void record_syslog(const char *path,const char *file1,const char *file2,int maxsize,const char * format, ... );
void Packetrecv_debug(const unsigned char *src,int len);
void Packetsend_debug(const unsigned char *src,int len);
void Serialrecv_debug(const unsigned char *src,int len,char parm);
void Serialsend_debug(const unsigned char *src,int len,char parm);
void StatusRequest_senddebug(const unsigned char *src,int len,char parm);
void StatusRequest_recvdebug(const unsigned char *src,int len,char parm);

void print_hexoflen(const char *src,int len);
void print_asciioflen(const unsigned char *src,int len);

int getTime(void *d,void *t,struct tm *mytm);
char *stringTrim(char *source);
int stringAna( char s_buffer[] );
unsigned char bin2ascii(unsigned char bin,char high);
unsigned char twoAsc2Byte(unsigned char str1,unsigned char str2);
float eightAsc2Float(unsigned char str1,unsigned char str2,unsigned char str3,unsigned char str4,
				unsigned char str5,unsigned char str6,unsigned char str7,unsigned char str8);
int Write_Disk(int fd,void *buff,int len);
int Read_Disk(int fd,void *buff,int len);
int get_netlink_status(const char *if_name);
void *checkupdate(void *arg);
int recvupdatetag(int *fd,char *str,char *outfilename,char *mac);
void print_time(void);

#endif


