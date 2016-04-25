#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "common.h"
#include "chargingpile.h"
#include "mypthread.h"
#include "rwconfig.h"
#include "ini-file.h"
#include "linklist.h"
#include "mytime.h"
#include "myprotocol.h"

extern struct servercfg remoteserver;
int restartnum = 0;
struct serial num[SERIALNUM];
unsigned int WG_DATAID = 0;
int debug_packet = 0;
int debug_strq = 0;

int main(int argc,char *argv[])
{
	int i = 0,myloglevel = LOG_ERR,runflag = 0;
	char buff[8] = {0},start_time[15] = {0},wgmac[17] = {0};
	pthread_attr_t attr;
	pthread_t pid,pid2,pid3[SERIALNUM];
	unsigned int stacksize;
	printf("usage:chargingpile [-f:前台运行] [-l loglevel:日志级别例-l 0不打印日志] [-dpacket:网络报文调试]\n");
	record_syslog(LOGSYSPATH, LOGSYSFILE1,LOGSYSFILE2,MAXSIZESYSLOG,"chargingpile restart\n");//记录系统重启日志

	//处理命令行参数
	for(i = 1;i < argc;i++){
		if(strcmp(argv[i],"-f") == 0){
			runflag = 1;
		}
		else if(strcmp(argv[i],"-l") == 0){
			i ++;
			myloglevel = atoi(argv[i]);
			printf("*****loglevel:%d*****\n",myloglevel);
			set_log_level(myloglevel);
		}
		else if(strcmp(argv[i],"-dpacket") == 0){
			printf("*****debug packet on******\n");
			debug_packet = 1;
		}
		else if(strcmp(argv[i],"-d0b") == 0){
			printf("*****debug 0b on******\n");
			debug_strq= 1;
		}
		else{
			printf("i do not know %s\n",argv[i]);
		}
	}
	if(runflag == 0){
		if((pid=fork())>0) 
			exit(0);
		else if(pid<0) 
			exit(1);
		setsid();
		if((pid=fork())>0) 	
			exit(0);
		else if(pid< 0) 
			exit(1);
	}


	EventIndex = read_profile_int("WGID", "lasteventindex",0,WGINFOINI);
	restartnum = read_profile_int("WGID","restartnum",0, WGINFOINI);
	WG_DATAID = read_profile_int("WGID","dataid",0, WGINFOINI);
	restartnum += 1;
	snprintf(buff,sizeof(buff),"%d",restartnum);
	write_profile_string("WGID","restartnum",buff, WGINFOINI);

	read_profile_string("WGID", "restarttime",start_time, 15,"00000000000000", WGINFOINI);
	printf("网关上一次重启时间start_time:%s dataid:%d\n",start_time,WG_DATAID);

	getmac("/etc/netup.sh",wgmac);
	mactoint(&wgid, wgmac);
	Rev32InByte(&wgid);
	printf("网关号:%u\n",wgid);
	memset(&remoteserver,'\0',sizeof(struct servercfg));
	readwg_parameter(WGCONFIGFILE);
	readrate_parameter( RATEFILE);
	read_blackcard(BLACKCARDFILE);

	//init linklist
	InitList(&head_tcpsendlist);
	InitList(&head_tcphavesendlist);
	InitList(&head_serialsendlist);
	InitList(&head_eventlist_old);
	//init 6 lists
	InitList(&head_serialsendlist1);
	InitList(&head_serialsendlist2);
	InitList(&head_serialsendlist3);
	InitList(&head_serialsendlist4);
	InitList(&head_serialsendlist5);
	InitList(&head_serialsendlist6);

	eventbuf = (char *)malloc(320*MAXEVENT);
	if(eventbuf != NULL)
		memset(eventbuf,'\0',320*MAXEVENT);
	else 
		printf("malloc error:%s\n",strerror(errno));

	if (pthread_attr_init(&attr) != 0){
		printf("pthread_attr_init fail\n");
	}
	pthread_attr_setstacksize(&attr,1*1024*1024);
	if (pthread_attr_getstacksize(&attr,&stacksize) == 0){
		printf("now thread  stacksize:%u KB\n",stacksize/1024);
	}
	//serial_debug(NULL, NULL,-1);

	pthread_create(&pid2,&attr,Socketprocess,NULL);
	for(i = 0; i < SERIALNUM;i++){
		printf("i:%d\n",i);
		num[i].portnum = ( i+1);
		num[i].serv_reconnect = SERVER_RECONNECT;
		pthread_create(&pid3[i],&attr,Serialprocess,(void *)&num[i]);
	}

	//pthread_join(pid3,NULL);
	pthread_join(pid2,NULL);
	printf("end\n");
	return 0;
}
