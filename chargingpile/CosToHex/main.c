#include <stdio.h>
#include <string.h>
#define OUTFILE "./pileupdatefile"
void s2bcd(void *d, void *s,const int len) 
{
	int i;
	unsigned char a,b;
	unsigned char *dest,*src;
	dest=(unsigned char *)d;
	src=(unsigned char *)s;
	for(i=0; i<len;i++)              /*"65"-->0x65*/
	{
		a=src[2*i]<='9'?src[2*i]:src[2*i]-'A'+0x0a;
		b=src[2*i+1]<='9'?src[2*i+1]:src[2*i+1]-'A'+0x0a;
		dest[i]   = ((a&0x0f)<<4)|(b&0x0f);
	}
	return;
}
int main(int argc,char **argv)
{
	char buff[3000] = {0},data[1034] = {0};
	FILE *fpr = NULL,*fpw;
	fpr = fopen(argv[1],"r");
	fpw = fopen(OUTFILE,"a+");
	int i = 0,j = 0;
	while(1){
		memset(buff,'\0',sizeof(buff));
		if(fgets(buff,sizeof(buff),fpr) == NULL)
			break;
		if(i == 0){
			i++;
			printf("%s\n",buff);
			s2bcd(data,buff,68);
			fwrite(data,34,1,fpw);
		}
		else{
			i++;
			printf("%s\n",buff);
			s2bcd(data,buff,2068);
			fwrite(data,1034,1,fpw);
		}
	}		
	
	fclose(fpr);
	fclose(fpw);	
	printf("文件个数:%d\n",i);
	return 0;
}
