#ifndef _tcpsocket_h
#define _tcpsocket_h
#include <stdio.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

#define TCPSEND_OVERTIME 1000000

void func1(void);
int my_close(int fd,int out_time);
int create_tcpconnect(int serverport,char *serverip); 
int RcvUdpPack(int sockfd,void *buff,int len,int out_time);

int tcp_send(int fd,void *buff,int len);
int tcp_recv(int fd,void * buff,int len,int out_time);

int create_tcpListenSock(void);
int sock_writestatus(int sockfd,char *data,int len_data);
int accept_sock_process(int listen_sock);
int NonblockTcpNetconnect(int *sock_fd,char *remote_ip,int remote_port,int local_port);
#endif

