#include <stdio.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>


mqd_t tcpsend_msg;
mqd_t serialsend_msg;
pthread_mutex_t tcpsend_msg_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t serialsend_msg_lock = PTHREAD_MUTEX_INITIALIZER;
int Create_posixmsgq(mqd_t *fd,const char *msgqname,int maxmsg,int msgsize)
{
	mqd_t msgq_id;
	struct mq_attr msgq_attr;
	if(maxmsg != 0 && msgsize != 0){
		msgq_attr.mq_maxmsg = maxmsg;
		msgq_attr.mq_msgsize = msgsize;
		msgq_id = mq_open(msgqname, O_RDWR | O_CREAT |O_NONBLOCK, S_IRWXU, &msgq_attr);
	}
	else{
		msgq_id	= mq_open(file, O_RDWR | O_CREAT|O_NONBLOCK, S_IRWXU, NULL);
	}
	
	if(msgq_id ==  -1){
		perror("mq_open");
		return -1;
	}
	if(mq_getattr(msgq_id, &msgq_attr) == -1){
		perror("mq_getattr");
		return -1;
	}
	printf("Queue \"%s\":\n\t- stores at most %ld messages\n\t- large at most %ld bytes each\n\t- currently holds %ld messages\n", 
			msgqname, msgq_attr.mq_maxmsg, msgq_attr.mq_msgsize, msgq_attr.mq_curmsgs);	
	return 0;
}

int Open_posixmsgq(mqd_t *fd)
{
	
	return 0;
}
int Close_posixmsgq(mqd_t *fd)
{
	int ret = 0;
	ret = mq_close(*fd);
	if(ret == -1){
		perror("mq_close");
		return -1;
	}
	return 0;	
}
int Unlink_posixmsgq(const char *msgqname)
{
	int ret = 0;
	ret = mq_unlink(msgqname);
	if(ret == -1){
		perror("mq_unlink");
		return -1;
	}
	return 0;	
}

