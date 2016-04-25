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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "common.h"
#include "serial.h"

pthread_mutex_t g_serialfilelock=PTHREAD_MUTEX_INITIALIZER;

int lux_open(int iComPort)
{
	int fd;							/* File descriptor for the port */

    const char *pComPort;

    switch (iComPort)
	{
	case 0:
		pComPort = "/dev/ttyS0";
		break;
	case 1:
		pComPort = "/dev/ttyS1";
		break;
	case 2:
		pComPort = "/dev/ttyS2";
		break;
	case 3:
		pComPort = "/dev/ttyS3";
		break;
	case 4:
		pComPort = "/dev/ttyS4";
		break;
	case 5:
		pComPort = "/dev/ttyS5";
		break;
	case 6:
		pComPort = "/dev/ttyS6";
		break;
	case 7:
		pComPort = "/dev/ttyS7";
		break;
	case 8:
		pComPort = "/dev/ttyS8";
		break;
	case 9:
		pComPort = "/dev/ttyS9";
		break;
	case 10:
		pComPort = "/dev/ttyS10";
		break;
	case 11:
		pComPort = "/dev/ttyS11";
		break;
	case 12:
		pComPort = "/dev/ttyS12";
		break;
	case 13:
		pComPort = "/dev/ttyS13";
		break;
	case 14:
		pComPort = "/dev/ttyS14";
		break;
	case 15:
		pComPort = "/dev/ttyS15";
		break;
	case 16:
		pComPort = "/dev/ttyS16";
		break;
	case 17:
		pComPort = "/dev/ttyS17";
		break;
	case 18:
		pComPort = "/dev/ttyS18";
		break;			
	/*
	case 8:
		pComPort = "/dev/ttyUSB0";
		break;
	case 9:
		pComPort = "/dev/ttyUSB1";
		break;
		*/
	case 100:
		pComPort = "/dev/modbus-0";
		break;
	case 101:
		pComPort = "/dev/modbus-1";
		break;
	default:
		pComPort = "/dev/ttyS1";
		break;
	}

	fd = open(pComPort, O_RDWR | O_NOCTTY | O_NDELAY);

	if (fd <0) {
		return -1;
		printf("Cannot open port %d\n", iComPort);
	}

	return fd;
}

void lux_close(int fd)
{
	tcflush(fd,TCIOFLUSH);
	close(fd);
}

unsigned short CRC16( unsigned char *puchMsg, short usDataLen )
{
	int i=0,j=0;
	unsigned short CRC = 0xFFFF;
	unsigned short chTemp = 0;

	if(puchMsg == NULL)
	{
		return 0;
	}
	for(i=0; i<usDataLen;++i)
	{
		CRC ^= puchMsg[i]; //for byte
		for(j=0;j<8;++j)
		{
			if(CRC & 0x0001) //for bit
			{
				CRC >>=1;
				CRC ^= 0xA001; //1010 0000 0000 0001
			}
			else
			{
				CRC >>=1;
			}
		}
	}
	//swap the high&low
	chTemp = CRC & 0x00FF;
	chTemp<<=8;
	CRC >>=8;
	CRC |= chTemp;

	return CRC;

}

//create the RTU frame and then copy to the buffer
void construct_rtu_frm ( unsigned char *dst_buf, unsigned char *tmp_buf, char DateLenth )
{
	unsigned short  crc_tmp=0;
	crc_tmp = CRC16( tmp_buf,DateLenth);
	*(tmp_buf+DateLenth) = crc_tmp >> 8 ;
	*(tmp_buf+DateLenth+1) = crc_tmp & 0xff;
	DateLenth++;
	DateLenth++;

	while ( DateLenth--)
	{ 

		*dst_buf=*tmp_buf;
		dst_buf++;
		tmp_buf++;	

	}
    *dst_buf='\0';
}

//create the command for reading holding register and then save in buffer
int  rtu_read_hldreg ( unsigned char *buf, unsigned char board_adr, int FunctionNum, int iFirstRegAddr, int iRegNum )
{
	unsigned char tmp[256]={0},DateLenth=0;
	memset(tmp,0,sizeof(tmp));
	tmp[0] = board_adr;                //device_id
	tmp[1] = FunctionNum;              //function num
	tmp[2] = iFirstRegAddr/256;        //get high section
	tmp[3] = iFirstRegAddr & 0xff;
	tmp[4] = iRegNum/256;
	tmp[5] = iRegNum & 0xff;
	DateLenth = 6;
	construct_rtu_frm(buf,tmp,DateLenth);

	return 8;

}

int  rtu_read_hldregTcp ( unsigned char *buf, unsigned char board_adr, int FunctionNum, int iFirstRegAddr, int iRegNum )
{
	unsigned char tmp[256]={0};
	memset(tmp,0,sizeof(tmp));
	tmp[4]=0;
	tmp[5]=6;
	tmp[6] = board_adr;                //device_id
	tmp[7] = FunctionNum;              //function num
	tmp[8] = iFirstRegAddr/256;        //get high section
	tmp[9] = iFirstRegAddr & 0xff;
	tmp[10] = iRegNum/256;
	tmp[11] = iRegNum & 0xff;

	memcpy(buf,tmp,12);
	
	return 12;

}

//check the CRC
//success 1
//fail	  0
int CheckCRC( unsigned char* chResponse, int iResLen )
{
	unsigned short crc_result=0,crc_tmp=0;
	unsigned char tmp = 0;
	
	
	crc_tmp=*( chResponse+iResLen-2);                      //get the CRC low byte	
	crc_tmp=crc_tmp * 256;                                 //crc_tmp (low>>8) and then add CRC high byte
	tmp = *( chResponse+iResLen-1);
	crc_tmp += tmp;      

	crc_result = CRC16( chResponse,iResLen-2);			//calculate then CRC data(swap the high/low bytes)


	if ( crc_tmp !=crc_result )                            //compare
	{
		return 0;
	}
	return 1;
}

int CheckRTUException( unsigned char *chResponse )
{
	if(*(chResponse+1) & 0x80)
		return 0;
	return 1;
}

int initialCom( int iComPort, int baud, int parity, int bit_cnt, int stop_bit,int *fdp )
{
	int status;
	int fd;
	struct termios options;

	status = lux_open(iComPort);

	if ( status <0 )
		return 0;
	else
		fd = status;

	if(tcgetattr(fd,&options)!=0)
	{	
		lux_close(fd);
		return 0;
	}

	switch(baud)
	{
	case 50:
		baud = B50;
		break;
	case 75:
		baud = B75;
		break;
	case 110:
		baud = B110;
		break;
	case 134:
		baud = B134;
		break;
	case 150:
		baud = B150;
		break;
	case 300:
		baud = B300;
		break;
	case 600:
		baud = B600;
		break;
	case 1200:
		baud = B1200;
		break;
	case 1800:
		baud = B1800;
		break;
	case 2400:
		baud = B2400;
		break;
	case 4800:
		baud = B4800;
		break;
	case 9600:
		baud = B9600;
		break;
	case 19200:
		baud = B19200;
		break;
	case 38400:
		baud = B38400;
		break;
	case 57600:
		baud = B57600;
		break;
	case 115200:
		baud = B115200;
		break;
	default:
		baud = B9600;
		break;
	}
	
	cfmakeraw(&options);
	options.c_iflag &=~(IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK
						|ISTRIP|INLCR|IGNCR|ICRNL
						|IUCLC|IXON|IXANY|IXOFF);
	options.c_oflag &=~OPOST;
	options.c_lflag &=~(ECHO| ECHOE |ECHONL|ICANON|ISIG|IEXTEN);
	options.c_cflag &=~(CSIZE|PARENB|CSTOPB|CREAD|CRTSCTS);
	options.c_cflag |=(CS8|CLOCAL|CREAD);
		
	cfsetispeed(&options, baud);
	cfsetospeed(&options, baud);

	switch(bit_cnt)
	{
	case 5:
		options.c_cflag &= ~CSIZE; /* Mask the character size bits */
		options.c_cflag |= CS5;    /* Select 5 data bits */
		break;
	case 6:
		options.c_cflag &= ~CSIZE; /* Mask the character size bits */
		options.c_cflag |= CS6;    /* Select 6 data bits */
		break;
	case 7:
		options.c_cflag &= ~CSIZE; /* Mask the character size bits */
		options.c_cflag |= CS7;    /* Select 7 data bits */
		break;
	case 8:
		options.c_cflag &= ~CSIZE; /* Mask the character size bits */
		options.c_cflag |= CS8;    /* Select 8 data bits */
		break;
	default:
		options.c_cflag &= ~CSIZE; /* Mask the character size bits */
		options.c_cflag |= CS8;    /* Select 8 data bits */
		break;
	}


	switch(parity)
	{
	case 0:		//P_NONE
		options.c_cflag &= ~PARENB;
		//options.c_iflag &= ~INPCK;
		break;
	case 1:		//P_ODD
		options.c_cflag |= PARENB;
		options.c_cflag |= PARODD;
		//options.c_iflag |= INPCK;
		break;        
	case 2:		//P_EVEN
		options.c_cflag |= PARENB;
		options.c_cflag &= ~PARODD;
		//options.c_iflag |= INPCK;
		break;
	default:
		options.c_cflag &= ~PARENB;
		//options.c_iflag &= ~INPCK;
		break;
	}

	switch(stop_bit)
	{
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		options.c_cflag &= ~CSTOPB;
		break;
	}

	if (tcsetattr(fd,TCSANOW,&options) != 0)
	{
		printf("Set data_bit/parity/stop_bit/timeouts error. (fd=%d)\n",fd);
		lux_close(fd);
		return 0;
	}
	tcflush(fd,TCIOFLUSH); 
	
	*fdp = fd;
	return 1;
}

void closeCom(int fd)
{
	lux_close(fd);
}

unsigned char GetLength(unsigned short lenid,int high)
{
	//
	unsigned short s_sum=0x00;
	unsigned char LCHKSUM=0x00;
	unsigned char LENID[3]={0};
	unsigned char s_Ret=0x00;
	
	LENID[0]=(unsigned char)(lenid&0x000f); //<<D3D2D1D0
	LENID[1]=(unsigned char)((lenid&0x00f0)>>4); //<<D7D6D5D4
	LENID[2]=(unsigned char)((lenid&0x0f00)>>8);//<<D11D10D9D8	
	s_sum=LENID[0]+LENID[1]+LENID[2];
	LCHKSUM=(~(s_sum %16) +1)&0xffff;//<<和后模16的余数取反加1
	if(high)
	{
		//<<D15	D14	D13	D12	D11	D10	D9	D8
		s_Ret=(unsigned char)((LCHKSUM<<4)&0xf0)+(unsigned char)LENID[2];
	}
	else
	{
		s_Ret=(unsigned char)((LENID[1]<<4)&0xf0)+(unsigned char)LENID[0];
	}

	return s_Ret;

}

static unsigned short CreateFCS(char *Data,int iDataLen)
{
	int i;
	unsigned short int FCS = 0;
	for(i=1; i<iDataLen; ++i)
	{
		FCS += *(Data+i);
	}
	FCS = FCS%65536;
	FCS ^= 0xFFFF;
	FCS = FCS + 1;

	return FCS;

}

int checkFCS(char *Data,int iDataLen)
{
	unsigned short int FCS;
	char buff[4];
	
	if (iDataLen<5) return -1;
	FCS=CreateFCS(Data,iDataLen-5);
	buff[0] = bin2ascii((FCS&0xff00)>>8,'H');					   // <<CHKSUM
	buff[1] = bin2ascii((FCS&0xff00)>>8,'L');
	buff[2] = bin2ascii((FCS&0xff),'H');						   // <<CHKSUM
	buff[3] = bin2ascii((FCS&0xff),'L');

	if (strncmp(Data+iDataLen-5,buff,4)==0) return 0;
	return -1;
}

unsigned char CheckSum(unsigned char *p,int len)
{
	unsigned char sum=0;
	int i;
	for (i=0;i<len;i++)
	{
		sum+=p[i];
	}
	return sum;
}

void serial_debug(const char *s,unsigned char *buff,int len)
{
	FILE *fp;
	char tmp[1024];
	int i;
	static int bb=0;

	
	if (s==NULL && buff==NULL)
	{
		if (access("/g400serial.txt",F_OK)) bb=-1;
		else bb=1;
		return;
	}
	
	if (bb!=1) return;
	if (len>300) return;

	while(pthread_mutex_lock(&g_serialfilelock)!=0);

	fp=fopen("/g400serial.txt","a");
	if (fp==NULL)
	{
		while(pthread_mutex_unlock(&g_serialfilelock)!=0);
		return;
	}
	
	sprintf(tmp,"%s(%d):",s,len);

	for (i=0;i<len;i++) sprintf(tmp+strlen(tmp),"%c%c ",bin2ascii(buff[i],'H'),bin2ascii(buff[i],'L'));
	strcat(tmp,"\n");
	fwrite(tmp,1,strlen(tmp),fp);
	fclose(fp);

	while(pthread_mutex_unlock(&g_serialfilelock)!=0);
	return;
}

int  serial_read(int m_hComDev, void* buf, unsigned int buflen,int out_time)
{ 
	int i,m,n,num;
	int recv_len;
	fd_set rdset,errset;
	struct timeval timeout;
	unsigned char *p;

	recv_len=0;
	p=(unsigned char *)buf;

	while(1)
	{
		FD_ZERO(&rdset);
		FD_SET(m_hComDev,&rdset);
		FD_ZERO(&errset);
		FD_SET(m_hComDev,&errset);
		if (recv_len==0)
		{
			timeout.tv_sec=out_time/1000;
			timeout.tv_usec=(out_time%1000)*1000;
		}
		else
		{
			timeout.tv_sec=0;
			timeout.tv_usec=200000;
		}
		i=select(m_hComDev+1,&rdset,NULL,&errset,&timeout);
		if(i==0)
		{
			return 0;
		}
		else if(i<0)
		{
			if(errno==EINTR)
			{
				continue;
			}
			return 0;
		}
		else
		{
			if(FD_ISSET(m_hComDev,&rdset))
			{
				m=read(m_hComDev,p+recv_len,buflen-recv_len);
				if(m>0)
				{			
					recv_len+=m;
					if((unsigned int)recv_len>=buflen)
					{
						return recv_len;
					}
					else
					{
						if(recv_len>2 && p[recv_len-1]==ETX_CP)
						{
							for(n=recv_len-2,num=0;n>=0;n--)
							{
								if (p[n]==DLE_CP)
								{
									num++;
									continue;
								}
								break;
							}
							if (num%2==0) return recv_len;
						}
						continue;
					}
				}
				else if (m==0)
				{
					continue;
				}
				else
				{
					if(errno==EINTR /*|| errno==EAGAIN*/)
					{
						continue;
					}
					else
					{
						return 0;
					}
				}
			}
			return 0;
		}
	}
} 

/*收取充电桩回复数据*/
int MGTR_UART_Read(int m_hComDev, void* buf, unsigned int buflen,int out_time)
{
	if(m_hComDev<0) return 0;

	int i,m,n,num;
	int recv_len;
	fd_set rdset,errset;
	struct timeval timeout;
	unsigned char *p;

	recv_len=0;
	p=(unsigned char *)buf;

	while(1)
	{
		FD_ZERO(&rdset);
		FD_SET(m_hComDev,&rdset);
		FD_ZERO(&errset);
		FD_SET(m_hComDev,&errset);
		if (recv_len==0)
		{
			timeout.tv_sec=out_time/1000;
			timeout.tv_usec=(out_time%1000)*1000;
		}
		else
		{
			timeout.tv_sec=0;
			timeout.tv_usec=200000;
		}
		i=select(m_hComDev+1,&rdset,NULL,&errset,&timeout);
		if(i==0)
		{
			return 0;
		}
		else if(i<0)
		{
			if(errno==EINTR)
			{
				continue;
			}
			return 0;
		}
		else
		{
			if(FD_ISSET(m_hComDev,&rdset))
			{
				m=read(m_hComDev,p+recv_len,buflen-recv_len);
				if(m>0)
				{			
					recv_len+=m;
					if((unsigned int)recv_len>=buflen)
					{
						return recv_len;
					}
					else
					{
						if(recv_len>2 && p[recv_len-1]==ETX_CP)
						{
							for(n=recv_len-2,num=0;n>=0;n--)
							{
								if (p[n]==DLE_CP)
								{
									num++;
									continue;
								}
								break;
							}
							if (num%2==0) return recv_len;
						}
						continue;
					}
				}
				else if (m==0)
				{
					continue;
				}
				else
				{
					if(errno==EINTR /*|| errno==EAGAIN*/)
					{
						continue;
					}
					else
					{
						return 0;
					}
				}
			}
			return 0;
		}
	}
}
int serial_write(int fd,unsigned char *buffer,int length) 
{ 
	int m,n,k;
	char *p;
	
	k=0;
	n=length;
	p=(char *)buffer;
	tcflush(fd,TCIOFLUSH);
	while(1)
	{
		m=write(fd,p+k,n);
		if(m==n)
		{
			serial_debug("write",buffer,k+n);
			return (k+n);
		}
		else
		{
			if(m>0)
			{
				k+=m;
				n=n-m;
			}
			else
			{
				if(errno==EINTR || errno==EAGAIN) 
				{
					continue;
				}
				if(k==0)
				{
					serial_debug("write",buffer,-1);
					return -1;
				}
				else 
				{
					serial_debug("write",buffer,k);
					return k;
				}
			}
		}
	}
} 


