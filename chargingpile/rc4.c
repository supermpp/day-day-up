#include <stdio.h>
#include <string.h>
#include "rc4.h"
void RC4(RC4_KEY *key, size_t len, const unsigned char *indata,unsigned char *outdata)
{
	// d 表示 key 的值（key 长度固定为 256 个 int 长）
	register RC4_INT *d;
	// x，y 表示 8 bits 索引
	// tx，ty 用于临时保存 key 中的值（详见代码）
	register RC4_INT x,y,tx,ty;
	// 当前正在处理（明文和密文的）位置的索引
	size_t i;

	x=key->x;
	y=key->y;
	d=key->data; 

	// 中间略过 RC4_CHUNK 相关代码若干

	// LOOP 将输入的 in（1 byte）加密为 out（1 byte）
#define LOOP(in,out) x=((x+1)&0xff);  tx=d[x]; y=(tx+y)&0xff; d[x]=ty=d[y]; d[y]=tx; (out) = d[(tx+ty)&0xff]^ (in);

#ifndef RC4_INDEX
#define RC4_LOOP(a,b,i)	LOOP(*((a)++),*((b)++))
#else
#define RC4_LOOP(a,b,i)	LOOP(a[i],b[i])
#endif

	i=len>>3;
	// 如果明文长度大于等于 8 bytes
	if (i)
	{
		// 一次处理 8 bytes
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
	// 处理（剩余的）小于 8 bytes 的明文
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
	// 指向生成的 key 的具体内容
	register RC4_INT *d;
	unsigned int i;

	d= &(key->data[0]);
	key->x = 0;
	key->y = 0;
	id1=id2=0;     

	// 下面 SK_LOOP 中 &0xff 操作其含义为 % 256
	// 对于 A % B 如果 B 为 2 的 x 次幂，那么 A % B == A & (B - 1)
	//
	// SK_LOOP 用于生成 RC4 key 的第 n 位的值
#define SK_LOOP(d,n) { tmp=d[(n)];  id2 = (data[id1] + tmp + id2) & 0xff;  if (++id1 == len) id1=0; d[(n)]=d[id2]; d[id2]=tmp; }

	// 略去部分代码

	for (i=0; i < 256; i++) d[i]=i;
	// 一次计算 4 个 int
	// 这里可以得知，生成 key 的算法的效率和参数 data 的长度无关
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
