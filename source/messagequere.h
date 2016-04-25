#ifndef _messagequere_h
#define _messagequere_h
#include <stdio.h>

#define TCPSENDMSG "/tcpsendmsg"
#define SERIALSENDMSG "/serialsendmsg"
#define MAXMSG 100

extern mqd_t tcpsend_msg;
extern mqd_t serialsend_msg;

extern pthread_mutex_t tcpsend_msg_lock;
extern pthread_mutex_t serialsend_msg_lock;
int Create_posixmsgq(mqd_t *fd,const char *msgqname,int maxmsg,int msgsize);
int Open_posixmsgq(mqd_t *fd);
int Close_posixmsgq(mqd_t *fd);
int Unlink_posixmsgq(const char *msgqname);
	
#endif
