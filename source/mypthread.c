#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>


#include "common.h"
#include "chargingpile.h"
#include "tcpsocket.h"
#include "rwconfig.h"
#include "ini-file.h"
#include "myprotocol.h"
#include "linklist.h"
#include "mytime.h"
#include "mypthread.h"
#include "serial.h"
#include "rwserial.h"


unsigned int EventIndex = 0;//用来记录事件插入index，每次都保存到文件，启动会从文件读出来
unsigned short count_updatefilenum = 0,num_updatefile = 0;
unsigned int wgid = 0;
char Flag_Recvheadfile = 0;
char Flag_Recvlastfile = 0;

char *eventbuf = NULL;
pthread_mutex_t eventbuf_lock = PTHREAD_MUTEX_INITIALIZER;

unsigned char FLAG_GATHER = 0x00;//事件补传命令标志
unsigned char Start_Time[4] = {0},End_Time[4] = {0};//事件补传起始结束日期

char CPversion[2] = {0};//充电桩版本号
extern struct serial num[SERIALNUM];
//struct status **chargingstat = NULL;
void *Mainprocess(void *arg)
{


	/*从事件文件里把历史事件读到内存里*/


	/*有事件补传命令*/
	while(1){
		sleep(1);
		if(FLAG_GATHER == 0x01)
		{
			FLAG_GATHER = 0x00;
		}
	}
	return NULL;
}

int socketpolling(int *fd)
{

	printf("-------------pthread socketpolling-------------\n");
	int sockfd = *fd;
	unsigned char recvbuf[1200] = {0}, Data[1100] = {0},tmp[512] = {0},servertime[7] = {0},timestr[14] = {0},serialnum[3] = {0},time_beg[15] = {0},buff[320] = {0};
	unsigned char *p = NULL;
	int i = 0,headlen = 0,lastlen = 0,recvlen = 0,ret = 0,totalsize = 0,Data_size = 0,file_size = 0,fp = 0,length = 0;
	int Outtime = atoi((char *)remoteserver.time_out); 
	unsigned char Filetype = 0x00,Lastflag = 0x00,LASTFRAME = 0x01;
	unsigned char CmdID = 0;
	unsigned int SERVER_DATAID = 0,T_SN = 0,size_updatefile = 0;
	unsigned short ht = 0,count_updatefilenum = 0;
	int flag1 = 0,flag2 =0,flag4 = 0,flag_wrestart = 0,flag_heart = 0;//文件代号0x01 0x02 0x04
	int cur_time = 0,next_time = 0;
	struct tm t1;		
	struct tm*tm1;
	struct node tmpnode;
	int time_begin = time(NULL);

#if 1
	/*从文件里将所有未上传的事件读出来写到head_tcphavesendlist里去*/
	for(i = 0; i < EventIndex;i++)
	{
		fp = open(EVENTFILE,O_CREAT|O_RDWR,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
		if(fp < 0)
		{
			printf("-fopen error:%s\n",strerror(errno));
		}
		lseek(fp,i*320,SEEK_SET);
		memset(buff,'\0',sizeof(buff));
		ret = read(fp,buff,320);
		close(fp);
		if(buff[8] == UPNOT)
		{
			memset(&tmpnode,'\0',sizeof(tmpnode));
			tmpnode.cmdid = CMDID_DEAL_UPDATE;
			if(buff[13] == 0x01)
				length = 40;
			else if(buff[13] == 0x02)
				length = 60;
			else if(buff[13] == 0x04)
				length = 291;
			else
				length = 36;
			tmpnode.sendtime = 0;
			memcpy(&(tmpnode.DATAID),buff+4,sizeof(tmpnode.DATAID));
			tmpnode.T_SN = wgid;
			print_log(LOG_DEBUG,"tmpnode.DATAID:%d tmpnode.T_SN:%d\n",tmpnode.T_SN,tmpnode.DATAID);
			memcpy(tmp,buff+14,length);
			tmpnode.DATA_LEN = Response_Frame(tmpnode.DATA, tmpnode.cmdid, tmpnode.DATAID, tmpnode.T_SN,tmp,length);

			AddFromEnd(head_tcphavesendlist, &tmpnode);
			printf("i=%d EventIndex=%d从/root/eventlist.txt里读取一条未上传事件到head_tcphavesendlist里事件时间:%02x%02x年 %02x月 %02x日\n"
					,i,EventIndex,buff[0],buff[1],buff[2],buff[3]);
		}
	}
#endif

	print_log(LOG_DEBUG,"while(1)----------------------------------------------------\n");
	while(1){
		memset(&tmpnode,'\0',sizeof(struct node));
		memset(recvbuf,'\0',sizeof(recvbuf));
		memset(tmp,'\0',sizeof(tmp));
		p = recvbuf;
		headlen = tcp_recv(sockfd,recvbuf,3,Outtime*100000);
		print_log(LOG_DEBUG,"tcp_recv ret:%d\n",headlen);
		if(headlen == -2)
		{
			/*tcp连接关闭*/
			return -2;
		}
		//Packetrecv_debug(recvbuf, 3);
		if((headlen == 3) && (recvbuf[0] == STX)){
			lastlen = (recvbuf[1]*256 + recvbuf[2]) + 2 ;//余下报文长度recvbuf[2] = Length    XOR+ETX = 2
			recvlen = tcp_recv(sockfd,p+headlen,lastlen,Outtime);
			totalsize = headlen + lastlen;
			if(recvbuf[4] != 0x88)//心跳包不打印出来
				Packetrecv_debug(recvbuf, totalsize);
			if(recvlen == lastlen){

				//获取tcp报文的CmdID DATAID T_SN Data
				if( (Data_size = AnalyzePacket(recvbuf,totalsize,&CmdID,&SERVER_DATAID,&T_SN,Data)) > 0){
					print_log(LOG_DEBUG,"CmdID:%02x  SERVER_DATAID:%d  T_SN:%d\n",CmdID,SERVER_DATAID,T_SN);
					if(T_SN != wgid ){
						printf("*****************************T_SN error*****************************\n");
					}
					print_log(LOG_DEBUG,"************************recv cmdid %02x************************\n",CmdID);
					if(CmdID == CMDID_FILEDOWNLOAD){
						//文件下传,在本线程应答服务器
						Lastflag = Data[0];
						Filetype = Data[1];
						p = Data;
						file_size = Data_size - 2;
						memcpy(tmp,p+2,file_size);//tmp 保存文件内容

						if(Filetype == CMDID_FILEDOWNLOAD){//集中器参数设置文件,应答帧在本线程生成且直接回复给服务器
							printf("集中器参数设置文件下传\n");
							if(Lastflag == LASTFRAME){//最后一帧
								if(flag1 == 0){
									writeconfigfile(WGCONFIGFILE,tmp,file_size,"w+");
								}
								else{
									writeconfigfile(WGCONFIGFILE,tmp,file_size,"a+");
									flag1 = 0;
								}
							}
							else{
								if(flag1 == 0){
									writeconfigfile(WGCONFIGFILE,tmp,file_size,"w+");
									flag1 = 1;
								}
								else{
									writeconfigfile(WGCONFIGFILE,tmp,file_size,"a+");
								}
							}
						} 
						else if(Filetype == 0x02){//费率文件
							printf("费率文件下传\n");
							if(Lastflag == LASTFRAME){//最后一帧
								if(flag2 == 0){
									writeconfigfile(RATEFILE,tmp,file_size,"w+");
								}
								else{
									writeconfigfile(RATEFILE,tmp,file_size,"a+");
									flag2 = 0;
								}
							}
							else{
								if(flag2 == 0){
									writeconfigfile(RATEFILE,tmp,file_size,"w+");
									flag2 = 1;
								}
								else{
									writeconfigfile(RATEFILE,tmp,file_size,"a+");
								}
							}
						}
						else if(Filetype == 0x03){//网关程序升级文件
							printf("程序升级文件\n");
						}
						else if(Filetype == 0x04){//交通卡黑名单
							printf("黑名单文件下传\n");
							if(Lastflag == LASTFRAME){//最后一帧
								if(flag4 == 0){
									writeconfigfile(BLACKCARDFILE,tmp,file_size,"w+");
								}
								else{
									writeconfigfile(BLACKCARDFILE,tmp,file_size,"a+");
									flag4 = 0;
								}
							}
							else{
								if(flag4 == 0){
									writeconfigfile(BLACKCARDFILE,tmp,file_size,"w+");
									flag4 = 1;
								}
								else{
									writeconfigfile(BLACKCARDFILE,tmp,file_size,"a+");
								}
							}
						}
						else if(Filetype == F_TYPE_05){//充电桩更新下载文件
							printf("充电桩更新下载文件\n");
							print_hexoflen(Data+2, 34);
							ret = Deal_Updatefile(Data+2,&num_updatefile,&size_updatefile, &count_updatefilenum);							
							if(ret == FILEHEAD){
								Flag_Recvlastfile = 0;
								writeupdatefile(PILEUPDATEFILE,Data+2,34,FILEHEAD);
							}
							else if(ret == FILEIN){
								writeupdatefile(PILEUPDATEFILE,Data+2,1034,FILEIN);
								Flag_Recvlastfile = 0;
							}
							else if(ret == FILELAST){
								writeupdatefile(PILEUPDATEFILE,Data+2,1034,FILELAST);
								Flag_Recvlastfile = 1;
							}
							else{//文件格式不正确
								tmp[0] = UPDATE_FORMWRONG;
							}
						}
						memset(Data,'\0',sizeof(Data));
						memset(tmp,'\0',sizeof(tmp));
						tmp[0] = UPDATE_SUCCESS;
						Data_size = Response_Frame(Data,CmdID, SERVER_DATAID, T_SN,tmp,1);
						ret = tcp_send(sockfd,Data,Data_size);
						print_log(LOG_DEBUG, "tcp reponse for 0x01\n");
						if(ret == Data_size){
							Packetsend_debug(Data, Data_size);
						}
						if(Lastflag == LASTFRAME)
						{
							if((Filetype == 0x01)||(Filetype == 0x02)||(Filetype == 0x04)){
								sync();
								sleep(5);
								system("reboot");
							}
						}
					}

					else if(CmdID == CMDID_PASSTHROUGH){
						/*数据透传,将透传指令写进head_serialsendlist,应答帧在串口线程收取,
						  根据应答情况在串口线程应答服务器*/
						//结构填值写到head_serialsendlist
						tmpnode.cmdid = CmdID;
						tmpnode.DATAID = SERVER_DATAID;
						tmpnode.T_SN = T_SN;
						tmpnode.Serialnum = Data[0];
						memcpy(&(tmpnode.Serialtimeout),Data+1,sizeof(tmpnode.Serialtimeout));
						tmpnode.DATA_LEN = (Data_size - 3);
						memcpy(tmpnode.DATA,Data+3,Data_size - 3);

						//new
						while(pthread_mutex_lock(&serialsendlist_lock)!=0);
						if(tmpnode.Serialnum == 1){
							AddFromEnd(head_serialsendlist1, &tmpnode);
						}
						if(tmpnode.Serialnum == 2){
							AddFromEnd(head_serialsendlist2, &tmpnode);
						}
						if(tmpnode.Serialnum == 3){
							AddFromEnd(head_serialsendlist3, &tmpnode);
						}
						if(tmpnode.Serialnum == 4){
							AddFromEnd(head_serialsendlist4, &tmpnode);
						}
						if(tmpnode.Serialnum == 5){
							AddFromEnd(head_serialsendlist5, &tmpnode);
						}
						if(tmpnode.Serialnum == 6){
							AddFromEnd(head_serialsendlist6, &tmpnode);							
						}
						while(pthread_mutex_unlock(&serialsendlist_lock)!=0);
					}
					else if(CmdID == 0x83){//应答设备状态上报
						//去查找head_tcphavesendlist,0x03报文发送成功，则删除丢弃报文；失败，则把该报文重新放到head_tcpsendlist里
						if(Data[0] == SUCCESS){
							if(DelDesignedlist(head_tcphavesendlist,&tmpnode,SERVER_DATAID) == 0)
								print_log(LOG_DEBUG,"0x03 send success! delete 0x03 %d from head_tcphavesendlist\n",tmpnode.DATAID);
						}
						else{
							if(DelDesignedlist(head_tcphavesendlist,&tmpnode,SERVER_DATAID) == 0){
								print_log(LOG_DEBUG,"0x03 send fail! delete 0x03 %d from head_tcphavesendlist and put in head_tcpsendlist\n",tmpnode.DATAID);
								while(pthread_mutex_lock(&tcpsendlist_lock)!=0);
								AddFromEnd(head_tcpsendlist, &tmpnode);
								while(pthread_mutex_unlock(&tcpsendlist_lock)!=0);
							}
						}
					}
					else if(CmdID == CMDID_CHARGCONTROL){//充电控制命令
						Deal_0x04(SERVER_DATAID,T_SN, Data,Data_size);//充电控制指令转化为对充电桩的指令发送到head_serialsendlist
						//应答帧：成功的应答码，只代表充电桩收到了命令，命令是否成功执行由交易事件来确定
					}
					else if(CmdID == 0x85){//应答交易事件上传
						//去查找head_tcphavesendlist,0x05报文发送成功，则删除丢弃报文；失败，则把该报文重新放到head_tcpsendlist里
						if( (Data[0] == SUCCESS)||(Data[0] == FAIL) ){//服务器会应答给我0x01，就会一直一直发送一条报文
							if(DelDesignedlist(head_tcphavesendlist,&tmpnode,SERVER_DATAID) == 0)
								print_log(LOG_DEBUG,"服务器成功应答交易事件,成功从head_tcphavesendlist里删除cmdid=0x05 DATAID=%d\n",SERVER_DATAID);
							else
								print_log(LOG_DEBUG,"服务器成功应答交易事件,head_tcphavesendlist里没有查找到cmdid=0x05 DATAID=%d\n",SERVER_DATAID);
							Set_EventFlag(EVENTFILE,SERVER_DATAID);
							print_log(LOG_DEBUG,"Set_EventFlag Set_EventFlag Set_EventFlag\n");
						}
						else{
							if(DelDesignedlist(head_tcphavesendlist,&tmpnode,SERVER_DATAID) == 0){
								print_log(LOG_DEBUG,"0x05 send fail! delete 0x05 %d from head_tcphavesendlist and put in head_tcpsendlist\n",SERVER_DATAID);
								while(pthread_mutex_lock(&tcpsendlist_lock)!=0);
								AddFromEnd(head_tcpsendlist, &tmpnode);
								while(pthread_mutex_unlock(&tcpsendlist_lock)!=0);
							}
						}
					}
					else if(CmdID == CMDID_SERVER_GATHER){//事件补传命令
						//放到主进程里去读历史交易事件
						FLAG_GATHER = 0x01;
						memcpy(Start_Time,Data,4);
						memcpy(End_Time,Data+4,4);
						//Event_transfer(startdate,enddate);
					}
					else if(CmdID == CMDID_PROCESS_QUERY){//充电过程查询
						tmpnode.cmdid = CmdID;
						tmpnode.DATAID = SERVER_DATAID;
						tmpnode.T_SN = T_SN;
						memcpy(tmpnode.SN,Data,Data_size);
						Getserianuml_SN(tmpnode.SN,(char *)serialnum);//查找充电桩SN属于哪一个串口
						while(pthread_mutex_lock(&serialsendlist_lock)!=0);
						if(atoi(serialnum) == 1){
							AddFromEnd(head_serialsendlist1,&tmpnode);
						}
						else if(atoi(serialnum) == 2){
							AddFromEnd(head_serialsendlist2,&tmpnode);
						}
						else if(atoi(serialnum) == 3){
							AddFromEnd(head_serialsendlist3,&tmpnode);
						}
						else if(atoi(serialnum) == 4){
							AddFromEnd(head_serialsendlist4,&tmpnode);
						}
						else if(atoi(serialnum) == 5){
							AddFromEnd(head_serialsendlist5,&tmpnode);
						}
						else if(atoi(serialnum) == 6){
							AddFromEnd(head_serialsendlist6,&tmpnode);
						}
						while(pthread_mutex_unlock(&serialsendlist_lock)!=0);
					}
					else if(CmdID == 0x88){//心跳包
						flag_heart = 0;
						memcpy(servertime,Data,sizeof(servertime));
						//判断是否要更新采集器时间servertime
						BCDtoStr(servertime,sizeof(servertime),timestr);
						Strtotm(timestr,&t1);
						//print_log(LOG_DEBUG,"timestr:%.14s  curtime:%d servertime:%d\n",timestr,time(NULL), mktime(&t1));
						if(abs(time(NULL) - mktime(&t1)) > TIMEDELAY){
							setTime((char *)timestr);					
							print_log(LOG_DEBUG,"设置集中器系统时间\n");
						}
					}

				}
			}

		}

		/******************************************/
		//head_tcpsendlist 里有数据则取一条发送
		/******************************************/
		while(pthread_mutex_lock(&tcpsendlist_lock)!=0);
		if(head_tcpsendlist->next != NULL){
			memset(&tmpnode,'\0',sizeof(tmpnode));
			DelFromHead(head_tcpsendlist, &tmpnode);
			tmpnode.sendtime = time(NULL);
			if(tmpnode.cmdid == CMDID_DEAL_UPDATE){//交易控制事件要备份到head_tcphavesendlist，用于重发
				AddFromEnd(head_tcphavesendlist, &tmpnode);
				print_log(LOG_DEBUG,"从head_tcpsendlist取放到head_tcphavesendlist  cmdid:%02x DATA_LEN:%d DATAID:%d T_SN:%d\n",tmpnode.cmdid,tmpnode.DATA_LEN,tmpnode.DATAID,tmpnode.T_SN);
			}
			ret = tcp_send(sockfd,tmpnode.DATA,tmpnode.DATA_LEN);

			print_log(LOG_INFO,"发送报文时间:%d\n************************send cmdid %02x************************\n",time(NULL),tmpnode.DATA[4]);
			if(tmpnode.cmdid == CMDID_DEAL_UPDATE)
				write_log_time(LOG_INFO,"send cmdid %02x eventtype:%02x %12s\n",tmpnode.DATA[4],tmpnode.eventtype,tmpnode.SN);			
			else 
				write_log_time(LOG_INFO,"send cmdid %02x\n",tmpnode.DATA[4]);

			if(ret == tmpnode.DATA_LEN){
				Packetsend_debug(tmpnode.DATA, tmpnode.DATA_LEN);
			}
			else{
				AddFromEnd(head_tcpsendlist,&tmpnode);
			}
		}
		while(pthread_mutex_unlock(&tcpsendlist_lock)!=0);

		/********************************************/
		//超时时间到了从head_tcphavesendlist里面取出一个
		/********************************************/
		if(head_tcphavesendlist->next != NULL){//第一个是最先插入的，是最早发送的一条报文
			if( (time(NULL) - head_tcphavesendlist->next->sendtime) > WGSEVER_OUTTIME){
				memset(&tmpnode,'\0',sizeof(tmpnode));
				DelFromHead(head_tcphavesendlist,&tmpnode);
				if(tmpnode.cmdid == CMDID_DEAL_UPDATE){//交易控制事件要备份到head_tcphavesendlist，用于重发
					tmpnode.sendtime = time(NULL);
					AddFromEnd(head_tcphavesendlist, &tmpnode);
					print_log(LOG_DEBUG,"从head_tcphavesendlist取放到head_tcphavesendlist  cmdid:%02x DATA_LEN:%d DATAID:%d T_SN:%d\n",tmpnode.cmdid,tmpnode.DATA_LEN,tmpnode.DATAID,tmpnode.T_SN);
				}
				ret = tcp_send(sockfd,tmpnode.DATA,tmpnode.DATA_LEN);
				if(ret == tmpnode.DATA_LEN){
					print_log(LOG_DEBUG,"************************重新发送成功cmdid %02x************************\n",tmpnode.DATA[4]);
					Packetsend_debug(tmpnode.DATA, tmpnode.DATA_LEN);
				}
				else{
					print_log(LOG_DEBUG,"************************重新发送失败cmdid %02x************************\n",tmpnode.DATA[4]);
					while(pthread_mutex_lock(&tcpsendlist_lock)!=0);
					AddFromEnd(head_tcpsendlist,&tmpnode);
					while(pthread_mutex_unlock(&tcpsendlist_lock)!=0);

				}

			}
		}

		/*心跳包*/
		cur_time = time(NULL);
		if(cur_time > next_time){
			if(flag_heart == 0){
				ht = HeartTime;
				Rev16InByte(&ht);
				Data_size = Response_Frame(Data,CMDID_HEARTTAG,WG_DATAID,wgid,&ht,sizeof(ht));
				ret = tcp_send(sockfd, Data, Data_size);
				//Packetsend_debug(Data,ret);
				WG_DATAID++;
				print_log(LOG_DEBUG,"************************send cmdid 0x08************************WG_DATAID:%d\n",WG_DATAID);
				next_time = (cur_time + HeartTime);
				flag_heart = 1;
			}
			else
			{
				/*心跳包超时*/
				return -1;
			}
		}
		//程序运行超过60分钟开始记下程序重启时间
		if( ((cur_time - time_begin) > TIME_WRESTART) &&(flag_wrestart == 0) ){
			tm1 = localtime(&time_begin);
			snprintf(time_beg,15,"%04d%02d%02d%02d%02d%02d",tm1->tm_year+1900,tm1->tm_mon+1,tm1->tm_mday,tm1->tm_hour,tm1->tm_min,tm1->tm_sec);			
			write_profile_string("WGID","restarttime",time_beg,WGINFOINI);
			flag_wrestart = 1;
		}
	}
}


void *Socketprocess(void *arg)
{
	char remoteip[17] = {0},remoteport[6] = {0};
	int timeuploadstate,timetcpout;
	int socketfd,ret,i = 0;
#if 0
	struct servercfg *initserver = (struct servercfg *)arg;
	memcpy(remoteip,initserver->ip,sizeof(initserver->ip));
	memcpy(remoteport,initserver->port,sizeof(initserver->port));
	timeuploadstate = atoi(initserver->time_upload);
	timetcpout = atoi(initserver->time_out);
#endif
	memcpy(remoteip,remoteserver.ip,sizeof(remoteserver.ip));
	memcpy(remoteport,remoteserver.port,sizeof(remoteserver.port));
	timeuploadstate = atoi((char *)remoteserver.time_upload);
	timetcpout = atoi((char *)remoteserver.time_out);
	print_log(LOG_DEBUG, "remoteip:%s remoteport:%s timeuploadstate:%d timetcpout:%d\n",remoteip,remoteport,timeuploadstate,timetcpout);
	while(1){
		ret = NonblockTcpNetconnect(&socketfd,remoteip, atoi(remoteport),0);
		for(;;){
			if(ret < 0){
				usleep(2000*1000);
				ret = NonblockTcpNetconnect(&socketfd,remoteip, atoi(remoteport),0);
			}
			else
				break;
		}
		printf("连接服务器成功\n");
		for(i = 0;i < SERIALNUM;i++)
			num[i].serv_reconnect = SERVER_RECONNECT;
		socketpolling(&socketfd);
		close(socketfd);
	}

	return NULL;
}
void *Serialprocess(void *arg)
{
	struct serial *port = (struct serial*)malloc(sizeof(struct serial));
	memcpy(port, (struct serial*)arg,sizeof(struct serial));
	struct node *temp = NULL;
	print_log(LOG_DEBUG,"port->portnum:%d\n",port->portnum);

	if(port->portnum == 1)
		temp = head_serialsendlist1;
	else if(port->portnum == 2)
		temp = head_serialsendlist2;
	else if(port->portnum == 3)
		temp= head_serialsendlist3;
	else if(port->portnum == 4)
		temp= head_serialsendlist4;
	else if(port->portnum == 5)
		temp= head_serialsendlist5;
	else if(port->portnum == 6)
		temp= head_serialsendlist6;

	int fd = 0,update_index = -1,fileindex = 0,fileoffset = 0,readversion_num =0,start_flag = 0;
	int i = 0,max = 0,len = 0,writelen = 0,readlen = 0,count = 0,len_frame_stat = 0,reslen = 0,frame_eventdata_len = 0,flag = 0,frame_clean_len = 0,ret = 0,index = 0;
	int maxdevice = 4,present_subset_count = 0,cleannum = 0;//maxdevice 要根据struct node 里data的大小而定
	//int sub_cnt = -1;
	unsigned char frame[512] = {0},frame_t[64] = {0},rx_frame[300] = {0},res_frame[300] = {0},frame_stat[512] = {0},tmp_stat[512] = {0},buff_fail[5] = {0};
	unsigned char carddata[300] = {0},frame_event_data[300] = {0},frame_event[300] = {0},money[4] = {0},frame_clean[64] = {0},dataid_str[11] = {0};
	unsigned char data_query[64] = {0},bcdtime[7] = {0},strtime[14] = {0},frame_clock[32] = {0},buff_update[1034] = {0},frame_update[1300] = {0},update_addr[2] = {0};
	char index_str[6] = {0};//40000
	int data_query_l = 0,clock_len = 0;
	unsigned char EventType = 0x00,psn_own = 0;
	int nexttime = 0,curtime = 0,flag_clocksync = 0,flag_cleanbuf = 0,flag_getdevmsg = 0;
	struct status tmp,strquery,dststat;
	struct event_str eventp;
	memset(&eventp,'\0',sizeof(struct event_str));
	memset(&tmp,'\0',sizeof(struct status));
	memset(&strquery,'\0',sizeof(struct status));
	memset(&dststat,'\0',sizeof(struct status));


	struct node tmpnode,delnode,delnode_s,sendnode,tmpnode_res;
	memset(&tmpnode,'\0',sizeof(struct node));
	memset(&delnode,'\0',sizeof(struct node));
	memset(&delnode_s,'\0',sizeof(struct node));
	memset(&sendnode,'\0',sizeof(struct node));
	memset(&tmpnode_res,'\0',sizeof(struct status));

	//创建属于此串口的所有充电桩的结构体
	struct status **chargingstat = NULL;
	struct status **p_stat = NULL;

	chargingstat = (struct status **)malloc(sizeof(struct status *)*MAX_CHARGING_NUM);
	for (i=0;i<MAX_CHARGING_NUM;i++){
		chargingstat[i]=(struct status *)malloc(sizeof(struct status));
		memset(chargingstat[i],0,sizeof(struct status));
	}

	//new
	index = (port->portnum - 1);
	max = EverySerialNum[index];//portnum 
	printf("-------------%d 号串口线程-------------max:%d\n",port->portnum,max);

	for(i = 0;i < max;i++){
		//new
		memcpy(chargingstat[i]->SN,(char *)AllSerialSn[index]+(i*12),12);
		print_log(LOG_DEBUG,"index:%d-SN:%12s\n",index,chargingstat[i]->SN);
		Read_Status(StatusFile,chargingstat[i], i,port->portnum);//把文件里记录的充电桩状态读上来
		//new 通过将connet字段强置成UNCONNECT确保每次上电都上报一次状态
		chargingstat[i]->UpStatus = UPDATEFREE;
		chargingstat[i]->connect = UNCONNECT;
		chargingstat[i]->Hw_Status = 0x00;
	}
	present_subset_count = max;
	p_stat = chargingstat;
	while(1){
		if(initialCom(port->portnum,BAUND,PARITY,DATABITS,STOPBITS,&fd) != 1){
			sleep(1);
			continue;
		}
		else
			break;
	}
	printf("%d 号串口线程串口初始化完成\n",port->portnum);
	nexttime = (time(NULL) +  SAVESTATTIME);
	while(1){
		curtime = time(NULL);
		flag_clocksync = 0;
		if(num[index].serv_reconnect == SERVER_RECONNECT){
			print_log(LOG_DEBUG,"服务器重新连接-----串口号:%d\n",port->portnum);
			for(i = 0;i < max;i++){
				chargingstat[i]->UpStatus = UPDATEFREE;
				chargingstat[i]->connect = UNCONNECT;
				chargingstat[i]->Hw_Status = 0x00;
			}
			num[index].serv_reconnect = SERVER_CONNET;
		}
		//轮询充电桩
		for(i = 0;i < present_subset_count ;i++){		
			usleep(50*1000);
			memset(&tmp,'\0',sizeof(struct status));
			//轮询之前先保存一个前一次的状态,用于判断充电桩是否有变化
			memcpy(&tmp,chargingstat[i],sizeof(struct status));
			memset(frame,'\0',sizeof(frame));
			psn_own = PSN++;
			len = Create_Frame_StatusRequest(psn_own,chargingstat[i]->SN, frame);
			writelen = serial_write(fd,frame,len);
#if 0
			print_time();
			StatusRequest_senddebug(frame, len,CMD_QUERY_STATE);
#endif 
			if(writelen == len)
			{
				memset(frame,'\0',sizeof(frame));
				readlen = 0;
				readlen = MGTR_UART_Read(fd,frame,512,MGTRURATR_OUTITME);
				print_log(LOG_DEBUG,"MGTR_UART_Read out time %d\n",readlen);
#if 1
				print_time();
				StatusRequest_recvdebug(frame,readlen,CMD_QUERY_STATE);
#endif
				//设备状态查询成功,解析返回值赋值给对应充电桩的结构体chargingstat [i]
				ret = Unpack_Frame_StatusRequest(frame, readlen,chargingstat[i],rx_frame,psn_own);
				if(ret > 0)
				{
					chargingstat[i]->connectfailn = 0x00;
					chargingstat[i]->connect = CONNECT;
					memcpy(bcdtime,rx_frame+9,sizeof(bcdtime));
					BCDtoStr(bcdtime,sizeof(bcdtime),strtime);
					if(flag_clocksync == 0){
						if(abs(time14ToInt(strtime) - time(NULL)) > ClockDeviation)
						{
							psn_own = PSN++;
							clock_len = Frame_Clock_Sync(psn_own,frame_clock);

							serial_write(fd,frame_clock, clock_len);
							flag_clocksync = 1;
							Serialsend_debug(frame_clock,clock_len,CMD_CLOCK_SYNC);
						}
					}
				}
				else if(ret == 0)//设备忙
				{
					flag_cleanbuf = 1;
					chargingstat[i]->CEvent = 0x00;//防止状态查询失败导致重复判定有事件(因为状态查询失败的时候CEvent状态不清)
				}
				else if(ret == -2)
				{
					flag_cleanbuf = 1;
					chargingstat[i]->CEvent = 0x00;//防止状态查询失败导致重复判定有事件(因为状态查询失败的时候CEvent状态不清)
					chargingstat[i]->connectfailn += 0x01;
					print_log(LOG_DEBUG,"%.12s connectfailn = %02x\n",chargingstat[i]->SN,chargingstat[i]->connectfailn);
					if(chargingstat[i]->connectfailn > 0x0a)//当连接失败次数超过10次才将通讯状态置为中断
					{	
						print_log(LOG_DEBUG,"%.12s connectfailn = %02x\n",chargingstat[i]->SN,chargingstat[i]->connectfailn);
						chargingstat[i]->connect = UNCONNECT;
						chargingstat[i]->connectfailn = 0x00;
					}
				}
				else if(ret == -5)
				{
					flag_cleanbuf = 1;
					chargingstat[i]->CEvent = 0x00;//防止状态查询失败导致重复判定有事件(因为状态查询失败的时候CEvent状态不清)
				}


			}
			else
			{
				;
			}

			//下发获取设备信息指令(1上电查询2更新完了之后查询)
			if(chargingstat[i]->FLAG_CMD11 == SEND)
			{

				psn_own = PSN++;
				len = Frame_GetDevmsg(psn_own,chargingstat[i]->SN, frame);
				writelen = serial_write(fd,frame,len);
				Serialsend_debug(frame,len,CMD_GET_DEVMSG);
				if(writelen == len)
				{
					memset(frame,'\0',sizeof(frame));
					readlen = MGTR_UART_Read(fd,frame,512,MGTRURATR_OUTITME);
					Serialrecv_debug(frame,readlen,CMD_GET_DEVMSG);
					len = MGTR_UART_rx_format(frame,readlen,rx_frame,psn_own);
					if((len > 0)&&(rx_frame[5] == STATUS_SUCCESS))
					{
						chargingstat[i]->FLAG_CMD11 = NOSEND;
						memcpy(chargingstat[i]->CPFWW,rx_frame+9,sizeof(chargingstat[i]->CPFWW));
						print_log(LOG_DEBUG,"FWW:%02x%02x\n",chargingstat[i]->CPFWW[0],chargingstat[i]->CPFWW[1]);

					}				
				}
			}

			//更新充电桩固件			
			if((!access(PILEUPDATEFILE,F_OK)))
			{
				print_log(LOG_DEBUG,"F_OK\n");
				if( (chargingstat[i]->CStatus == 0x00)&&(memcmp(chargingstat[i]->CPFWW,CPversion,sizeof(CPversion)) != 0) )
				{
					if(update_index == -1)//下发0000文件
					{
						update_index = i;
						chargingstat[i]->UpStatus = UPDATING;//如果桩处于更新中的状态就接收其他的指令
						memset(buff_update,'\0',sizeof(buff_update));

						ret = Read_Nbytes(buff_update,fileoffset,34,PILEUPDATEFILE);
						print_log(LOG_DEBUG,"ret:%d\n",ret);
						print_hexoflen(buff_update,ret);
						memcpy(CPversion,buff_update+6,2);
						psn_own = PSN++;
						len = Frame_UpdateCPfile(psn_own, buff_update,ret,chargingstat[i]->SN,frame_update);
						writelen = serial_write(fd,frame_update,len);
						Serialsend_debug(frame_update,len,CMD_UPDATECP);
						print_log(LOG_DEBUG,"开始更新[%.12s] FWW:%02x%02x CPversion:%02x%02x\n",chargingstat[i]->SN,chargingstat[i]->CPFWW[0],chargingstat[i]->CPFWW[1],CPversion[0],CPversion[1]);

						if(writelen == len)
						{
							memset(frame_update,'\0',sizeof(frame_update));
							memset(rx_frame,'\0',sizeof(rx_frame));
							readlen = MGTR_UART_Read(fd,frame_update,512,UPDATE_OUTTIME);
							Serialrecv_debug(frame_update,readlen,CMD_UPDATECP);
							len = MGTR_UART_rx_format(frame_update,readlen,rx_frame,psn_own);
							Serialrecv_debug(rx_frame,len,CMD_UPDATECP);
							if((len > 0)&&(rx_frame[5] == STATUS_SUCCESS))
							{
								print_log(LOG_DEBUG,"0000文件下发成功[%.12s]\n",chargingstat[i]->SN);
								fileindex++;
								fileoffset += 34;
							}	
							else
							{
								print_log(LOG_DEBUG,"0000文件下发失败[%.12s]\n",chargingstat[i]->SN);
								chargingstat[i]->UpStatus = UPDATEFREE;
								fileindex = 0;
								fileoffset = 0;
								update_index = -1;
							}
						}

						print_log(LOG_DEBUG,"fileindex:%d\n",fileindex);
					}
					else if(update_index == i)
					{
						//下发更新文件0001-ffff文件
						memset(buff_update,'\0',sizeof(buff_update));
						ret = Read_Nbytes(buff_update ,fileoffset,1034,PILEUPDATEFILE);
						memcpy(update_addr,buff_update,2);
						//print_hexoflen(buff_update,ret);
						psn_own = PSN++;
						len = Frame_UpdateCPfile(psn_own, buff_update,ret,chargingstat[i]->SN,frame_update);

						writelen = serial_write(fd,frame_update,len);
						Serialsend_debug(frame_update,len,CMD_UPDATECP);
						if(writelen == len)
						{
							memset(frame_update,'\0',sizeof(frame_update));
							memset(rx_frame,'\0',sizeof(rx_frame));
							readlen = MGTR_UART_Read(fd,frame_update,512,UPDATE_OUTTIME);
							print_log(LOG_DEBUG,"readlen:%d\n",readlen);
							Serialrecv_debug(frame_update,readlen,CMD_UPDATECP);
							len = MGTR_UART_rx_format(frame_update,readlen,rx_frame,psn_own);
							Serialrecv_debug(rx_frame,len,CMD_UPDATECP);

							if((len > 0)&&(rx_frame[5] == STATUS_SUCCESS))
							{
								print_log(LOG_DEBUG,"非0000文件下发成功[%.12s]\n",chargingstat[i]->SN);
								fileindex++;
								fileoffset += 1034;
							}
							else
							{
								print_log(LOG_DEBUG,"非0000文件下发失败[%.12s]\n",chargingstat[i]->SN);
								chargingstat[i]->UpStatus = UPDATEFREE;
								fileindex = 0;
								fileoffset = 0;
								update_index = -1;
							}
						}

						print_log(LOG_DEBUG,"fileindex:%d  \n",fileindex);
						//下发完ffff文件
						//if(count_updatefilenum == (fileindex - 1))
						if((update_addr[0] == 0xff)&&(update_addr[1] == 0xff))
						{
							chargingstat[i]->UpStatus = UPDATEFREE;
							memcpy(chargingstat[i]->CPFWW,CPversion,2);
							update_index = -1;
							flag_getdevmsg = 0;//去重新获取充电桩的版本号 
							fileoffset = 0;
							fileindex = 0;
							print_log(LOG_DEBUG,"[%.12s]更新成功\n",chargingstat[i]->SN);
							/*****************debug:NOSEND  release:SEND*****************/
							chargingstat[i]->FLAG_CMD11 = SEND;
						}
					}
				}
			}

			//每一圈循环都是从通讯失败的里面取一个,跳过余下的不通的
#if 0
			if(chargingstat[i]->connect == CONNECT)
			{
				;
			}
			else if(chargingstat[i]->connect == UNCONNECT)
			{
				sub_cnt = i;
				h_read = 1;
				for(j = sub_cnt + 1;j < present_subset_count;j++)
				{
					if(chargingstat[i]->connect == UNCONNECT)
						break;
				}
				if(j >= present_subset_count)
					sub_cnt = -1;
			}
#endif

			/*tmp与现在的状态比较,如果状态(当前状态,通信状态)发生改变上报
			  Ctatus充电桩当前状态,
			  CEvent充电桩内部事件状态位设置 1-有，0-无
			  B0 - 卡片刷卡信息B1 - 卡片扣款交易B2 - 充电控制事件*/
			print_log(LOG_DEBUG,"桩%.12s  通信状态%02x %02x当前桩状态%02x %02x硬件状态%02x %02x\n",chargingstat[i]->SN,chargingstat[i]->connect,tmp.connect,chargingstat[i]->CStatus,tmp.CStatus,chargingstat[i]->Hw_Status,tmp.Hw_Status);
			if( (chargingstat[i]->CStatus != tmp.CStatus) || (chargingstat[i]->connect != tmp.connect) ||(chargingstat[i]->Hw_Status != tmp.Hw_Status))
			{
				Record_Status(StatusFile,chargingstat[i],i,port->portnum);//当桩状态有变化写文件
				if(chargingstat[i]->CStatus != tmp.CStatus)
					print_log(LOG_DEBUG,"--------------------充电桩状态改变--------------------\n");
				else if(chargingstat[i]->connect != tmp.connect)
					print_log(LOG_DEBUG,"--------------------充电桩通信状态改变--------------------\n");
				else if(chargingstat[i]->Hw_Status != tmp.Hw_Status)
					print_log(LOG_DEBUG,"--------------------充电桩硬件状态改变--------------------\n");
				count++;
				len_frame_stat = Create_Data_Status(tmp_stat,count, chargingstat[i]);//组织待上传的0x03报文
				print_hexoflen(tmp_stat,len_frame_stat);
				if(count == maxdevice){//每maxdevice个设备的状态组成一个报文
					len = Response_Frame(frame_stat, CMDID_STAT_UPDATE, WG_DATAID, wgid,tmp_stat, len_frame_stat);
					tmpnode_res.DATAID = WG_DATAID;
					WG_DATAID++;
					tmpnode.cmdid = CMDID_STAT_UPDATE;
					tmpnode.DATA_LEN = len;
					memcpy(tmpnode.DATA,frame_stat,len);
					while(pthread_mutex_lock(&tcpsendlist_lock)!=0);
					AddFromEnd(head_tcpsendlist,&tmpnode);//将设备状态帧写到head_tcpsendlist
					memset(&tmpnode,'\0',sizeof(tmpnode));
					while(pthread_mutex_unlock(&tcpsendlist_lock)!=0);
					count = 0;
				}
			}

			//交易事件上传head_eventlist_old里面是一个个交易事件对应的充电桩指令
			memset(frame_event_data,'\0',sizeof(frame_event_data));
			if(chargingstat[i]->CEvent != 0x00)
			{
				print_log(LOG_DEBUG,"****************************事件********************\n");		
				memset(frame,'\0',sizeof(frame));
				psn_own = PSN++;
				if( (chargingstat[i]->CEvent & 0x01) == EVENT_READCARD )//卡片刷卡信息(成功，则下发一条扣款指令)
				{
					len = Frame_EventQuery(psn_own, chargingstat[i]->SN, EVENT_READCARD,frame);
					EventType = EVENT_READCARD;
					print_log(LOG_DEBUG,"****************************[%.12s]读卡片事件********************\n", chargingstat[i]->SN);
				}
				else if( (chargingstat[i]->CEvent & 0x02) == EVENT_PAYMENT )//卡片扣款交易
				{
					len = Frame_EventQuery(psn_own, chargingstat[i]->SN,EVENT_PAYMENT,frame);
					EventType = EVENT_PAYMENT;
					print_log(LOG_DEBUG,"****************************[%.12s]卡片扣款交易事件********************\n",chargingstat[i]->SN);
				}
				else if( (chargingstat[i]->CEvent & 0x04) == EVENT_CHARGCTL )//充电控制事件
				{
					len = Frame_EventQuery(psn_own, chargingstat[i]->SN, EVENT_CHARGCTL,frame);
					EventType = EVENT_CHARGCTL;
					print_log(LOG_DEBUG,"****************************[%.12s]充电控制事件********************\n",chargingstat[i]->SN);
				}
				writelen = serial_write(fd,frame, len);//事件查询
				Serialsend_debug(frame,len,CMD_EVENT_QUERY);
				if(writelen == len)
				{
					memset(frame,'\0',sizeof(frame));
					readlen = MGTR_UART_Read(fd,frame,512,MGTRURATR_OUTITME);
					Serialrecv_debug(frame,readlen,CMD_EVENT_QUERY);

					len = 0;
					len = MGTR_UART_rx_format(frame,readlen,rx_frame,psn_own);//20+Event
					if(len > 0)//事件查询指令正常回帧
					{
						if(rx_frame[5] == STATUS_SUCCESS)//事件查询指令执行成功
						{
							if(EventType == EVENT_READCARD)//卡片刷卡信息
							{
								/*************************************************/
								chargingstat[i]->CFlg = CTRLB_CARD;
								print_log(LOG_DEBUG,"chargingstat[i]->CFlg = CTRLB_CARD\n");
								/*************************************************/
								memcpy(carddata,rx_frame+20,25);//25bytes刷卡卡片信息
								memcpy(chargingstat[i]->CardID,carddata+1,sizeof(chargingstat[i]->CardID));
								memcpy(chargingstat[i]->CardSN,carddata+5,sizeof(chargingstat[i]->CardSN));//保存物理卡号、卡片序列号
								memcpy(chargingstat[i]->Yue,carddata+20,sizeof(chargingstat[i]->Yue));
								frame_eventdata_len = Create_Data_EventUp(frame_event_data, chargingstat[i],carddata, &flag,money);
								print_log(LOG_DEBUG,"卡片刷卡信息,money:%02x %02x %02x %02x  frame_eventdata_len:%d\n",money[0],money[1],money[2],money[3],frame_eventdata_len);

								/*清除交易事件*/
								while(1)
								{
									psn_own = PSN++;
									frame_clean_len = Frame_CleanEvent(psn_own,chargingstat[i],EVENT_READCARD,frame_clean);
									Serialsend_debug(frame_clean,frame_clean_len, CMD_CLEAN_TRAN);

									len = serial_write(fd,frame_clean,frame_clean_len);
									if(len == frame_clean_len)
									{
										frame_clean_len = MGTR_UART_Read(fd, frame, 512, MGTRURATR_OUTITME);
										Serialrecv_debug(frame, frame_clean_len,CMD_CLEAN_TRAN);
										ret = MGTR_UART_rx_format(frame,frame_clean_len,rx_frame,psn_own);
										if(ret > 0)
										{
											if(rx_frame[5] == STATUS_SUCCESS)
											{
												print_log(LOG_DEBUG,"读卡片事件-----清交易事件[%02x]-----成功\n",EVENT_READCARD);
												break;
											}
											else
											{
												print_log(LOG_DEBUG,"读卡片事件-----清交易事件[%02x]-----失败\n",EVENT_READCARD);
												if(cleannum > CLEANMAX)
												{
													cleannum = 0;
													break;
												}
												cleannum++;
												continue;
											}
										}
										else if(ret == -2)
										{
											print_log(LOG_DEBUG,"读卡片事件-----清交易事件指令-----超时\n",EVENT_READCARD);
											cleannum++;
											if(cleannum > CLEANMAX)
											{
												cleannum = 0;
												break;
											}
											continue;
										}
										else if(ret == -5)
										{
											print_log(LOG_DEBUG,"读卡片事件-----清交易事件指令-----格式不正确\n",EVENT_READCARD);
											cleannum++;
											if(cleannum > CLEANMAX)
											{
												cleannum = 0;
												break;
											}
											continue;
										}
									}
								}

								if(frame_eventdata_len == 0)//卡内余额足够支付下发扣款指令
								{
									//LOOP:
									psn_own = PSN++;
									writelen = Frame_payment(psn_own,chargingstat[i],frame_t);
									len = serial_write(fd,frame_t,writelen);
									Serialsend_debug(frame_t,writelen,CMD_PAYMENT_TRAN);
									if(len == writelen)
									{
										readlen = MGTR_UART_Read(fd, frame,512,MGTRURATR_OUTITME);
										Serialrecv_debug(frame,readlen,CMD_PAYMENT_TRAN);
										ret = MGTR_UART_rx_format(frame,readlen,rx_frame,psn_own);
										/**************************************************************************/
										if(ret > 0)
										{
											if(rx_frame[5] == STATUS_SUCCESS)//扣款成功
											{
												print_log(LOG_DEBUG,"扣款成功\n");
												/*将产生扣款交易事件*/;
											}
											//else if(rx_frame[5] == STATUS_BUSY)//状态忙
											else//设备忙或者其他原因导致扣款失败
											{
												/*重发机制把这一帧数据重新丢到head_serialsendlist里*/
												print_log(LOG_DEBUG,"卡片扣款指令-----状态忙-----将重新发送\n");
												sendnode.cmdid = CMDID_CHARGCONTROL;
												memcpy(sendnode.Charg_SN,chargingstat[i]->EventSN,sizeof(chargingstat[i]->EventSN));
												memcpy(sendnode.SN,chargingstat[i]->SN,sizeof(chargingstat[i]->SN));
												sendnode.DATA_LEN = writelen;
												memcpy(sendnode.DATA,frame_t,writelen);
												//old
												//AddFromEnd(head_serialsendlist,&sendnode);
												//new
												while(pthread_mutex_lock(&serialsendlist_lock)!=0);
												AddFromEnd(temp,&sendnode);
												while(pthread_mutex_unlock(&serialsendlist_lock)!=0);
											}
											/*
											   else//扣款失败
											   {
											   print_log(LOG_DEBUG,"扣款失败\n");
											   }
											   */
										}
										else if(ret == -2)
										{
											print_log(LOG_DEBUG,"卡片扣款指令-----超时-----将重新发送\n");
											sendnode.cmdid = CMDID_CHARGCONTROL;
											memcpy(sendnode.Charg_SN,chargingstat[i]->EventSN,sizeof(chargingstat[i]->EventSN));
											memcpy(sendnode.SN,chargingstat[i]->SN,sizeof(chargingstat[i]->SN));
											sendnode.DATA_LEN = writelen;
											memcpy(sendnode.DATA,frame_t,writelen);

											while(pthread_mutex_lock(&serialsendlist_lock)!=0);
											AddFromEnd(temp,&sendnode);
											while(pthread_mutex_unlock(&serialsendlist_lock)!=0);
										}
										else if(ret == -5)
										{
											print_log(LOG_DEBUG,"卡片扣款指令-----格式不正确-----将重新发送\n");
											sendnode.cmdid = CMDID_CHARGCONTROL;
											memcpy(sendnode.Charg_SN,chargingstat[i]->EventSN,sizeof(chargingstat[i]->EventSN));
											memcpy(sendnode.SN,chargingstat[i]->SN,sizeof(chargingstat[i]->SN));
											sendnode.DATA_LEN = writelen;
											memcpy(sendnode.DATA,frame_t,writelen);

											while(pthread_mutex_lock(&serialsendlist_lock)!=0);
											AddFromEnd(temp,&sendnode);
											while(pthread_mutex_unlock(&serialsendlist_lock)!=0);
										}
									}
								}
								else//卡内余额不足,上传读卡交易事件
								{	
									/*把交易事件报文frame_event写到队列里*/
									memset(frame_event,'\0',sizeof(frame_event));
									reslen = Response_Frame(frame_event,CMDID_DEAL_UPDATE,WG_DATAID,wgid,frame_event_data,frame_eventdata_len);
									memset(&tmpnode_res,'\0',sizeof(tmpnode_res));
									tmpnode_res.DATAID = WG_DATAID;
									tmpnode_res.cmdid = CMDID_DEAL_UPDATE;
									tmpnode_res.eventtype = frame_event_data[0];//事件类型
									memcpy(tmpnode_res.SN,chargingstat[i]->SN,sizeof(chargingstat[i]->SN));
									tmpnode_res.DATA_LEN = reslen;
									memcpy(tmpnode_res.DATA,frame_event,reslen);
									while(pthread_mutex_lock(&tcpsendlist_lock)!=0);
									AddFromEnd(head_tcpsendlist,&tmpnode_res);
									while(pthread_mutex_unlock(&tcpsendlist_lock)!=0);

									/*记录交易事件*/
									memset(&eventp,'\0',sizeof(eventp));
									memcpy(eventp.e_time,chargingstat[i]->DT,4);
									eventp.e_dataid = WG_DATAID;
									eventp.e_upflag = UPNOT;
									memcpy(eventp.e_sn,&EVENTSN,sizeof(EVENTSN));
									eventp.e_type = chargingstat[i]->CFlg;
									memcpy(eventp.e_data,frame_event_data,frame_eventdata_len);
									Record_Event(EVENTFILE, &eventp,EventIndex);
									Record_Event_Mem(eventbuf, &eventp,EventIndex);
									WG_DATAID++;
									snprintf(dataid_str,11,"%d",WG_DATAID);
									print_log(LOG_DEBUG,"dataid_str:%s %d\n",dataid_str,WG_DATAID);
									write_profile_string("WGID", "dataid", dataid_str,WGINFOINI);

									if(EventIndex > (MAXEVENT-1))
										EventIndex = 0;
									else 
										EventIndex++;
									snprintf(index_str,6,"%d",EventIndex);
									write_profile_string("WGID", "lasteventindex",index_str, WGINFOINI);
									EVENTSN++;

								}

							}
							else if(EventType == EVENT_PAYMENT)//卡片扣款交易
							{
								/*************************************************/
								chargingstat[i]->CFlg = CTRLB_PAYMENTEND;//先给一个临时的控制码，再转回到
								/*************************************************/
								memcpy(carddata,rx_frame+20,241);
								//组交易事件上传命令data部分
								frame_eventdata_len = Create_Data_EventUp(frame_event_data, chargingstat[i],carddata, &flag,money);
								print_log(LOG_DEBUG,"frame_eventdata_len:%d\n",frame_eventdata_len);
								if(frame_eventdata_len == -1)//扣款未确认,网关重新下发扣款指令进行扣款
								{
									print_log(LOG_DEBUG,"######################交易不确认需要网关重新刷卡判断######################\n");
									//扣款未确认的事件不上报，但也要先清除再下发扣款指令
									while(1)
									{
										psn_own = PSN++;
										frame_clean_len = Frame_CleanEvent(psn_own, chargingstat[i], EVENT_PAYMENT,frame_clean);
										Serialsend_debug(frame_clean,frame_clean_len, CMD_CLEAN_TRAN);
										len = serial_write(fd, frame_clean, frame_clean_len);
										if(len == frame_clean_len)
										{
											frame_clean_len = MGTR_UART_Read(fd, frame, 512, MGTRURATR_OUTITME);
											Serialrecv_debug(frame,frame_clean_len,CMD_CLEAN_TRAN);
											ret = MGTR_UART_rx_format(frame,frame_clean_len,rx_frame,psn_own);
											if(ret > 0)
											{
												if(rx_frame[5] == STATUS_SUCCESS)
												{
													print_log(LOG_DEBUG,"[%.12s]清交易事件[%02x]成功\n",chargingstat[i]->SN,EVENT_PAYMENT);
													break;
												}
												else
												{
													print_log(LOG_DEBUG,"[%.12s]清交易事件[%02x]失败\n",chargingstat[i]->SN,EVENT_PAYMENT);
													cleannum++;
													if(cleannum > CLEANMAX)
													{
														cleannum = 0;
														break;
													}
													continue;
												}
											}
											else if(ret == -2)
											{
												print_log(LOG_DEBUG,"卡片扣款交易事件---清交易事件指令---超时\n");
												cleannum++;
												if(cleannum > CLEANMAX)
												{
													cleannum = 0;
													break;
												}
												continue;
											}
											else if(ret == -5)
											{
												print_log(LOG_DEBUG,"卡片扣款交易事件---清交易事件指令---格式不正确\n");
												cleannum++;
												if(cleannum > CLEANMAX)
												{
													cleannum = 0;
													break;
												}
												continue;
											}
										}
									}

									psn_own = PSN++;
									writelen = Frame_payment(psn_own,chargingstat[i],frame_t);
									len = serial_write(fd,frame_t,writelen);
									Serialsend_debug(frame_t,writelen,CMD_PAYMENT_TRAN);
									if(len == writelen)
									{
										readlen = MGTR_UART_Read(fd, frame,512,MGTRURATR_OUTITME);
										Serialrecv_debug(frame,readlen,CMD_PAYMENT_TRAN);
										ret = MGTR_UART_rx_format(frame,readlen,rx_frame,psn_own);
										/**************************************************************************/
										if(ret > 0)
										{
											if(rx_frame[5] == STATUS_SUCCESS)//扣款成功
											{
												print_log(LOG_DEBUG,"扣款成功\n");
												/*将产生扣款交易事件*/;
											}
											//else if(rx_frame[5] == STATUS_BUSY)//状态忙
											else//设备忙或者其他原因失败重发
											{
												/*重发机制把这一帧数据重新丢到head_serialsendlist里*/
												print_log(LOG_DEBUG,"交易不确认-----卡片扣款指令-----状态忙\n");
												sendnode.cmdid = CMDID_CHARGCONTROL;
												memcpy(sendnode.Charg_SN,chargingstat[i]->EventSN,sizeof(chargingstat[i]->EventSN));
												memcpy(sendnode.SN,chargingstat[i]->SN,sizeof(chargingstat[i]->SN));
												sendnode.DATA_LEN = writelen;
												memcpy(sendnode.DATA,frame_t,writelen);
												//old
												//AddFromEnd(head_serialsendlist,&sendnode);
												//new
												while(pthread_mutex_lock(&serialsendlist_lock)!=0);
												AddFromEnd(temp,&sendnode);
												while(pthread_mutex_unlock(&serialsendlist_lock)!=0);
											}
											/*
											   else//扣款失败
											   {
											   print_log(LOG_DEBUG,"扣款成功\n");
											   }
											   */
										}
									}
								}
								else//扣款成功或者是失败
								{
									print_log(LOG_DEBUG,"eventtype:%02x\n",frame_event_data[0]);

									/*把交易事件报文frame_event写到队列里*/
									memset(frame_event,'\0',sizeof(frame_event));
									memset(&tmpnode_res,'\0',sizeof(tmpnode_res));
									tmpnode_res.eventtype = frame_event_data[0];//事件类型
									reslen = Response_Frame(frame_event,CMDID_DEAL_UPDATE,WG_DATAID,wgid,frame_event_data,frame_eventdata_len);
									tmpnode_res.DATAID = WG_DATAID;
									tmpnode_res.cmdid = CMDID_DEAL_UPDATE;

									tmpnode_res.DATA_LEN = reslen;
									memcpy(tmpnode_res.SN,chargingstat[i]->SN,sizeof(chargingstat[i]->SN));
									memcpy(tmpnode_res.DATA,frame_event,reslen);
									while(pthread_mutex_lock(&tcpsendlist_lock)!=0);
									AddFromEnd(head_tcpsendlist,&tmpnode_res);
									while(pthread_mutex_unlock(&tcpsendlist_lock)!=0);
									/*记录交易事件*/
									memset(&eventp,'\0',sizeof(eventp));
									memcpy(eventp.e_time,chargingstat[i]->DT,4);
									eventp.e_dataid = WG_DATAID;
									eventp.e_upflag = UPNOT;
									memcpy(eventp.e_sn,&EVENTSN,sizeof(EVENTSN));
									/*************************************************/
									chargingstat[i]->CFlg = CTRLB_CARD;
									eventp.e_type = chargingstat[i]->CFlg;//再转回到刷卡事件
									/*************************************************/
									memcpy(eventp.e_data,frame_event_data,frame_eventdata_len);
									Record_Event(EVENTFILE, &eventp,EventIndex);
									Record_Event_Mem(eventbuf, &eventp,EventIndex);
									WG_DATAID++;
									snprintf(dataid_str,11,"%d",WG_DATAID);
									print_log(LOG_DEBUG,"dataid_str:%s %d\n",dataid_str,WG_DATAID);
									write_profile_string("WGID", "dataid", dataid_str,WGINFOINI);

									if(EventIndex > (MAXEVENT-1))
										EventIndex = 0;
									else 
										EventIndex++;
									snprintf(index_str,6,"%d",EventIndex);
									write_profile_string("WGID", "lasteventindex",index_str, WGINFOINI);
									EVENTSN++;
									//事件清除
									while(1)
									{
										psn_own = PSN++;
										frame_clean_len = Frame_CleanEvent(psn_own, chargingstat[i], EVENT_PAYMENT,frame_clean);
										Serialsend_debug(frame_clean,frame_clean_len, CMD_CLEAN_TRAN);
										len = serial_write(fd, frame_clean, frame_clean_len);
										if(len == frame_clean_len)
										{
											frame_clean_len = MGTR_UART_Read(fd, frame, 512, MGTRURATR_OUTITME);
											Serialrecv_debug(frame,frame_clean_len,CMD_CLEAN_TRAN);
											ret = MGTR_UART_rx_format(frame,frame_clean_len,rx_frame,psn_own);
											if(ret > 0)
											{
												if(rx_frame[5] == STATUS_SUCCESS)
												{
													print_log(LOG_DEBUG,"扣款交易指令-----清交易事件[%02x]-----成功\n",EVENT_PAYMENT);
													break;
												}
												else
												{
													print_log(LOG_DEBUG,"扣款交易指令-----清交易事件[%02x]失败---设备忙或者是其他失败原因\n",EVENT_PAYMENT);
													cleannum++;
													if(cleannum > CLEANMAX)
													{
														cleannum = 0;
														break;
													}
													continue;
												}
											}
											else if(ret == -2)
											{
												print_log(LOG_DEBUG,"扣款交易指令---清交易事件指令---超时\n");
												cleannum++;
												if(cleannum > CLEANMAX)
												{
													cleannum = 0;
													break;
												}
												continue;
											}
											else if(ret == -5)
											{
												print_log(LOG_DEBUG,"充电控制事件---清交易事件指令---格式不正确\n");
												cleannum++;
												if(cleannum > CLEANMAX)
												{
													cleannum = 0;
													break;
												}
												continue;
											}
										}
									}
								}
							}
							else if(EventType == EVENT_CHARGCTL)//充电控制事件
							{	
								print_log(LOG_DEBUG,"****************************1 充电控制事件********************\n");
								chargingstat[i]->CFlg = rx_frame[20];//获取充电桩控制事件号
								//memcpy(chargingstat[i]->DT,rx_frame+21,sizeof(chargingstat[i]->DT));
								//memcpy(chargingstat[i]->ENERGY,rx_frame+28,sizeof(chargingstat[i]->ENERGY));//当前电能当前电量
								if(chargingstat[i]->CFlg == CTRLB_START)
								{	
									memcpy(chargingstat[i]->SCT,rx_frame+21,sizeof(chargingstat[i]->SCT));
									print_log(LOG_DEBUG,"-SCT:%02x %02x %02x %02x %02x %02x %02x\n",chargingstat[i]->SCT[0],chargingstat[i]->SCT[1],chargingstat[i]->SCT[2],chargingstat[i]->SCT[3],chargingstat[i]->SCT[4],chargingstat[i]->SCT[5],chargingstat[i]->SCT[6]);
									memcpy(chargingstat[i]->STARTENERGY,rx_frame+28,sizeof(chargingstat[i]->STARTENERGY));
								}
								else if(chargingstat[i]->CFlg == CTRLB_END)
								{
									memcpy(chargingstat[i]->ENDENERGY,rx_frame+28,sizeof(chargingstat[i]->ENDENERGY));
								}
								Record_Status(StatusFile,chargingstat[i],i,port->portnum);
								//组交易事件上传命令data部分
								print_log(LOG_DEBUG,"--------------%02x\n",chargingstat[i]->CFlg);
								frame_eventdata_len = Create_Data_EventUp(frame_event_data, chargingstat[i],carddata, &flag,money);
								print_hexoflen(frame_event_data, frame_eventdata_len);
								/*把交易事件报文frame_event写到队列里*/
								memset(frame_event,'\0',sizeof(frame_event));
								reslen = Response_Frame(frame_event,CMDID_DEAL_UPDATE,WG_DATAID,wgid,frame_event_data,frame_eventdata_len);
								memset(&tmpnode_res,'\0',sizeof(tmpnode_res));									
								tmpnode_res.DATAID = WG_DATAID;
								tmpnode_res.cmdid = CMDID_DEAL_UPDATE;
								tmpnode_res.eventtype = frame_event_data[0];//事件类型
								memcpy(tmpnode_res.SN,chargingstat[i]->SN,sizeof(chargingstat[i]->SN));
								print_hexoflen(frame_event, reslen);

								tmpnode_res.DATA_LEN = reslen;
								memcpy(tmpnode_res.DATA,frame_event,reslen);
								while(pthread_mutex_lock(&tcpsendlist_lock)!=0);
								AddFromEnd(head_tcpsendlist,&tmpnode_res);
								while(pthread_mutex_unlock(&tcpsendlist_lock)!=0);
								/*记录交易事件*/
								memset(&eventp,'\0',sizeof(eventp));
								memcpy(eventp.e_time,chargingstat[i]->DT,4);
								eventp.e_dataid = WG_DATAID;
								eventp.e_upflag = UPNOT;
								memcpy(eventp.e_sn,&EVENTSN,sizeof(EVENTSN));
								eventp.e_type = chargingstat[i]->CFlg;
								memcpy(eventp.e_data,frame_event_data,frame_eventdata_len);
								WG_DATAID++;
								snprintf(dataid_str,11,"%d",WG_DATAID);
								print_log(LOG_DEBUG,"dataid_str:%s %d\n",dataid_str,WG_DATAID);
								write_profile_string("WGID", "dataid", dataid_str,WGINFOINI);

								print_log(LOG_DEBUG,"eventp.e_time:%02x %02x %02x %02x\n",chargingstat[i]->DT[0],chargingstat[i]->DT[1],chargingstat[i]->DT[2],chargingstat[i]->DT[3]);
								Record_Event(EVENTFILE, &eventp,EventIndex);
								Record_Event_Mem(eventbuf, &eventp,EventIndex);
								if(EventIndex > (MAXEVENT-1))
									EventIndex = 0;
								else 
									EventIndex++;
								printf("EventIndex:%d\n",EventIndex);
								snprintf(index_str,5,"%d",EventIndex);
								printf("index_str:%s    EventIndex:%d\n",index_str,EventIndex);
								write_profile_string("WGID", "lasteventindex",index_str, WGINFOINI);
								EVENTSN++;
								//事件清除
								while(1)
								{
									psn_own = PSN++;
									frame_clean_len = Frame_CleanEvent(psn_own, chargingstat[i], EVENT_CHARGCTL,frame_clean);
									Serialsend_debug(frame_clean,frame_clean_len, CMD_CLEAN_TRAN);
									len = serial_write(fd, frame_clean, frame_clean_len);
									if(len == frame_clean_len)
									{
										len = MGTR_UART_Read(fd, frame, 512, MGTRURATR_OUTITME);
										Serialrecv_debug(frame,len,CMD_CLEAN_TRAN);
										ret = MGTR_UART_rx_format(frame,len,rx_frame,psn_own);
										if(ret > 0)
										{
											if(rx_frame[5] == STATUS_SUCCESS)
											{
												print_log(LOG_DEBUG,"充电控制事件---清交易事件[%02x]---成功\n",EVENT_CHARGCTL);
												break;
											}
											else//设备忙或者是其他失败原因
											{
												print_log(LOG_DEBUG,"充电控制事件---清交易事件[%02x]---失败---设备忙或者是其他失败原因\n",EVENT_CHARGCTL);
												cleannum++;
												if(cleannum > CLEANMAX)
												{
													cleannum = 0;
													break;
												}
												continue;
											}
										}
										else if(ret == -2)
										{
											print_log(LOG_DEBUG,"充电控制事件---清交易事件指令---超时\n");
											cleannum++;
											if(cleannum > CLEANMAX)
											{
												cleannum = 0;
												break;
											}
											continue;
										}
										else if(ret == -5)
										{
											print_log(LOG_DEBUG,"充电控制事件---清交易事件指令---格式不正确\n");
											cleannum++;
											if(cleannum > CLEANMAX)
											{
												cleannum = 0;
												break;
											}
											continue;
										}
									}
								}

							}

						}
						else//设备忙
						{
							print_log(LOG_DEBUG,"事件查询指令设备忙\n");
						}
					}
					else if(ret == -2)//收不到数据超时
					{
						print_log(LOG_DEBUG,"事件查询指令超时\n");
					}
					else if(ret == -5)//格式不正确
					{
						print_log(LOG_DEBUG,"事件查询指令回帧格式不正确\n");
					}


				}

			}


			/*每轮询一个充电桩，读一次head_serialsendlist,
			  head_serialsendlist里包括透传指令、充电控制指令
			  有数据则处理,无数据继续轮询*/
			if(temp->next != NULL)
			{
				while(pthread_mutex_lock(&serialsendlist_lock)!=0);
				DelFromHead(temp,&delnode);
				while(pthread_mutex_unlock(&serialsendlist_lock)!=0);
				psn_own = delnode.frame_psn;
				//如果控制指令下发到正在升级中的充电桩，则忽略掉该指令
				GetStatus_SN(&strquery,chargingstat,max,&delnode);
				if(strquery.UpStatus == UPDATING){
					print_log(LOG_DEBUG,"[%.12s]升级中...不能执行该指令\n",strquery.SN);
					break;
				}
				//判是否是读卡指令以及对应的桩状态是否在0x05 - 交易故障
				print_log(LOG_DEBUG,"chargingstat[i]->CStatus:%02x delnode.cmdid:%02x delnode.controlnum:%02x\n",chargingstat[i]->CStatus,delnode.cmdid,delnode.controlnum);

				if((delnode.cmdid == 0x04)&&(delnode.controlnum == CTRLB_CARD))
				{
					//通过SN去chargingstat里找到下标
					index = getIndex_BySN(chargingstat,max,&delnode);
					print_log(LOG_DEBUG,"will LOOP index:%d\n",index);
					if((index >= 0) && (chargingstat[index]->CStatus == 0x05))
					{
						i = index;
						print_log(LOG_DEBUG,"goto LOOP index:%d\n",index);
						psn_own = PSN++;
						writelen = Frame_payment(psn_own,chargingstat[i],frame_t);
						memcpy(delnode.DATA,frame_t,writelen);//把寻卡指令转为扣款指令
						delnode.DATA_LEN = writelen;
						print_hexoflen(delnode.DATA,writelen);
					}

				}

				/*充电控制命令将产生事件,已发送的保存一份
				  用于上报事件的时候查找相应的 数据组上报事件格式帧*/
				if(delnode.cmdid == CMDID_CHARGCONTROL)
				{
					//方式一通过把已发送的控制指令放到head_eventlist_old，供事件查询的时候匹配
					//AddFromEnd(head_eventlist_old, &delnode);
					//方式二通过充电桩SN匹配把事件序号赋给对应的充电桩的事件序号
					SetStatuslist_EventSN(chargingstat,max,&delnode);//充电桩状态结构已经有交易序号了
				}

				/*透传指令中超时时间为0x00时是广播报文,
				  无应答报文,只发不收网关直接回复服务器下发成功*/
				if( (delnode.cmdid == CMDID_PASSTHROUGH) && (delnode.Serialtimeout == 0x00) )
				{
					writelen = serial_write(fd,delnode.DATA,delnode.DATA_LEN);
					if(writelen == delnode.DATA_LEN)
					{
						frame_t[0] = 0x00;
						readlen = 1;
						//delnode.cmdid += 0x80;
						reslen = Response_Frame(res_frame, delnode.cmdid,delnode.DATAID,delnode.T_SN,frame_t,readlen);
					}
				}
				else if(delnode.cmdid == CMDID_PROCESS_QUERY)
				{
					print_hexoflen(delnode.SN,12);
					ret = GetStatus_SN( &strquery,chargingstat,max,&delnode);
					print_log(LOG_DEBUG,"sn:%12s\nret :%d\n",strquery.SN,ret);
					memset(data_query,'\0',sizeof(data_query));
					data_query_l= Create_Data_Query(data_query,&strquery);
					reslen = Response_Frame(res_frame, delnode.cmdid, delnode.DATAID, delnode.T_SN,data_query, data_query_l);
				}
				else//设备充电控制指令
				{
					writelen = serial_write(fd,delnode.DATA,delnode.DATA_LEN);
					print_log(LOG_DEBUG,"设备充电控制指令\n");
					Serialsend_debug(delnode.DATA,delnode.DATA_LEN,'0');
					if(writelen == delnode.DATA_LEN)
					{
						memset(frame,'\0',sizeof(frame));
						readlen = MGTR_UART_Read(fd,frame,512,MGTRURATR_OUTITME);
						Serialrecv_debug(frame, readlen, '0');
						if(readlen > 0)
						{
							len = 0;
							len = MGTR_UART_rx_format(frame,readlen,rx_frame,psn_own);
							//len > 0串口指令执行成功
							if(len > 0)
							{
								GetStatus_BySN(chargingstat, max,delnode.SN,&dststat);
								//充电桩指令执行状态码success	
								if(rx_frame[5] == STATUS_SUCCESS)
								{
									print_log(LOG_DEBUG,"充电控制指令执行状态码成功\n");
									memset(frame_t,'\0',sizeof(frame_t));
									//指令执行状态码
									/*回应服务端cmdid = 原来的+0x80
									  数据透传命令成功要上报服务器*/
									if(delnode.cmdid == CMDID_PASSTHROUGH)
									{
										//应答给服务器的数据是充电桩返回来的原始数据包括转义
										frame_t[0] = rx_frame[5];
										memcpy(frame_t+1,frame,readlen);
										readlen += 1;
										reslen = Response_Frame(res_frame, delnode.cmdid,delnode.DATAID,delnode.T_SN,frame_t,readlen);
									}
									else if(delnode.cmdid == CMDID_CHARGCONTROL)//充电过程控制命令
									{
										/*充电控制指令成功将串口数据应答给服务器,这时充电桩将产生事件*/
										frame_t[0] = rx_frame[5];
										frame_t[1] = 0x00;
										frame_t[2] = dststat.CStatus;
										frame_t[3] = 0x00;
										frame_t[4] = dststat.Hw_Status;
										readlen = 5;
										reslen = Response_Frame(res_frame, delnode.cmdid,delnode.DATAID,delnode.T_SN,frame_t,readlen);
										if(delnode.controlnum == CTRLB_END)
										{
											if(delnode.flag1 == 0x01)
											{
												/*保存服务器结束充电下发的开始充电时间电量*/
												SetStatuslist_SCTDN(chargingstat,max,&delnode);
											}
										}
										else if( (delnode.controlnum == CTRLB_CARD) || (delnode.controlnum == CTRLB_ETC) )
										{
											/*保存服务器选择交通卡下发的收费金额以及是否有效*/
											if(delnode.flag2 == 0x01)
											{
												ret = SetStatuslist_Pay(chargingstat,max,&delnode);
												if(ret == 0)
													print_log(LOG_DEBUG,"选择交通卡付费下发费用有效-----保存服务器下发下来的收费金额以及标志\n");
											}
										}
									}
								}
								else//充电桩指令执行状态码fail
								{	
									print_log(LOG_DEBUG,"充电控制指令执行状态码失败\n");

									//数据透传命令失败上报服务器
									if(delnode.cmdid == CMDID_PASSTHROUGH)
									{
										//回应服务端cmdid = 原来的+0x80
										buff_fail[0] =  rx_frame[5];
										reslen = Response_Frame(res_frame, delnode.cmdid,delnode.DATAID,delnode.T_SN,buff_fail,1);
										if(frame[3] == STATUS_BUSY)//充电桩状态忙要重新发送该指令
										{
											while(pthread_mutex_lock(&serialsendlist_lock)!=0);
											AddFromEnd(temp, &delnode);
											while(pthread_mutex_unlock(&serialsendlist_lock)!=0);
										}
										else
										{
											/*********其他错误状态则暂不处理********/;
										}
									}
									else if(delnode.cmdid == CMDID_CHARGCONTROL)//充电过程控制命令
									{
										buff_fail[0] = 0x01;//失败

										if( (rx_frame[5] == STATUS_WRONG)||(dststat.Hw_Status != STATUS_HWOK) )
										{
											if(rx_frame[5] == STATUS_WRONG)
												buff_fail[1] = 0x01;
											else
												buff_fail[1] = 0x00;
											buff_fail[2] = dststat.CStatus;

											if(dststat.Hw_Status != STATUS_HWOK)
												buff_fail[3] = 0x01;
											else
												buff_fail[3] = 0x00;
											buff_fail[4] = dststat.Hw_Status;
											reslen = Response_Frame(res_frame, delnode.cmdid,delnode.DATAID,delnode.T_SN,buff_fail,5);
											print_log(LOG_DEBUG,"rx_frame[5] = %02x   reslen:%d\n",rx_frame[5],reslen);
											print_hexoflen(buff_fail,5);
										}
										else if(rx_frame[5] == STATUS_BUSY){
											/*充电控制指令下发读取回复失败以后重新将事件放到链表里*/
											while(pthread_mutex_lock(&serialsendlist_lock)!=0);
											AddFromEnd(temp, &delnode);
											while(pthread_mutex_unlock(&serialsendlist_lock)!=0);
										}
									}

								}
							}
						}
						else
						{
							//*****无应答***** 则重新将指令放到链表里
							while(pthread_mutex_lock(&serialsendlist_lock)!=0);
							AddFromEnd(temp, &delnode);
							while(pthread_mutex_unlock(&serialsendlist_lock)!=0);
						}
					}
				}
				//将回应报文写到head_tcpsendlist里
				memset(&sendnode,'\0',sizeof(struct node));
				memcpy(&sendnode,&delnode,sizeof(struct node));
				sendnode.DATA_LEN = reslen;
				memcpy(sendnode.DATA,res_frame,reslen);
				while(pthread_mutex_lock(&tcpsendlist_lock)!=0);
				AddFromEnd(head_tcpsendlist,&sendnode);
				while(pthread_mutex_unlock(&tcpsendlist_lock)!=0);
			}
		}

		flag_getdevmsg = 1;
		if((count >0) && (count != maxdevice)){
			len = Response_Frame(frame_stat, CMDID_STAT_UPDATE, WG_DATAID, wgid,tmp_stat, len_frame_stat);
			WG_DATAID++;
			tmpnode.cmdid = CMDID_STAT_UPDATE;
			tmpnode.DATA_LEN = len;
			tmpnode.T_SN = wgid;
			memcpy(tmpnode.DATA,frame_stat,len);
			while(pthread_mutex_lock(&tcpsendlist_lock)!=0);
			AddFromEnd(head_tcpsendlist,&tmpnode);//将设备状态帧写到head_tcpsendlist
			while(pthread_mutex_unlock(&tcpsendlist_lock)!=0);
			print_log(LOG_DEBUG,"33333333333333333333333cmdid:%d    datalen:%d\n",head_tcpsendlist->next->cmdid,head_tcpsendlist->next->DATA_LEN);

			memset(&tmpnode,'\0',sizeof(tmpnode));
			count = 0;
		}



	}


	//free
	if(chargingstat != NULL){
		for(i = 0;i < MAX_CHARGING_NUM;i++){
			free(chargingstat[i]);
		}
		free(chargingstat);
		chargingstat = NULL;
	}


	return NULL;
}

