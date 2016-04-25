#include <stdio.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>


#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>

#include "common.h"
#include "tcpsocket.h"


//int my_close(int fd,void * buff,int len,int out_time)
//if return -2 -3 close tcp link
int my_close(int fd,int out_time)
{
	char buff[1] = {0};
	int wantrecv = 1;
	char *p;
	int i,m;
	int recv_len;
	fd_set rdset;
	struct timeval timeout;
	p=(char *)buff;
	recv_len=0;
	while(1)
	{
		if(out_time == -1){
			timeout.tv_sec = 0;
			timeout.tv_usec = 100;
		}
		else{
			timeout.tv_sec = out_time/1000000;
			timeout.tv_usec = out_time%1000000;
		}

		FD_ZERO(&rdset);
		FD_SET(fd,&rdset);
		i=select(fd+1,&rdset,NULL,NULL,&timeout);
		if(i==0)
		{
			print_log(1,"tcp_recv out time in select\n");
			close(fd);
			return -3;//time out
			
		}
		else if(i<0)
		{
			if(errno==EINTR)
			{
				print_log(1,"tcp_recv is interrupted in select\n");
				continue;
			}
			print_log(1,"tcp_recv error in select--%m\n");
			//return -1;
			close(fd);
			return -2;//20150122 tcp_recv error in select--Bad file descriptor
		}
		else
		{
			if(FD_ISSET(fd,&rdset))
			{
				m=recv(fd,p+recv_len,wantrecv-recv_len,0);
				if(m>0)
				{
					recv_len+=m;
					if(recv_len>=wantrecv)
					{
						return recv_len;
					}
					else
					{
						continue;
					}
				}
				else if (m==0)
				{
					print_log(1,"tcp link off%m\n");
					close(fd);
					return -2;//tcp server and client off 
				}
				else
				{
					if(errno==EINTR)
					{
						print_log(1,"tcp_recv is interrupted\n");
						continue;
					}
					if(errno==EAGAIN || errno==EWOULDBLOCK) 
					{
						print_log(1,"tcp_recv will again\n");
						continue;
					}
					print_log(1,"tcp_recv error %d--%m\n",errno);
					close(fd);
					return -1;
				}
			}
			close(fd);
			return -1;
		}
	}
}

void func1(void)
{
	printf("i am tcpsocket.c\n");
	return;
}




int tcp_send(int fd,void *buff,int len)
{
	int m,n,k,select_return;
	char *p;
	fd_set wrset;
	struct timeval timeout;

	k=0;
	n=len;
	p=(char *)buff;
	timeout.tv_sec=TCPSEND_OVERTIME/1000000;
	timeout.tv_usec=TCPSEND_OVERTIME%1000000;
	while(1)
	{
		FD_ZERO(&wrset);
		FD_SET(fd,&wrset);
		select_return=select(fd+1,NULL,&wrset,NULL,&timeout);
		if(select_return==0)
		{
			print_log(LOG_INFO,"tcp_send out time in select\n");
			return -1;
		}
		else if(select_return<0)
		{
			if(errno==EINTR)
			{
				print_log(LOG_INFO,"tcp_send is interrupted in select\n");
				continue;
			}
			print_log(LOG_ERR,"snmpdg400:net_send error in select--%m\n");
			return -1;
		}
		if(FD_ISSET(fd,&wrset)==0) return -1;

		m=send(fd,p+k,n,MSG_NOSIGNAL);
		if(m==n) {return len;}
		else
		{
			if(m>0)
			{
				print_log(LOG_INFO,"tcp_send will send %d bytes,but send %d\n",n,m);
				k+=m;
				n=n-m;
			}
			else
			{
				if(errno==EINTR)
				{
					print_log(LOG_INFO,"tcp_send is interrupted\n");
					continue;
				}
				if(errno==EAGAIN || errno==EWOULDBLOCK) 
				{
					print_log(LOG_INFO,"tcp_send will again\n");
					continue;
				}
				print_log(LOG_ERR,"tcp_send error %d--%m\n",errno);
				return -1;
			}
		}
	}
}



/*
 *该函数能处理因网络原因没能一次收完数据包的情况，
 返回值:超时

*/
int tcp_recv(int fd,void * buff,int len,int out_time)
{
	char *p;
	int i,m;
	int recv_len;
	fd_set rdset;
	struct timeval timeout;
	p=(char *)buff;
	recv_len=0;
	while(1)
	{
		if(out_time == -1){
			timeout.tv_sec = 0;
			timeout.tv_usec = 100;
		}
		else{
			timeout.tv_sec = out_time/1000000;
			timeout.tv_usec = out_time%1000000;
		}

		FD_ZERO(&rdset);
		FD_SET(fd,&rdset);
		i=select(fd+1,&rdset,NULL,NULL,&timeout);
		if(i==0)
		{
			print_log(1,"tcp_recv out time in select\n");
			return 0;//time out
		}
		else if(i<0)
		{
			if(errno==EINTR)
			{
				print_log(1,"tcp_recv is interrupted in select\n");
				continue;
			}
			print_log(1,"tcp_recv error in select--%m\n");
			//return -1;
			return -2;//20150122 tcp_recv error in select--Bad file descriptor
		}
		else
		{
			if(FD_ISSET(fd,&rdset))
			{
				m=recv(fd,p+recv_len,len-recv_len,0);
				if(m>0)
				{
					recv_len+=m;
					if(recv_len>=len)
					{
						return recv_len;
					}
					else
					{
						continue;
					}
				}
				else if (m==0)
				{
					print_log(1,"tcp link off%m\n");
					return -2;//tcp server and client off 
				}
				else
				{
					if(errno==EINTR)
					{
						print_log(1,"tcp_recv is interrupted\n");
						continue;
					}
					if(errno==EAGAIN || errno==EWOULDBLOCK) 
					{
						print_log(1,"tcp_recv will again\n");
						continue;
					}
					print_log(1,"tcp_recv error %d--%m\n",errno);
					return -1;
				}
			}
			return -1;
		}
	}
}

int RcvUdpPack(int sockfd,void *buff,int len,int out_time)
{
	fd_set rdset;
	int recv_len,retsl,n;
	struct timeval timeout;
	char *p = buff;

	recv_len = 0;
	while(1){
		FD_ZERO(&rdset);
		FD_SET(sockfd,&rdset);
		timeout.tv_sec = out_time;
		timeout.tv_usec = 0;

		retsl = select(sockfd+1,&rdset,NULL,NULL,&timeout);
		if(retsl == 0){
			printf("udp recive out time in select\n");
			return -1;
		}
		else if(retsl < 0){
			printf("udp recive interrupted in select\n");
			return -1;
		}
		else{
			if(FD_ISSET(sockfd,&rdset)){
				n = recvfrom(sockfd,p + recv_len,len - recv_len,MSG_NOSIGNAL,NULL,0);
				if(n > 0){
					recv_len += n;
					if(recv_len >= len){
						return recv_len;
					}
					else{
						continue;
					}
				}
				else if(n == 0){
					printf("udp recive error in recvfrom\n");
					print_log(LOG_ERR,"udp recive error in recvfrom\n");
					return -1;	
				}
				else{
					if(errno == EINTR){
						printf("udp recive is interrupted\n");
						print_log(LOG_ERR,"udp recive is interrupted\n");		
						continue;
					}
					else if(errno==EAGAIN || errno==EWOULDBLOCK){
						printf("udp recive will again\n");
						print_log(LOG_ERR,"udp recive will again\n");
						continue;
					}
				}

			}
			return -1;
		}
	}	
}

int accept_sock_process(int listen_sock)
{
	printf("accepting......\n");
	int accept_sock;
	long flag = 0;
	//struct sockaddr_in client_addr;
	/*
	   int sin_size;
	   sin_size = sizeof(struct sockaddr_in);
	   */
	for(;;){
		accept_sock = accept(listen_sock,NULL,NULL);

		if(accept_sock == -1){
			printf("accept() error:%s\n",strerror(errno));
			continue;
		}
		else{

			flag=fcntl(accept_sock,F_GETFL); //?????????,????open()???flags,???:??
			if(flag<0)
			{

				close(accept_sock);
				continue;
			}
			flag|=O_NONBLOCK;
			if(fcntl(accept_sock,F_SETFL,flag)<0)  //?????
			{

				close(accept_sock);
				continue;
			}

			break;
		}

	}

	return accept_sock;
}

/*
   判断tcp socket 是否可以写然后发报文
   */
int sock_writestatus(int sockfd,char *data,int len_data)
{
	int ret_select,ret_send;
	fd_set writefds;

	/*
	   备用
	   */

	/*
	   struct timeval tout;

	   tout.tv_sec = 1;
	   tout.tv_usec = 0;
	   */
	FD_ZERO(&writefds);
	FD_SET(sockfd,&writefds);



	ret_select = select(sockfd+1,NULL,&writefds,NULL,NULL) ;//时间参数传NULL 一直阻塞到可以写为止
	if(ret_select >0){
		if(FD_ISSET(sockfd,&writefds)){
			ret_send = send(sockfd,data,len_data,0);
			if(ret_send == -1){
				return -1;
				print_log(1,"send error : %s\n",strerror(errno));
			}
			else{
				print_log(LOG_DEBUG,"sock_writestatus-----want send %d bytes real send %d bytes\n",len_data,ret_send);
				return 1;
			}

		}
	}


	return 0;
}

/*
   返回tcp 监听套接字
   */
int create_tcpListenSock(void)
{
	//char localip[20] = {0};

	struct sockaddr_in serveraddr;//client_addr;
	int sockfd;//newfd;
	int ret;
	int keepalive = 1;
	//	int nRecvBuf = 256,nSndBuf = 256;
	int bReuseaddr=1;


	/*
	   ret = getlocalip(localip);
	   if(ret < 0){
	   printf("get localip error\n");
	   return -1;
	   }
	   */

	/* 服务器端开始建立socket描述符 */
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1){
		printf("socket error: %s\n",strerror(errno));
		return -1;
	}

	//set socket opt
	setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
	setsockopt(sockfd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&bReuseaddr,sizeof(int));


	/* 服务器端填充 serveraddr结构  */
	bzero(&serveraddr,sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(2404);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	/* 捆绑sockfd描述符  */
	ret = bind(sockfd,(struct sockaddr *)(&serveraddr),sizeof(struct sockaddr));
	if(ret < 0){
		printf("bind error,%s\n",strerror(errno));
		close(sockfd);
		return -1;
	}

	/* 监听sockfd描述符  */
	ret = listen(sockfd,5);
	if(ret == -1){
		printf("listen error:%s\n",strerror(errno));
		close(sockfd);
		return -1;
	}
	printf("listen\n");

	return sockfd;//返回监听socket
}


int create_tcpconnect(int serverport,char *serverip)
{
	struct sockaddr_in serveraddr;//clientaddr;
	int sockfd,res,ret,flag = 0;
	//create socket
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd < 0){
		printf("%s\n",strerror(errno));	
		return -1;
	}
	flag = fcntl(sockfd,F_GETFL,0);
	if(flag < 0){
		printf("fcnl get error:%s\n",strerror(errno));
		close(sockfd);
		return -1;
	}
	flag |= O_NONBLOCK;

	ret = fcntl(sockfd,F_SETOWN,flag);	
	if(ret < 0){
		printf("fcnl set error:%s\n",strerror(errno));
		close(sockfd);
		return -1;
	}

	//init bind client port
	/*
	   bzero(&clientaddr,sizeof(struct sockaddr_in));
	   clientaddr.sin_family = AF_INET;
	   clientaddr.sin_port = htons(clientport);
	   clientaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	   ret = bind(sockfd,(struct sockaddr*)&clientaddr,sizeof(clientaddr));
	   if(ret < 0){
	   printf("bind error,%s\n",strerror(errno));
	   close(sockfd);
	   return -1;
	   }
	   */

	//init server addr
	bzero(&serveraddr,sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(serverport);//鏉ヨ嚜浜庨厤缃枃浠?	
	serveraddr.sin_addr.s_addr = inet_addr(serverip);//鏉ヨ嚜浜庨厤缃枃浠?

	//connect
	res = connect(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr));
	if(res == -1){
		printf("connect fail:%s\n",strerror(errno));
		close(sockfd);
		return -1;
	}
	print_log(1,"*******tcpserver sockfd:%d\n",sockfd);

	return sockfd;
}

int NonblockTcpNetconnect(int *sock_fd,char *remote_ip,int remote_port,int local_port)
{
	int fd,opt,len;
	long flag;
	struct sockaddr_in pin2;
	struct sockaddr_in clientaddr;
	//struct linger ling ;
	fd_set wd;
	struct timeval timeout;
	int select_result,ret,on ;
	//int bind_num=0;
	//char Host_Addr_temp[20];
	//create stream socket
	fd=socket(AF_INET,SOCK_STREAM,0);
	if(fd<0) 
		return -1;
	on = 1;
	ret = setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&on, sizeof(on));
	if(ret < 0){
		printf("NonblockTcpNetconnect setsockopt set SO_REUSEADDR error:%s\n",strerror(errno));
		return -1;
	}
	opt=1;
	len=sizeof(opt);
	//set nonblock
	flag=fcntl(fd,F_GETFL);
	if(flag<0){
		close(fd);
		return -1;
	}
	flag |=O_NONBLOCK;
	if(fcntl(fd,F_SETFL,flag)<0) {
		close(fd);
		return -1;
	}
	//init bind client port
	if(local_port != 0){
		//tcp send rst
		#if 0
		ling.l_onoff = 1;
		ling.l_linger = 0;
		ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, (void*)&ling, sizeof(ling));
		if(ret < 0){
			printf("NonblockTcpNetconnect setsockopt set SO_LINGER error:%s\n",strerror(errno));
			return -1;
		}
		#endif		
		bzero(&clientaddr,sizeof(struct sockaddr_in));
		clientaddr.sin_family = AF_INET;
		clientaddr.sin_port = htons(local_port);
		clientaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		ret = bind(fd,(struct sockaddr*)&clientaddr,sizeof(clientaddr));
		if(ret < 0){
			printf("bind error,%s\n",strerror(errno));
			close(fd);
			return -1;
		}
	}


	//server ip port
	bzero(&pin2,sizeof(struct sockaddr_in));
	pin2.sin_family=AF_INET;
	pin2.sin_addr.s_addr=inet_addr(remote_ip);
	pin2.sin_port=htons(remote_port);

	if (connect(fd,(struct sockaddr *)&pin2,sizeof(struct sockaddr))!=0)
	{
		if(errno!=EINPROGRESS)
		{
			//printf("connect error :errno(%d)%s in NonblockTcpNetconnect\n",errno,strerror(errno));
			close(fd);
			return -1;
		}
		while(1)
		{
			FD_ZERO(&wd);
			FD_SET(fd,&wd);
			timeout.tv_sec=10;
			timeout.tv_usec=0;
			select_result=select(fd+1,NULL,&wd,NULL,&timeout);
			if(select_result==0)
			{
				printf("select out time in NonblockTcpNetconnect\n");
				close(fd); 
				return -1;
			}
			else if(select_result<0)
			{
				if(errno==EINTR) 
					continue;
				close(fd);
				return -1;
			}
			else
			{
				if(FD_ISSET(fd,&wd))
				{
					if (getsockopt(fd,SOL_SOCKET,SO_ERROR,&opt,(socklen_t *)&len)==0)
					{
						if (opt==0) 
							break;
					}
				}
				printf("connect error :opt is (%d)%s in NonblockTcpNetconnect\n",opt,strerror(errno));
				close(fd); 
				return -1;
			}
		}
	}
	*sock_fd=fd;
	return 0;
}


