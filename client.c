#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sqlite3.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include "client.h"


/****************************************************************************** 
函数名称：recvThread 
简要描述：客户端接收消息线程  

输入：_ClientSocket  套接字描述符        
输出：    
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/ 
void recvThread(void* _ClientSocket)			 			//接收线程
{
	iClientSocket = *(int*)_ClientSocket;
	MsgData msg;
    while(1)
	{
		if(recv(iClientSocket,&msg,sizeof(MsgData),0) == -1)//每次都判断
		{
			printf("服务器断开链接\n");
			exit(-1);
		}
		switch(msg.m_cmd)
		{
			case CLI_CMD_GROUPCHAT:
								printf("（群聊）%s : %s\n",msg.m_name,msg.m_mess);
								vSaveChatRecord(&msg,1);
								break;	
			case CLI_CMD_USR_OFFLINE:
								isChatOneOnline = 1;//没找到私聊的人或已下下线
								break;
			case CLI_CMD_PERSONCHAT: 
								printf("（私聊）%s对你说：%s\n",msg.m_fromName,msg.m_mess);
								vSaveChatRecord(&msg,0);
								break;
			case CLI_CMD_CHK_OL_USER:				
								printf("%s\n",msg.m_name);//在线人员
								break;			
			case CLI_CMD_SEND_FILE:
								printf("文件发送成功\n正在返回群聊........\n");
								break;
			case CLI_CMD_FILE_ERROR:
								printf("文件发送失败（此人不群聊中不能接收文件）\n正在返回群聊........\n");
								break;
			case CLI_CMD_SAVE_FILE:
								vSaveFile(&msg);
								printf("从%s处接收到一个文件\n",msg.m_fromName);
								break;
			case CLI_CMD_ERROR:
								printf("一些意想不到的错误发生了\n");
								break;
			case CLI_CMD_SENF_OFFLINEMSG:
								vReadOfflineRecord(&msg);
								break;
			case CLI_CMD_USEROFFLINE:
								printf("对方不存在或不在线\n");
								break;

		}
	}
}

/****************************************************************************** 
函数名称：vSaveChatRecord 
简要描述：本地保存聊天记录  

输入：_msg  数据包
	  _flag	1为群聊的聊天记录  0为私聊的聊天记录
输出：    
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/ 
void vSaveChatRecord(MsgData *_msg,int _flag)		//保存聊天记录分俩种，群聊私聊分开
{
	FILE *fp = fopen("localChat.txt","a+");
	ChatRecord temp;
	if(_flag == 1)
	{
		strcpy(temp.m_toName,"All");
		strcpy(temp.m_fromName,_msg->m_name);
		temp.m_flag = 1;
	}
	else if(_flag == 0)
	{
		strcpy(temp.m_toName,"you");
		strcpy(temp.m_fromName,_msg->m_fromName);
		temp.m_flag = 0;
	}
	else
	{
		strcpy(temp.m_toName,_msg->m_toName);
		strcpy(temp.m_fromName,_msg->m_fromName);
		temp.m_flag = 0;
	}
	strcpy(temp.m_mess,_msg->m_mess);
	strcpy(temp.m_time,pcGetTime());
	fwrite(&temp,sizeof(ChatRecord),1,fp);//将结构体直接写入文件

	fclose(fp);
}
/****************************************************************************** 
函数名称：pcGetTime 
简要描述：获取时间函数  

输入：
输出：时间   
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/ 
char *pcGetTime()							
{
	time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
	return asctime(timeinfo);
}

/****************************************************************************** 
函数名称：vSaveFile 
简要描述：获取时间函数  

输入：_msg	数据包
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/ 
void vSaveFile(MsgData *_msg)
{
	char buf[1024] = {0};
	sprintf(buf,"%s",_msg->m_fileName);
	int pd = open(buf, O_WRONLY|O_CREAT);		
	if (pd == -1)
    {
        perror ("error");
        return;
    }
	memset(buf,0,sizeof(buf));
    strcpy(buf, _msg->m_mess);
    write(pd,buf,strlen(buf));
	close(pd);	
}

/****************************************************************************** 
函数名称：vSaveFile 
简要描述：打印登录界面菜单  

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/ 
void vFirstMenu()
{
	printf("\n");
	printf("\tMeChat\t\n");
	printf("\t*****************\n");
	printf("\t1.注册\n");
	printf("\t2.登录\n");
	printf("\t3.退出\n");
	printf("\t*****************\n");
}

/****************************************************************************** 
函数名称：iGetRand 
简要描述：根据时间获取随机数 用于生成用户账号

输入：
输出：	  整形随机数
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/ 
int iGetRand()
{
	int temp;
	srand((unsigned) time(NULL));				//时间函数生成随机
	temp = rand()%10000;
	return temp;
}

/****************************************************************************** 
函数名称：vRegisterNewAccount 
简要描述：新用户注册函数  

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/ 
void vRegisterNewAccount()									//注册
{
	int accountTemp;	
	char tempName[30];
	char tempPassword[30];
	MsgData msg;
	msg.m_cmd = SER_CMD_REGISTER;
	
	accountTemp = iGetRand();							//生成账号
	sprintf(msg.m_account,"%d",accountTemp);		
	
	printf("若退出请，输入out\n");
	printf("输入你的昵称：");
	pcMyfgets(tempName, sizeof(tempName));
	if(strcmp(tempName,"out") == 0)
	{
		return ;
	}
	printf("输入你的密码：");
	pcMyfgets(tempPassword, sizeof(tempPassword));
	if(strcmp(tempPassword,"out") == 0)
	{
		return ;
	}
	strcpy(msg.m_name,tempName);
	strcpy(msg.m_pass,tempPassword);	
	
	if(strcmp(msg.m_name,"root") == 0)					//注册为群主
	{
		msg.m_root = 1;
		strcpy(msg.m_account,"111");
	}
	else
	{
		msg.m_root = 0;
	}
	send(iClientSocket,&msg,sizeof(MsgData),0);			//发送注册信息给服务端;	
	if(recv(iClientSocket,&msg,sizeof(MsgData),0) <= 0)	//每次都判断
	{
		printf("服务器断开链接\n");
		exit(-1);
	}		
	if(msg.m_flag == 1)
	{
		printf("注册成功\n");
		printf("\n您的登入账号为：%s 请务必牢记\n",msg.m_account);
	}
	else if(msg.m_flag == 0)
	{
		printf("服务器错误注册失败\n");
	}
	else if(msg.m_flag == 3)
	{
		printf("昵称重复请重新注册\n");
	}
} 

/****************************************************************************** 
函数名称：vCheckOnlineUsr 
简要描述：查询在线用户  

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vCheckOnlineUsr()									//查看在群聊中的人员
{
	MsgData msg;
	msg.m_cmd = SER_CMD_CHK_USR;
	printf("在线名单如下\n");
	send(iClientSocket,&msg,sizeof(MsgData),0);	
}

/****************************************************************************** 
函数名称：vChatToOne 
简要描述：私聊 

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vChatToOne()					
{
	MsgData msg;	
	char tempMess[1024];
	char oneName[30];
	
	vCheckOnlineUsr();				//查询在线用户
	while(1)
	{
		isChatOneOnline = 0;		
		msg.m_cmd = SER_CMD_STATE;
		printf("输入你想要私聊的人：(back退出私聊返群聊)\n");
		pcMyfgets(oneName, sizeof(oneName));
		if(strcmp(oneName,"back") == 0)
		{
			printf("正在返回群聊........\n");		
			return;
		}
		strcpy(msg.m_toName,oneName);
		strcpy(msg.m_fromName,cUsrName);
		if(strcmp(msg.m_toName, msg.m_fromName) == 0)
		{
			printf("请勿和自己私聊！！\n");
			continue;
		}
		printf("检验中请等待........\n");
		send(iClientSocket,&msg,sizeof(MsgData),0);		//向客户端发送查询在线用户请求
		sleep(1);
		
		if(isChatOneOnline == 1)						
		{
			vCheckChatRecord(0, oneName);
			printf("-------------以上为历史消息-------------\n");
			printf("在线中没有找到此人,将发送离线消息\n");
			break;
		}
		else
		{
			vCheckChatRecord(0, oneName);
			printf("-------------以上为历史消息-------------\n");
			printf("您可以开始私聊了\n");
			printf("输入back返回群聊\n");
			break;		
		}
		memset(oneName,0,strlen(oneName));
	}
	
	msg.m_cmd = SER_CMD_PERSONC;						//进入私聊模式
	while(1)
	{
		pcMyfgets(tempMess, 100);
		strcpy(msg.m_mess,tempMess);
		strcpy(msg.m_time,pcGetTime());
		if(strcmp(tempMess,"back") == 0)
		{
			printf("正在返回群聊........\n");		
			return;
		}
		else
		{
			if(isChatOneOnline == 1)
			{
				printf("对方已不在群聊，对方将在下次上线时收到您的离线消息\n");
				//break;
			}
			printf("你对 %s 说：%s\n",msg.m_toName,msg.m_mess);		
			send(iClientSocket,&msg,sizeof(MsgData),0);
			vSaveChatRecord(&msg, 2);
		}
		memset(tempMess,0,sizeof(tempMess));
	}
}

/****************************************************************************** 
函数名称：vCheckChatRecord 
简要描述：查看本地聊天记录函数 

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vCheckChatRecord(int _flag, char * _fromName)								
{
	FILE *fp = fopen("localChat.txt","a+");
	ChatRecord temp;
	int ret = fread(&temp,sizeof(ChatRecord),1,fp);			//1为群聊的聊天记录
	while(ret > 0)
	{
		if(_flag == 1 && temp.m_flag == 1)
		{
			printf("%s",temp.m_time);
			printf("%s",temp.m_fromName);
			printf(" 对%s",temp.m_toName);
			printf("说：\t%s\n",temp.m_mess);	
		}
		else if(_flag == 0 && temp.m_flag == 0)
		{
			//if(((strcmp(_fromName, temp.m_fromName) == 0) && (strcmp(temp.m_toName, "you") == 0))  || (strcmp(temp.m_fromName, cUsrName) == 0))
			if((strcmp(temp.m_fromName, _fromName) == 0) || (strcmp(temp.m_toName, _fromName) == 0))
			{
				printf("%s",temp.m_time);
				printf("%s",temp.m_fromName);
				printf(" 对%s",temp.m_toName);
				printf("说：\t%s\n",temp.m_mess);
			}
		}
		ret = fread(&temp,sizeof(ChatRecord),1,fp);
	}
	fclose(fp);
}

/****************************************************************************** 
函数名称：vSendFile 
简要描述：发送文件函数

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vSendFile()
{
	char buf[1024] = {0};
	char tempName[30];	//发送的用户
	char txt[30];		//发送的文件名
	MsgData msg;
	msg.m_cmd = SER_CMD_FILE_SEND;

	printf("请输入要发送的对象昵称\n");
	pcMyfgets(tempName, sizeof(tempName));
	strcpy(msg.m_toName,tempName);
	printf("输入要传的文件名(加后缀)\n");
	pcMyfgets(txt, sizeof(txt));
	strcpy(msg.m_fileName,txt);
	sprintf(buf,"%s",txt);
	int pd = open(buf, O_RDONLY|O_CREAT);
    if (pd == -1)
    {
        perror ("error");
        return;
    }	
	int ret = 0;	
    memset(buf,0,sizeof(buf));	
    ret = read(pd, buf, 1024);
    buf[ret] = '\0';				//结束
    strcpy(msg.m_mess,buf);	
	strcpy(msg.m_fromName,cUsrName);
	send(iClientSocket,&msg,sizeof(MsgData),0);	
	close(pd);	
}

/****************************************************************************** 
函数名称：vThirdChatMenu 
简要描述：打印群聊菜单

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vThirdChatMenu()  //群聊手册
{
	system("clear");
	printf("********************************************\n");
	printf("\t\t 群聊菜单\n");
	printf("q2：进入私聊模式\n");
	printf("q3：查看在线人员\n");
	printf("q4：查看本地聊天记录\n");
	printf("q6：发送文件\n");
	printf("输入bye退出群聊返回主界面\n");
	printf("********************************************\n");
}

/****************************************************************************** 
函数名称：vChatToAll 
简要描述：群聊功能函数

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vChatToAll()				//群聊功能
{
	MsgData msg;	
	char tempMess[1024];	
	msg.m_cmd = SER_CMD_GOURPC;
	msg.m_flag = 1;
	
	strcpy(msg.m_name,cUsrName);
	system("clear");
	printf("群聊\n");
	sprintf(msg.m_mess,"进入了群聊");	
	send(iClientSocket,&msg,sizeof(MsgData),0);	
	memset(msg.m_mess,0,strlen(msg.m_mess));
	printf("输入help显示菜单\n");
	while(1)
	{
		msg.m_flag = 0;
		pcMyfgets(tempMess, sizeof(tempMess));		
		strcpy(msg.m_mess,tempMess);		
		strcpy(msg.m_time,pcGetTime());	
		if (strcmp(msg.m_mess,"bye") == 0)					//bye退出群聊操作；
		{
			memset(tempMess,0,sizeof(tempMess));		
			sprintf(msg.m_mess,"退出了群聊\n");
			send(iClientSocket,&msg,sizeof(MsgData),0);		//将退出的消息发给公频
			memset(msg.m_mess,0,strlen(msg.m_mess));	
			strcpy(msg.m_mess,"bye");						//bye退出在公频的在线人数；
			send(iClientSocket,&msg,sizeof(MsgData),0);
			break;
		}
		else if(strcmp(msg.m_mess,"q2") == 0)				//q2进入私聊模式；
		{
			vChatToOne();	
			//sleep(1);
			printf("您已返回群聊\n");			
		}
		else if(strcmp(msg.m_mess,"q3") == 0)				//查看在线人员
		{
			vCheckOnlineUsr();
			//sleep(1);
			printf("\n您已返回群聊\n");	
		}
		else if(strcmp(msg.m_mess,"q4") == 0)				//查看聊天记录
		{
			vCheckChatRecord(1, "All");								//改为文件
			//sleep(1);
			printf("\n您已返回群聊\n");	
		}
		else if(strcmp(msg.m_mess,"q6") == 0)			//发送文件
		{
			vSendFile();
			//sleep(1);
			printf("您已返回群聊\n");
		}
		else if(strcmp(msg.m_mess,"help") == 0)	
		{
			vThirdChatMenu();							//手册
			//sleep(1);
			printf("您已返回群聊\n");
		}			
		else
		{
			send(iClientSocket,&msg,sizeof(MsgData),0);
			//sleep(1);
		}
		memset(msg.m_mess,0,strlen(msg.m_mess));	
		memset(msg.m_time,0,strlen(msg.m_time));	
		memset(tempMess,0,sizeof(tempMess));	
	}	
}

/****************************************************************************** 
函数名称：iUserLogin 
简要描述：用户登录函数

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
int iUserLogin()												//登入
{
	MsgData msg;	
	msg.m_cmd = SER_CMD_LOGIN;	
	char tempAccount[30];
	char tempPassword[30];	
	printf("输入你的账号：");

	pcMyfgets(tempAccount, sizeof(tempAccount));
	printf("输入你的密码：");
	pcMyfgets(tempPassword, sizeof(tempPassword));
	strcpy(msg.m_account,tempAccount);
	strcpy(msg.m_pass,tempPassword);	
	send(iClientSocket,&msg,sizeof(MsgData),0);	
	if(recv(iClientSocket,&msg,sizeof(MsgData),0) <= 0)//每次都判断
	{
		printf("服务器断开链接\n");
		exit(-1);
	}
	//sleep(1);
	if(msg.m_flag == 1)							//判断服务器回执
	{
		printf("\n");
		printf("欢迎你%s\n",msg.m_name);
		printf("\n");
		strcpy(cUsrName,msg.m_name);	
		strcpy(cUsrAccount,msg.m_account);
	}
	else if(msg.m_flag == 2)
	{
		printf("该账号已登入\n");
		return 0;
	}
	else if(msg.m_flag == 3)
	{
		printf("密码错误\n");
		return 0;
	}
	else if(msg.m_flag == 5)
	{
		printf("无此用户\n");
		return 0;
	}
	else
	{
		printf("系统错误,请重试\n");
		return 0;
	}
	return 1;
}

/****************************************************************************** 
函数名称：vSecondMenuAction 
简要描述：登录后的二级菜单

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vSecondMenuAction()
{
	char getWork[5];
	int actions;
	int flag = 0;
	while(1)
	{	
		//sleep(1);
		printf("\t****************\n");
		printf("\t1.进入群聊\n");
		printf("\t2.离线消息接收\n");
		printf("\t3.退出到登录界面\n");
		printf("\t****************\n");
		
		pcMyfgets(getWork, sizeof(getWork));
		actions = atoi(getWork);
		
		switch(actions)
		{
			case 1:printf("actions = %d\n",actions);vCheckChatRecord(1, getWork);vChatToAll();break;
			case 2:vCheckOfflineRecord();break;
			case 3:vSendLogoutMsg();flag = 1;break;		
		}
		
		if(flag == 1)
		{		
			bossFlag = 1;
			MsgData msg;
			msg.m_cmd = SER_CMD_RETURN;
			send(iClientSocket,&msg,sizeof(MsgData),0);
			break;
		}
	}
}


void vReadOfflineRecord(MsgData *_msg)
{
	printf("%s",_msg->m_time);
	printf("%s",_msg->m_fromName);
	printf("对你说：\t%s\n",_msg->m_mess);	
}


/****************************************************************************** 
函数名称：vCheckOfflineRecord
简要描述：发送查看离线聊天记录命令

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vCheckOfflineRecord()			
{
	MsgData msg;	
	msg.m_cmd = SER_CMD_CHK_OFFLINEMSG;
	strcpy(msg.m_name,cUsrName);	
	send(iClientSocket,&msg,sizeof(MsgData),0);	
}


/****************************************************************************** 
函数名称：vSendLogoutMsg 
简要描述：发送离线提醒

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vSendLogoutMsg()			
{
	MsgData msg;	
	msg.m_cmd = SER_CMD_LOGOUT;	
	strcpy(msg.m_name,cUsrName);	
	send(iClientSocket,&msg,sizeof(MsgData),0);	
}

/****************************************************************************** 
函数名称：vFirstMenuActions 
简要描述：一级菜单 包括注册、登录（创建线程）的操作

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vFirstMenuActions()
{
	char getWork[5];// 
	int actions;
	while(1)
	{	
		vFirstMenu();
		printf("输入操作:");
		pcMyfgets(getWork, sizeof(getWork));
		actions = atoi(getWork);
		switch(actions)
		{
			case 1://注册
					vRegisterNewAccount();				
				break;					
			case 2://登入
					if(iUserLogin() == 1)					
					{
						bossFlag = 0;
						pthread_t pId;				//启用线程
						pthread_create(&pId, NULL, (void*)recvThread, &iClientSocket);
						pthread_detach(pId);
						vSecondMenuAction();					
					}
				break;	
			case 3:
					vSendLogoutMsg();
					exit(0);							//退出
				break;
		}
		memset(getWork,0,strlen(getWork));
	}
}


/****************************************************************************** 
函数名称：pcMyfgets 
简要描述：去掉fgets读取的"/n",替换为"/0"

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
char *pcMyfgets(char *data, int n)
{
	fgets(data, n, stdin);
	char *find = strchr(data, '\n');
	if(find)
	{
		*find = '\0';
	}
	return data;
}

/****************************************************************************** 
函数名称：iSocketInit 
简要描述：去掉fgets读取的"/n",替换为"/0"

输入：
输出：	  
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
int iSocketInit()
{
	int clientsocket;
	if ((clientsocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror ("socket error");
        return -1;
    }
	struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family  = AF_INET;     // 设置地址族
    addr.sin_port    = htons(PORT); // 端口号转换为网络字节序
    inet_aton(IP,&(addr.sin_addr));
	
	if (connect(clientsocket, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror ("connect errer");
        return -1;
    }
	printf("连接服务器成功\n");
	return clientsocket;
}




int main(int argc,char *argv[])
{
	if(argc != 2)
	{
		printf("请加上IP地址\n");
		exit(-1);
	}
	else
	{
		printf("IP = %s\n",argv[1]);
		strcpy(IP,argv[1]);
		printf("IP =  %s\n",IP);
	}

	iClientSocket = iSocketInit();
	vFirstMenuActions();
    close(iClientSocket);
    return 0;	
}


