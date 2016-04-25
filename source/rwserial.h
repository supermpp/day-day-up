#ifndef _rwserial_h
#define _rwserial_h
#include <netinet/in.h>


#define MGTRURATR_OUTITME 40
#define UPDATE_OUTTIME 4000

int initialCom( int iComPort, int baud, int parity, int bit_cnt, int stop_bit,int *fdp );
int  rtu_read_hldreg ( unsigned char *buf, unsigned char board_adr, int FunctionNum, int iFirstRegAddr, int iRegNum );
void serial_debug(const char *s,unsigned char *buff,int len);
int  serial_read(int m_hComDev, void* buf, unsigned int buflen,int out_time);
int serial_write(int fd,unsigned char  *buffer,int length) ;
int CheckRTUException( unsigned char *chResponse );
void closeCom(int fd);
int CheckCRC( unsigned char* chResponse, int iResLen );
unsigned short CRC16( unsigned char *puchMsg, short usDataLen );
unsigned char GetLength(unsigned short lenid,int high);
/*收取充电桩回复数据*/
int MGTR_UART_Read(int m_hComDev, void* buf, unsigned int buflen,int out_time);

#endif

