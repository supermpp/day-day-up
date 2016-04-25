#include <stdio.h>
#include <string.h>
#include "rc4.h"
void RC4(RC4_KEY *key, size_t len, const unsigned char *indata,unsigned char *outdata)
{
	// d ��ʾ key ��ֵ��key ���ȹ̶�Ϊ 256 �� int ����
	register RC4_INT *d;
	// x��y ��ʾ 8 bits ����
	// tx��ty ������ʱ���� key �е�ֵ��������룩
	register RC4_INT x,y,tx,ty;
	// ��ǰ���ڴ������ĺ����ĵģ�λ�õ�����
	size_t i;

	x=key->x;
	y=key->y;
	d=key->data; 

	// �м��Թ� RC4_CHUNK ��ش�������

	// LOOP ������� in��1 byte������Ϊ out��1 byte��
#define LOOP(in,out) x=((x+1)&0xff);  tx=d[x]; y=(tx+y)&0xff; d[x]=ty=d[y]; d[y]=tx; (out) = d[(tx+ty)&0xff]^ (in);

#ifndef RC4_INDEX
#define RC4_LOOP(a,b,i)	LOOP(*((a)++),*((b)++))
#else
#define RC4_LOOP(a,b,i)	LOOP(a[i],b[i])
#endif

	i=len>>3;
	// ������ĳ��ȴ��ڵ��� 8 bytes
	if (i)
	{
		// һ�δ��� 8 bytes
		for (;;)
		{
			RC4_LOOP(indata,outdata,0);
			RC4_LOOP(indata,outdata,1);
			RC4_LOOP(indata,outdata,2);
			RC4_LOOP(indata,outdata,3);
			RC4_LOOP(indata,outdata,4);
			RC4_LOOP(indata,outdata,5);
			RC4_LOOP(indata,outdata,6);
			RC4_LOOP(indata,outdata,7);
#ifdef RC4_INDEX
			indata+=8;
			outdata+=8;
#endif
			if (--i == 0) break;
		}
	}
	// ����ʣ��ģ�С�� 8 bytes ������
	i=len&0x07;
	if (i)
	{
		for (;;)
		{
			RC4_LOOP(indata,outdata,0); if (--i == 0) break;
			RC4_LOOP(indata,outdata,1); if (--i == 0) break;
			RC4_LOOP(indata,outdata,2); if (--i == 0) break;
			RC4_LOOP(indata,outdata,3); if (--i == 0) break;
			RC4_LOOP(indata,outdata,4); if (--i == 0) break;
			RC4_LOOP(indata,outdata,5); if (--i == 0) break;
			RC4_LOOP(indata,outdata,6); if (--i == 0) break;
		}
	}
	key->x=x;
	key->y=y;
}


void RC4_set_key(RC4_KEY *key, int len, const unsigned char *data)
{
	register RC4_INT tmp;
	register int id1,id2;
	// ָ�����ɵ� key �ľ�������
	register RC4_INT *d;
	unsigned int i;

	d= &(key->data[0]);
	key->x = 0;
	key->y = 0;
	id1=id2=0;     

	// ���� SK_LOOP �� &0xff �����京��Ϊ % 256
	// ���� A % B ��� B Ϊ 2 �� x ���ݣ���ô A % B == A & (B - 1)
	//
	// SK_LOOP �������� RC4 key �ĵ� n λ��ֵ
#define SK_LOOP(d,n) { tmp=d[(n)];  id2 = (data[id1] + tmp + id2) & 0xff;  if (++id1 == len) id1=0; d[(n)]=d[id2]; d[id2]=tmp; }

	// ��ȥ���ִ���

	for (i=0; i < 256; i++) d[i]=i;
	// һ�μ��� 4 �� int
	// ������Ե�֪������ key ���㷨��Ч�ʺͲ��� data �ĳ����޹�
	for (i=0; i < 256; i+=4)
	{
		SK_LOOP(d,i+0);
		SK_LOOP(d,i+1);
		SK_LOOP(d,i+2);
		SK_LOOP(d,i+3);
	}
}


void rc4DE(char *buff,int len)
{
	RC4_KEY key;
	unsigned char keystr[20]="KingsWayCo.,Ltd.";

	RC4_set_key(&key,strlen((char *)keystr),keystr);
	RC4(&key,(unsigned int)len,(unsigned char *)buff,(unsigned char *)buff);
	return;
}
