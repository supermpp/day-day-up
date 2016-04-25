#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "linklist.h"
#include "common.h"

struct node *head_tcpsendlist;
struct node *head_tcphavesendlist;
struct node *head_serialsendlist;

//new
/*每一个串口有自己的串口数据链表*/
struct node *head_serialsendlist1;
struct node *head_serialsendlist2;
struct node *head_serialsendlist3;
struct node *head_serialsendlist4;
struct node *head_serialsendlist5;
struct node *head_serialsendlist6;

//用于上报事件的时候查找相应的 数据组上报事件格式帧
struct node *head_eventlist_old;//存储已经发送过的事件

pthread_mutex_t tcpsendlist_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t serialhavesendlist_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t serialsendlist_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t serialsendlist1_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t serialsendlist2_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t serialsendlist3_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t serialsendlist4_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t serialsendlist5_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t serialsendlist6_lock = PTHREAD_MUTEX_INITIALIZER;
void InitList(struct node **p)
{
	*p = (struct node *)malloc(sizeof(struct node));
	if((*p) != NULL){
		(*p)->next = NULL;
	}
}
#if 0
struct node *InitList()
{
	struct node *head;
	head = (struct node *)malloc(sizeof(struct node));
	if(!head)
		return 0;
	head->next = NULL;

	return head;
}
#endif
//从尾部开始插入链表
struct  node *AddFromEnd(struct node *head,struct node *new )
{
	struct node *p = head;
	if(head == NULL){
		head = new;
		new->next = NULL;
	}
	else{
		while(p->next != NULL)
			p = p->next;
		p->next = (struct node *)malloc(sizeof(struct node));
		memcpy(p->next,new,sizeof(struct node));
		//print_log(LOG_DEBUG,"============cmdid:%02x len:%d============\n",p->next->cmdid,p->next->DATA_LEN);
		p->next->next = NULL;
	}
	return head;
}

//从头开始删除第一个，将删除的结果放到deldata里
struct node *DelFromHead(struct node *head,struct node *deldata)
{
	struct node *tmp,*del;
	if(head == NULL){
		printf("head is null\n");
		deldata = NULL;
	}
	else{
		tmp = head;
		if(tmp->next == NULL){
			printf("list is null\n");
			deldata = NULL;
		}
		else if(tmp->next->next == NULL){
			del = tmp->next;
			memcpy(deldata,del,sizeof(struct node));
			free(del);
			tmp->next = NULL;

		}
		else{
			del = tmp->next;
			head->next = del->next;
			del->next = NULL;
			memcpy(deldata, del,sizeof(struct node));
			free(del);
		}
	}
	return head;
}
//以key为条件查找链表，结果返回给dstnode
int Findlist(struct node *head,struct node *dstnode,unsigned int key)
{
	struct node *p = head;
	if(p == NULL){
		printf("head is null\n");
		dstnode = NULL;
		return -1;
	}
	else{
		p = p->next;
		if(p == NULL){
			printf("list is null\n");
			dstnode = NULL;
			return -1;
		}
		else{
			while(1){
				if(p->DATAID != key){
					p = p->next;
					if(p->next == NULL){
						if(p->DATAID == key){
							memcpy(dstnode,p,sizeof(struct node));
							break;
						}
						else{
							dstnode = NULL;
							return -1;
						}
					}
				}	
				else{
					memcpy(dstnode,p,sizeof(struct node));
					break;
				}
			}
		}
	}
	return 0;
}
//以key为条件进行删除,将删除的节点放到dstnode里
int DelDesignedlist(struct node *head,struct node *dstnode,int key)
{
	struct node *p = head,*ahead = NULL,*behind = NULL;
	if(p == NULL){
		printf("head is null\n");
		return -1;
	}
	else{
		if(p->next == NULL){
			printf("list is null\n");
			return -1;
		}
		else{
			while(1){
				ahead = p;
				p = p->next;
				if(p->DATAID == key){
					if(p->next == NULL){
						memcpy(dstnode,p,sizeof(struct node));
						free(p);
						ahead->next = NULL;
						break;
					}
					else{
						memcpy(dstnode,p,sizeof(struct node));
						behind = p->next;
						ahead->next = behind;
						free(p);
						break;
					}
				}
				else{
					if(p->next == NULL && p->DATAID != key){
						printf("not find\n");
						return -1;
					}
				}
			}
		}
	}
	return 0;
}
void print_list(struct node*head)
{
	struct node *p = head;
	while(1){
		if(p->next != NULL){
			printf("[%d]\n",p->DATAID);
			p = p->next;
		}
		else{
			printf("[%d]\n",p->DATAID);
			break;
		}
	}
	return ;
}

/*
通过SN匹配把交易事件赋给对应充电桩
num:二位数组成员数
key:SN充电桩编号
*/
int SetStatuslist_EventSN(struct status **p,int num,struct node *src)
{
	int i = 0;
	for(i = 0;i < num;i++){
		if(memcmp(p[i]->SN,src->SN,12) == 0){
			if(1){
				memcpy(p[i]->EventSN,src->Charg_SN,sizeof(src->Charg_SN));
			}
			return 0;
		}
	}
	return -1;
}
/*
通过SN匹配找到充电桩的状态
num:二维数组成员数
SN:要匹配的充电桩SN
dst:匹配到的充电桩的状态结构
返回值:0匹配成功-1匹配失败
*/
int GetStatus_BySN(struct status **p,int num,unsigned char *SN,struct status *dst)
{
	int i = 0;
	for(i = 0;i < num;i++){
		if(memcmp(p[i]->SN,SN,12) == 0){
			memcpy(dst,p[i],sizeof(struct status));
			return 0;
		}
	}
	return -1;
}


/*
充电过程控制命令
通过SN匹配把服务器传下来的开始充电时间电量赋给对应充电桩
num:二位数组成员数
key:SN充电桩编号
*/
int SetStatuslist_SCTDN(struct status **p,int num,struct node *src)
{
	int i = 0;
	for(i = 0;i < num;i++){
		if(memcmp(p[i]->SN,src->SN,12) == 0){
			if(1){
				memcpy(p[i]->SCT,src->Stime,sizeof(src->Stime));
				memcpy(p[i]->STARTENERGY,src->SDn,sizeof(src->SDn));
			}
			return 0;
		}
	}
	return -1;
}

/*
充电过程控制命令
通过SN匹配把服务器传下来的充电费用赋给对应充电桩
num:二位数组成员数
key:SN充电桩编号
*/
int SetStatuslist_Pay(struct status **p,int num,struct node *src)
{
	int i = 0;
	for(i = 0;i < num;i++){
		if(memcmp(p[i]->SN,src->SN,12) == 0){
			if(1){
				//memcpy(&(p[i]->MoneyCharg),src->Pay,sizeof(src->Pay));
				p[i]->Flag1 = src->flag2;
				p[i]->MoneyCharg = src->Pay;
			}
			return 0;
		}
	}
	return -1;
}
/*
通过SN匹配把服务器传下来的充电过程查询的充电桩的状态传到dst里
src:充电桩状态二维数组
dst:要存放的匹配到的充电桩的状态
num:二位数组成员数
key:SN充电桩编号
*/
int GetStatus_SN(struct status *dst,struct status **src,int num,struct node *p)
{
	int i = 0;
	for(i = 0;i < num;i++){
		if(memcmp(src[i]->SN,p->SN,12) == 0){
			if(1){
				memcpy(dst,src[i],sizeof(struct status));
			}
			return 0;
		}
	}
	return -1;
}

/*
通过SN匹配查找下标
*/
int getIndex_BySN(struct status **src,int num,struct node *p)
{
	int i = 0;
	for(i = 0;i < num;i++){
		if(memcmp(src[i]->SN,p->SN,12) == 0)
			return i;
	}
	return -1;
}



/*充电桩属性链表*/
//初始化

#if 0
struct status *InitList_status(void)
{
	struct status*head ;
	head = (struct status *)malloc(sizeof(struct status));
	if(!head)
		return 0;
	head->next = NULL;
	return head;
}
//尾部添加一个
struct  status *AddFromEnd_status(struct status *head,struct status *new)
{
	struct status *p = head;
	if(head == NULL){
		head = new;
		new->next = NULL;
	}
	else{
		while(p->next != NULL)
			p = p->next;
		p->next = (struct status *)malloc(sizeof(struct status));
		memcpy(p->next,new,sizeof(struct status));
		p->next->next = NULL;
	}
	return head;
}
#endif


