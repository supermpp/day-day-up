#ifndef _mytime_h
#define _mytime_h
#include <stdio.h>

void BCDtoChar( char src,unsigned char *dst);
void BCDtoStr(unsigned char *src,int len,unsigned char *dst);
void Strtotm(unsigned char *src,struct tm *dst);
int CharToInt(char *s,int len);
time_t time14ToInt(char *s);
void setTime(char *s);
void StrtoBCD(unsigned char *src,int len,unsigned char *dst);
void Rev32InByte(void *val);
void Rev16InByte(void *val);
#endif
