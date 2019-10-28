#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include "server.h"

sqlite3 *db = NULL;//数据库
char *errmsg = NULL;//错误集
char **result = NULL;//查询结果


/****************************************************************************** 
函数名称：vServiceThread 
简要描述：客户端接收消息线程  

输入：_ClientSocket  套接字描述符        
输出：    
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/ 
void vServiceThread(void* _clientSocket)
{
	int clientSocket = *(int*) _clientSocket;
	MsgData msg;
    printf("pthread = %d\n",clientSocket);
    while(1)
	{
        if (recv(clientSocket,&msg,sizeof(MsgData),0) <= 0)
		{												
			printf("用户%d: %s已退出\n",clientSocket,msg.m_name);
			vLinkDeleteNode(clientSocket,&msg);					//删除链表中节点
			//通知群聊里用户此人下线
			break;
        }
		switch(msg.m_cmd)
		{
			case SER_CMD_REGISTER:
								vRegisterNewAccount(clientSocket,&msg);
						break;		//注册插入数据库1
			case SER_CMD_LOGIN:
								vLoginAccount(clientSocket,&msg);
						break;		//登录验证2
			case SER_CMD_GOURPC:
								vChatToAll(clientSocket,&msg);
						break;		//进入群聊3
			case SER_CMD_STATE:
								vCheckInchatUser(clientSocket,&msg);
						break;		//检验对象的状态，在线或者是否接受私聊
			case SER_CMD_PERSONC:
								vChatToOne(clientSocket,&msg);
						break;		//进入私聊5
			case SER_CMD_CHK_USR:
								vClientCheckInChatUser(clientSocket,&msg);
						break;		//查看在线用户
			case SER_CMD_FILE_SEND:
								vSendFile(clientSocket,&msg);
						break;		//文件传输
			case SER_CMD_LOGOUT:
								vLinkOffline(clientSocket);
						break;		//下线
			case SER_CMD_CHK_OFFLINEMSG:
								vCheckOfflineRecord(clientSocket, &msg);
						break;		//下线
			case SER_CMD_RETURN:
								send(clientSocket,&msg,sizeof(MsgData),0);
						break;		//返回主程序
		}
    }
	close(clientSocket);
}

/****************************************************************************** 
函数名称：vLinkDeleteNode 
简要描述：将用户从服务器在线用户链表中删除

输入：_ClientSocket  套接字描述符
	  _msg	接收到的数据包
输出：    
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/ 
void vLinkDeleteNode(int _clientSocket,MsgData *_msg)		
{
	MyLink *p = pH->next;   							//下一个节点
	MyLink *pR = pH;									//当前节点
	while(p != NULL)
	{
		if(p->iCopyClientSocket == _clientSocket)			//查询下线用户的节点
		{
			pR->next = p->next;
			free(p);
			return;
		}
		pR = pR->next;									//重新连接链表
		p = pR->next;
	}
}

/****************************************************************************** 
函数名称：vSqlCreateTable 
简要描述：数据库Table创建  

输入：
输出：    
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/ 
void vSqlCreateTable()
{
	printf("vSqlCreateTable\n");
	char *sql = "create table if not exists info(id integer primary key,account text,name text,password text,root text);";  
	printf("sql = %s\n",sql);
	if(SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, &errmsg))//判断是否成功成功返回SQLITE_OK  
    {  
        printf("失败原因:%s\n",errmsg);  
        exit(-1);  
    }
}

/****************************************************************************** 
函数名称：vSqlDisplayAccount 
简要描述：打印数据库内容，校验数据库是否创建成功

输入：
输出：    
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vSqlDisplayAccount()
{  
    int i = 0;  
    int nrow = 0;  
    int ncol = 0;  
	
	if(sqlite3_open("test.db", &db))
	{
		printf("数据库读取失败!\n");
	}
	else
	{
		printf("数据库读取成功!\n");
	}	
    char *sql = "select * from info;";   
    if(SQLITE_OK != sqlite3_get_table(db, sql, &result, &nrow, &ncol, &errmsg))//判断sqlite3_get_table是否运用成功，成功返回SQLITE_OK  
    {  
        printf("fail:%s\n",errmsg);  
        printf("\n");  
        exit(-1);  
    }  
    for(i = 0; i < (nrow + 1) * ncol; i++)//将表中的数据打印出来  
    {  
        printf("%-12s",result[i]);  
        if((i + 1) % ncol == 0)  
        {  
            printf("\n");  
        }  
    }
    sqlite3_free_table(result);//释放result  
	sqlite3_close(db);
}

/****************************************************************************** 
函数名称：vRegisterNewAccount 
简要描述：将客户端发来的用户注册信息保存到数据库中

输入：_ClientSocket  套接字描述符
	  _msg	接收到的数据包
输出：    
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vRegisterNewAccount(int _clientSocket, MsgData *_msg)
{
	char buf[200];									//数据库命令数组
	int iSqlRow = 0;								//行
	int iSqlCol = 0;								//列
	char cTempName[30];

	
	if(sqlite3_open("test.db", &db))
	{
		printf("数据库读取失败!\n");
		exit(-1);
	}
	else
	{
		printf("数据库读取成功!\n");
	}
	vSqlCreateTable();			
	
	//检查昵称重复注册
	sprintf(buf,"select name from info where name = '%s'",_msg->m_name);
	if(SQLITE_OK != sqlite3_get_table(db, buf, &result, &iSqlRow, &iSqlCol, &errmsg))
	{
		printf("view error%s\n",errmsg);
		return;
	}
	memset(buf,0,sizeof(buf));							//用完初始化

	if(iSqlCol == 0)								//没查到结果表示无此账号，可以注册
	{
		sqlite3_free_table(result);				//写入数据库
		sprintf(buf, "insert into info(account,name,password,root) values ('%s','%s','%s','%d');", _msg->m_account,_msg->m_name,_msg->m_pass,_msg->m_root);
		int ret = sqlite3_exec(db, buf, NULL, NULL, &errmsg);
		if (ret != SQLITE_OK)
		{
			printf("错误原因%s\n",errmsg);
			_msg->m_flag = 0;
		}
		else
		{
			_msg->m_flag = 1;		//成功	
			vSqlDisplayAccount();	//终端打印数据库
		}
	}
	else
	{
		printf("iSqlCol = %d\n",iSqlCol);
		printf("result[iSqlCol] = %s\n",result[iSqlCol]);
		strcpy(cTempName,result[iSqlCol]);
		sqlite3_free_table(result);
		if(strcmp(result[iSqlCol],_msg->m_name) == 0)
		{
			_msg->m_flag = 3;//重名了	
		}	
	}
	send(_clientSocket,_msg,sizeof(MsgData),0);
	sqlite3_close(db);
}

/****************************************************************************** 
函数名称：iCheckAccountState 
简要描述：检测账户的登录状态，查看账户是否注册在数据库中

输入： _msg	接收需要检测的账户
输出：    
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
int iCheckAccountState(MsgData *_msg)
{	
	char buf[200];
	char tempName[30];
	int nrow = 0;  
    int ncol = 0;

	if(sqlite3_open("test.db", &db))
	{
		printf("数据库开启失败!\n");
		return -1;
	}
	//根据账号得到昵称
	sprintf(buf,"select name from info where account = '%s'",_msg->m_account);
	if(SQLITE_OK != sqlite3_get_table( db, buf , &result, &nrow, &ncol, &errmsg))
	{
		printf("数据库读取错误！\n");
		sqlite3_close(db);
		return -1;
	}
	if(ncol == 0)
	{	
		printf("无此用户\n");
		return 3;
	}
	strcpy(tempName,result[ncol]);
	sqlite3_free_table(result); 
	MyLink *p = pH;		//表头
	//由昵称在在线链表中查询是否在线
	while(p->next != NULL)
	{
		p = p->next;
		if(strcmp(p->m_name,tempName) == 0)//检查链表,只要登录了就在链表中
		{
			return 0;	//已在线    	//修改昵称的话，链表中的name也要修改
		}
	}
	sqlite3_close(db);
	return 1;							//此人不在线
	
}

/****************************************************************************** 
函数名称：iCheckPassword 
简要描述：检测账户密码是否正确

输入： _msg	接收需要检测的账户密码
输出： 正确返回1  失败返回0
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
int iCheckPassword(int _clientSocket,MsgData *_msg)//检验密码
{
	char buf[200];
	char tempPassword[30];
	int nrow = 0;  
    int ncol = 0;
	
	//打开数据库
	if(sqlite3_open("test.db", &db))
	{
		printf("开启数据库失败！\n");
		return -1;
	}

	//密码检查
	sprintf(buf,"select password from info where account = '%s'",_msg->m_account);
	if(SQLITE_OK != sqlite3_get_table(db, buf, &result, &nrow, &ncol, &errmsg))
	{
		printf("读取数据库失败\n");
		sqlite3_close(db);	
		return -1;
	}
	strcpy(tempPassword,result[ncol]);
	sqlite3_free_table(result); 
	memset(buf,0,sizeof(buf));
	if(strcmp(_msg->m_pass,tempPassword) == 0)
	{
		return 1;		//正确返回1
	}
	else
	{
		return 0;
	}
	//sqlite3_close(db);	
}

/****************************************************************************** 
函数名称：iLinkInsertOnlineUser 
简要描述：将新登录的用户插入在线链表中

输入： _msg	接收需要检测的账户密码
输出： 正确返回1  失败返回-1
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
int iLinkInsertOnlineUser(int _clientSocket,MsgData *_msg)//链表插入人员
{
	MyLink *p = pH;
	MyLink *pNew = (MyLink *)malloc(sizeof(MyLink));
	if(pNew == NULL)
	{
		printf("malloc ERRER！\n");
		return -1;
	}
	strcpy(pNew->m_name,_msg->m_name);
	pNew->iCopyClientSocket = _clientSocket;
	pNew->isInChat = 0;
	pNew->next = p->next;
	p->next = pNew;
	return 1;
}

/****************************************************************************** 
函数名称：vLoginAccount
简要描述：用户登录处理函数

输入： _msg	接收需要检测的账户密码
输出： 
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vLoginAccount(int _clientSocket,MsgData *_msg)//登录
{
	char buf[200];
	int nrow = 0;  
    int ncol = 0;

	if(sqlite3_open("test.db", &db))
	{
		printf("数据库开启失败\n");
		_msg->m_flag = -1;
		send(_clientSocket,_msg,sizeof(MsgData),0);
		return;
	}
	int mFlag = iCheckAccountState(_msg);//重复在线查询
	if(mFlag == 0)//此人已在线
	{
		_msg->m_flag = 2;//已登录
	}
	else if(mFlag == -1)//意外情况
	{
		_msg->m_flag = -1;
	}
	else if(mFlag == 3)//此用户不存在
	{
		_msg->m_flag = 5;
	}
	else if(mFlag == 1)//此人不在线
	{
		if(iCheckPassword(_clientSocket,_msg) == 1)//密码检验函数 == 1 密码正确
		{
			sprintf(buf,"select name from info where account = '%s'",_msg->m_account);//根据账号将昵称找出来
			if(SQLITE_OK != sqlite3_get_table(db, buf, &result, &nrow, &ncol, &errmsg))
			{
				printf("数据库读取失败\n");
				_msg->m_flag = -1;					//意外情况		 = -1,cli要printf说发生意外
			}
			else
			{
				strcpy(_msg->m_name,result[ncol]);	
				sqlite3_free_table(result);
				if(iLinkInsertOnlineUser(_clientSocket,_msg) == -1)//插入在线链表
				{
					//增加malloc失败的提示
					_msg->m_flag = -1;
				}
				else
				{
					printf("%s登录成功\n",_msg->m_name);	
					_msg->m_flag = 1;					//正确flag	
				}
			}
		}
		else									
		{
			_msg->m_flag = 3;//密码错误
		}
	}	
	send(_clientSocket,_msg,sizeof(MsgData),0); 
	sqlite3_close(db);	
}

/****************************************************************************** 
函数名称：vLinkQuitChat
简要描述：用户退出群聊处理 （仍在线）

输入： _msg	接收需要退出的账户
输出： 
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vLinkQuitChat(int _clientSocket,MsgData *_msg)//退出群聊的处理
{
	MyLink *p = pH->next;
	while(p != NULL)
	{
		if(p->iCopyClientSocket == _clientSocket)
		{
			p->isInChat = 0;//仅退出群聊，人在链表中就是在线
		}
		//是否加入没找到此套接字的处理？
		p = p->next;
	}
}

/****************************************************************************** 
函数名称：vChatToAll
简要描述：群聊处理函数

输入： _msg	接收需要私聊的账户
输出： 
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vChatToAll(int _clientSocket,MsgData *_msg)  			//群聊操作	
{
	MyLink *p = pH->next;
	MyLink *pP = pH->next;

	
	if(strcmp(_msg->m_mess,"bye") == 0)
	{	
		vLinkQuitChat(_clientSocket,_msg);
		return;
	}
	else
	{
		while(pP != NULL)
		{
			if(pP->iCopyClientSocket == _clientSocket)
			{
				pP->isInChat = 1;				//讲用户标记为进入群聊
			}
			pP = pP->next;
		}
	}

	//群聊操作
	while(p != NULL)
	{	
		if(p->isInChat == 1)
		{
			send(p->iCopyClientSocket,_msg,sizeof(MsgData),0);//发送给所以在群聊的人;
			//printf("在聊天室的人 %s\n",p->name);
		}
		p = p->next;
	}
	//printf("\n");
}

/****************************************************************************** 
函数名称：vCheckInchatUser
简要描述：检测用户是否在线

输入： _msg	接收需要检测的账户名称
输出： 
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vCheckInchatUser(int _clientSocket,MsgData *_msg)//私聊检验
{
	MyLink *p = pH;
	int flag = 0;	
	while(p->next != NULL)	//查询用户是否进入群聊
	{
		p = p->next;
		printf("查询的用户%s在线\n",p->m_name);
		if((strcmp(p->m_name,_msg->m_toName) == 0) && p->isInChat == 1)
		{
			flag = 1;
			break;
		}		
	}
	if(flag == 0)
	{
		_msg->m_cmd = CLI_CMD_USR_OFFLINE;				//告诉客户端希望私聊的用户已下线
		send(_clientSocket,_msg,sizeof(MsgData),0);
	}
}

/****************************************************************************** 
函数名称：vServerDisplayInchatUser
简要描述：打印在群聊中的人

输入： 
输出： 
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vServerDisplayInchatUser()//打印在群聊中的人
{
	int num = 0;
	MyLink *p = pH->next;
	while(p != NULL)
	{
		if(p->isInChat == 1)
		{
			num++;
			printf("：%s\n",p->m_name);
		}
		p = p->next;
	}
}


/****************************************************************************** 
函数名称：vChatToOne
简要描述：私聊操作（对方如果私聊时下线，需要通知并关闭私聊返回群聊）

输入： 
输出： 
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vChatToOne(int _serverSocket, MsgData *_msg)//私聊
{
	MyLink *p = pH;
	int flag = 0;
	//printf("聊天室中用户如下：\n");
	//vServerDisplayInchatUser();  //调试用
	
	while(p->next != NULL)		//检测用户是否在线
	{
		p = p->next;
		if((strcmp(p->m_name,_msg->m_toName) == 0) && p->isInChat == 1)
		{
			flag = 1;			//不在线
			send(p->iCopyClientSocket,_msg,sizeof(MsgData),0);
			break;
		}	
	}
	if(flag == 0)				//在线
	{
		_msg->m_cmd = CLI_CMD_USR_OFFLINE;			//用户下线
		vSaveOfflineRecord(_serverSocket, _msg);
		send(_serverSocket,_msg,sizeof(MsgData),0);
	}
}

typedef struct OfflineRecord
{
	char m_mess[1024];
	char m_fromName[30];
	char m_toName[30];
	char m_time[30];
	int  m_iClientSocket;
}OfflineRecord;

void vSaveOfflineRecord(int _serverSocket, MsgData * _msg)
{
	FILE *fp = fopen("OfflineRecord.txt","a+");
	OfflineRecord temp;
	
	temp.m_iClientSocket = _serverSocket;
	strcpy(temp.m_fromName,_msg->m_fromName);
	strcpy(temp.m_toName,_msg->m_toName);
	strcpy(temp.m_mess,_msg->m_mess);
	strcpy(temp.m_time,pcGetTime());
	fwrite(&temp,sizeof(OfflineRecord),1,fp);//将结构体直接写入文件

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


void vCheckOfflineRecord(int _serverSocket, MsgData *_msg)
{
	//MyLink *p = pH;
	MsgData msg;
	 
	FILE *fp = fopen("OfflineRecord.txt","r");
	OfflineRecord temp;
	int ret = fread(&temp,sizeof(OfflineRecord),1,fp);			
	while(ret > 0)
	{
		
		if(strcmp(_msg->m_name, temp.m_toName) == 0)
			{
				msg.m_cmd = CLI_CMD_SENF_OFFLINEMSG;
				strcpy(msg.m_toName, temp.m_toName);
				strcpy(msg.m_fromName, temp.m_fromName);
				strcpy(msg.m_mess, temp.m_mess);
				strcpy(msg.m_time, temp.m_time);
				send(_serverSocket, &msg, sizeof(MsgData), 0);
			}
		ret = fread(&temp,sizeof(OfflineRecord),1,fp);
	}
	fclose(fp);
}


/****************************************************************************** 
函数名称：vClientCheckInChatUser
简要描述：发送在线用户给客户端

输入： 
输出： 
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vClientCheckInChatUser(int clientSocket,MsgData *msg)
{
	MyLink *p = pH;
	while(p->next != NULL)
	{
		p = p->next;
		if(p->isInChat == 1)
		{
			strcpy(msg->m_name,p->m_name);
			send(clientSocket,msg,sizeof(MsgData),0);
		}
	}
}

/****************************************************************************** 
函数名称：vSendFile
简要描述：发送文件

输入： _msg 需要发送给的用户昵称
输出： 
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vSendFile(int clientSocket,MsgData *msg)
{
	int flag = 0;	
	char buf[1024] = {0};
    strcpy(buf, msg->m_mess);	
	MyLink *p = pH;
	while(p->next != NULL)				//找toname是否在群聊；
	{
		p = p->next;
		if((strcmp(p->m_name,msg->m_toName) == 0) && (p->isInChat == 1))
		{
			flag = 1;
			msg->m_cmd = CLI_CMD_SAVE_FILE;
			send(p->iCopyClientSocket,msg,sizeof(MsgData),0);	
		}
	}
	if(flag == 1)
	{
		msg->m_cmd = CLI_CMD_SEND_FILE;
		send(clientSocket,msg,sizeof(MsgData),0);	
	}
	else//不在群聊不能发送文件
	{
		msg->m_cmd = CLI_CMD_FILE_ERROR;
		send(clientSocket,msg,sizeof(MsgData),0);
	}
}

/****************************************************************************** 
函数名称：vLinkOffline
简要描述：将用户从服务器在线用户链表中删除（未完成）

输入： _msg 需要发送给的用户昵称
输出： 
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
void vLinkOffline(int clientSocket)
{
	//int flog = 0;
	MyLink *p = pH;
	MyLink *pR = NULL;
	while(p->next!=NULL)
	{
		pR = p;
		p = p->next;
		if(p->iCopyClientSocket == clientSocket)
		{
			pR->next = p->next;
			printf("%s已下线！\n",p->m_name);
			//flog = 1;
			free(p);
		}
	}
	/*
	if(flog == 0)
	{
		printf("查无此人\n");
	}*/
}

/****************************************************************************** 
函数名称：initLink
简要描述：链表初始化

输入：
输出： 
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
MyLink * initLink()
{
	MyLink *p = (MyLink *)malloc(sizeof(MyLink));
	if(p == NULL)
	{
		printf("ERRER\n");
		exit(-1);
	}
	p->next = NULL;
	return p;
}

/****************************************************************************** 
函数名称：iSocketInit
简要描述：Socket初始化

输入：
输出： 返回套接字描述符
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
int iSocketInit()
{
	PORT = 7777;
	int fdSocket = socket(AF_INET,SOCK_STREAM,0);
    if (fdSocket == -1)
	{
        perror("创建socket失败");
        exit(-1);
    }
    struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP);
	//addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    if (bind(fdSocket,(struct sockaddr *)&addr,sizeof(addr)) == -1)
	{
		printf("IP = %s\n",IP);
        perror("绑定失败");
        exit(-1);
    }
    if (listen(fdSocket,100) == -1)
	{
        perror("设置监听失败");
        exit(-1);
    }
	printf("等待客户端的连接..........\n");
	return fdSocket;
}

/****************************************************************************** 
函数名称：iSocketAccept
简要描述：Socket接收

输入： serverSocket服务器描述符
输出： 返回客户端套接字描述符
修改日志:2019-10-25 Tiandy 创建该函数 
******************************************************************************/
int iSocketAccept(int serverSocket)//返回链接上的套接字
{
	struct sockaddr_in client_addr;// 用来保存客户端的ip和端口信息
    socklen_t len;
    int clientSocket;
    if ((clientSocket = accept(serverSocket,(struct sockaddr *)&client_addr,  &len)) == -1)
    {
        perror ("accept");
    }
    printf ("成功接收一个客户端: %s\n", inet_ntoa(client_addr.sin_addr));
    return clientSocket;
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
		strcpy(IP,argv[1]);
		printf("IP =  %s\n",IP);		
	}
	pH = initLink();								//初始化链表
	int iServerSocket = iSocketInit();				//初始化socket
	while(1)
	{		
		int iClientSocket = iSocketAccept(iServerSocket);	//接收cli
		pthread_t id;							//创建线程
        pthread_create(&id, NULL, (void*)vServiceThread, &iClientSocket);
        pthread_detach(id); 						//待线程结束后回收其资源
	}	
	close(iServerSocket);	
	return 0;
}

