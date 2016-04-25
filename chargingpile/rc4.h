#ifndef _rc4_h
#define _rc4_h


#define RC4_INT unsigned int 
typedef struct rc4_key_st
{
    RC4_INT x,y; // key ¿¿¿
    RC4_INT data[256];//key ¿¿¿¿¿
} RC4_KEY;

void RC4(RC4_KEY *key, size_t len, const unsigned char *indata,unsigned char *outdata);
void RC4_set_key(RC4_KEY *key, int len, const unsigned char *data);
void rc4DE(char *buff,int len);

#endif

