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

//Socket数据包
typedef struct Data	
{
	int work;					//cli工作指令
	int flag;					//回执flag
	char mess[1024];			//消息
	char account[30];			//用户登入账号
	char name[30];				//用户昵称
	char pass[30];				//用户密码
	char online[30];			//在线情况
	char time[30];	
	int root;					//root权限？	
	char toName[30];			
	char fromName[30];
	char fileName[30];

}MsgData;

typedef struct Mylink				//聊用聊表
{
	char name[30];
	int copyClientSocket;			//拷贝的cli套接字
	int isInChat;				//在群聊否  1在0不在,是on chat??

	struct Mylink *next;
}MyLink;

void linkDeleteNode(int clientSocket,MsgData *msg);
void registerNewAccount(int clientSocket, MsgData *msg);
void enterAccount(int clientSocket,MsgData *msg);
void chatAll(int clientSocket,MsgData *msg);
void getTheManState(int clientSocket,MsgData *msg);
void chatOne(int serverSocket, MsgData *msg);
void lookPeopleInChat(int clientSocket,MsgData *msg);
void sendFile(int clientSocket,MsgData *msg);
void linkOffline(int clientSocket);
void serviceThread(void* _clientSocket);

#define SER_REGISTER	1
#define SER_LOGIN		2
#define SER_GOURPC		3
#define SER_PERSONC		5
#define SER_STATE		4
#define SER_CHK_USR		6
#define SER_FILE_SEND	13
#define SER_LOGOUT		15
#define SER_RETURN		16




char IP[15];					
short PORT;
MyLink *pH;//在线链表

sqlite3 *db = NULL;//数据库
char *errmsg = NULL;//错误集
char **result = NULL;//查询结果

void serviceThread(void* _clientSocket)
{
	int clientSocket = *(int*) _clientSocket;
	MsgData msg;
    printf("pthread = %d\n",clientSocket);
    while(1)
	{
        if (recv(clientSocket,&msg,sizeof(MsgData),0) <= 0)
		{												
			printf("用户%d: %s已退出\n",clientSocket,msg.name);
			linkDeleteNode(clientSocket,&msg);					//删除链表中节点
			//通知群聊里用户此人下线
			break;
        }
		switch(msg.work)
		{
			case SER_REGISTER:registerNewAccount(clientSocket,&msg);break;		//注册插入数据库1
			case SER_LOGIN:enterAccount(clientSocket,&msg);break;		//登入验证2
			case SER_GOURPC:chatAll(clientSocket,&msg);break;		//进入群聊3
			case SER_STATE:getTheManState(clientSocket,&msg);break;		//检验对象的状态，在线或者是否接受私聊
			case SER_PERSONC:chatOne(clientSocket,&msg);break;					//进入私聊5
			case SER_CHK_USR:lookPeopleInChat(clientSocket,&msg);break;	//查看在线用户
			case SER_FILE_SEND:sendFile(clientSocket,&msg);break;	//文件传输
			case SER_LOGOUT:linkOffline(clientSocket);break;		//下线
			case SER_RETURN:send(clientSocket,&msg,sizeof(MsgData),0);break;//返回主程序
		}
    }
	close(clientSocket);
}

void linkDeleteNode(int clientSocket,MsgData *msg)		//用户从在线链表中删除
{
	MyLink *p = pH->next;   							//下一个节点
	MyLink *pR = pH;									//当前节点
	while(p != NULL)
	{
		if(p->copyClientSocket == clientSocket)			//查询下线用户的节点
		{
			pR->next = p->next;
			free(p);
			return;
		}
		pR = pR->next;									//重新连接链表
		p = pR->next;
	}
}

void createTable()
{
	printf("createTable\n");
	char *sql = "create table if not exists info(id integer primary key,account text,name text,password text,root text);";  
	printf("sql = %s\n",sql);
	if(SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, &errmsg))//判断是否成功成功返回SQLITE_OK  
    {  
        printf("失败原因:%s\n",errmsg);  
        exit(-1);  
    }
}

void displayAccount()//数据库校验display
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


//注册信息写入数据库
void registerNewAccount(int clientSocket, MsgData *msg)
{
	char buf[200];									//数据库命令数组
	int nrow = 0;									//行
	int ncol = 0;									//列
	char tempName[30];

	
	if(sqlite3_open("test.db", &db))
	{
		printf("数据库读取失败!\n");
		exit(-1);
	}
	else
	{
		printf("数据库读取成功!\n");
	}
	createTable();			
	
	//检查昵称重复注册
	sprintf(buf,"select name from info where name = '%s'",msg->name);
	if(SQLITE_OK != sqlite3_get_table(db, buf, &result, &nrow, &ncol, &errmsg))
	{
		printf("view error%s\n",errmsg);
		return;
	}
	memset(buf,0,sizeof(buf));							//用完初始化

	if(ncol == 0)								//没查到结果表示无此账号，可以注册
	{
		sqlite3_free_table(result);				//写入数据库
		sprintf(buf, "insert into info(account,name,password,root) values ('%s','%s','%s','%d');", msg->account,msg->name,msg->pass,msg->root);
		int ret = sqlite3_exec(db, buf, NULL, NULL, &errmsg);
		if (ret != SQLITE_OK)
		{
			printf("错误原因%s\n",errmsg);
			msg->flag = 0;
		}
		else
		{
			msg->flag = 1;		//成功	
			displayAccount();	//终端打印数据库
		}
	}
	else
	{
		printf("ncol = %d\n",ncol);
		printf("result[ncol] = %s\n",result[ncol]);
		strcpy(tempName,result[ncol]);
		sqlite3_free_table(result);
		if(strcmp(result[ncol],msg->name) == 0)
		{
			msg->flag = 3;//重名了	
		}	
	}
	send(clientSocket,msg,sizeof(MsgData),0);
	sqlite3_close(db);
}


int checkOnlineAccount(MsgData *msg)//重复登入检验
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
	sprintf(buf,"select name from info where account = '%s'",msg->account);
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
		if(strcmp(p->name,tempName) == 0)//检查链表,只要登入了就在链表中
		{
			return 0;	//已在线    	//修改昵称的话，链表中的name也要修改
		}
	}
	sqlite3_close(db);
	return 1;							//此人不在线
	
}

int checkPassword(int clientSocket,MsgData *msg)//检验密码
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
	sprintf(buf,"select password from info where account = '%s'",msg->account);
	if(SQLITE_OK != sqlite3_get_table(db, buf, &result, &nrow, &ncol, &errmsg))
	{
		printf("读取数据库失败\n");
		sqlite3_close(db);	
		return -1;
	}
	strcpy(tempPassword,result[ncol]);
	sqlite3_free_table(result); 
	memset(buf,0,sizeof(buf));
	if(strcmp(msg->pass,tempPassword) == 0)
	{
		return 1;		//正确返回1
	}
	else
	{
		return 0;
	}
	//sqlite3_close(db);	
}



int linkInsertOnlinePeople(int clientSocket,MsgData *msg)//链表插入人员
{
	MyLink *p = pH;
	MyLink *pNew = (MyLink *)malloc(sizeof(MyLink));
	if(pNew == NULL)
	{
		printf("malloc ERRER！\n");
		return -1;
	}
	strcpy(pNew->name,msg->name);
	pNew->copyClientSocket = clientSocket;
	pNew->isInChat = 0;
	pNew->next = p->next;
	p->next = pNew;
	return 1;
}


void enterAccount(int clientSocket,MsgData *msg)//登入
{
	char buf[200];
	int nrow = 0;  
    int ncol = 0;

	if(sqlite3_open("test.db", &db))
	{
		printf("数据库开启失败\n");
		msg->flag = -1;
		send(clientSocket,msg,sizeof(MsgData),0);
		return;
	}
	int mFlag = checkOnlineAccount(msg);//重复在线查询
	if(mFlag == 0)//此人已在线
	{
		msg->flag = 2;//已登入
	}
	else if(mFlag == -1)//意外情况
	{
		msg->flag = -1;
	}
	else if(mFlag == 3)//此用户不存在
	{
		msg->flag = 5;
	}
	else if(mFlag == 1)//此人不在线
	{
		if(checkPassword(clientSocket,msg) == 1)//密码检验函数 == 1 密码正确
		{
			sprintf(buf,"select name from info where account = '%s'",msg->account);//根据账号将昵称找出来
			if(SQLITE_OK != sqlite3_get_table(db, buf, &result, &nrow, &ncol, &errmsg))
			{
				printf("数据库读取失败\n");
				msg->flag = -1;					//意外情况		 = -1,cli要printf说发生意外
			}
			else
			{
				strcpy(msg->name,result[ncol]);	
				sqlite3_free_table(result);
				if(linkInsertOnlinePeople(clientSocket,msg) == -1)//插入在线链表
				{
					//增加malloc失败的提示
					msg->flag = -1;
				}
				else
				{
					printf("登入成功\n");	
					msg->flag = 1;					//正确flag	
				}
			}
		}
		else									
		{
			msg->flag = 3;//密码错误
		}
	}	
	send(clientSocket,msg,sizeof(MsgData),0); 
	sqlite3_close(db);	
}


void offLink(int ClientSocket,MsgData *msg)//退出群聊的处理
{
	MyLink *p = pH->next;
	while(p != NULL)
	{
		if(p->copyClientSocket == ClientSocket)
		{
			p->isInChat = 0;//仅退出群聊，任在链表中就是在线
		}
		//是否加入没找到此套接字的处理？
		p = p->next;
	}
}



void chatAll(int clientSocket,MsgData *msg)  			//群聊操作	
{
	MyLink *p = pH->next;
	MyLink *pP = pH->next;

	
	if(strcmp(msg->mess,"bye") == 0)
	{	
		offLink(clientSocket,msg);
		return;
	}
	else
	{
		while(pP != NULL)
		{
			if(pP->copyClientSocket == clientSocket)
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
			send(p->copyClientSocket,msg,sizeof(MsgData),0);//发送给所以在群聊的人;
			//printf("在聊天室的人 %s\n",p->name);
		}
		p = p->next;
	}
	//printf("\n");
}



void getTheManState(int clientSocket,MsgData *msg)//私聊检验
{
	MyLink *p = pH;
	int flag = 0;	
	while(p->next != NULL)	//查询用户是否进入群聊
	{
		p = p->next;
		printf("checkName = %s\n",p->name);
		if((strcmp(p->name,msg->toName) == 0) && p->isInChat == 1)
		{
			flag = 1;
			break;
		}		
	}
	if(flag == 0)
	{
		msg->work = 4;				//告诉客户端希望私聊的用户已下线
		send(clientSocket,msg,sizeof(MsgData),0);
	}
	/************
	************/
}


void linkDisplayInchatPeople()//打印在群聊中的人
{
	int num = 0;
	MyLink *p = pH->next;
	while(p != NULL)
	{
		if(p->isInChat == 1)
		{
			num++;
			printf("：%s\n",p->name);
		}
		p = p->next;
	}
}



//对方如果私聊时下线，需要通知并关闭私聊返回群聊
void chatOne(int serverSocket, MsgData *msg)//私聊
{
	MyLink *p = pH;
	int flag = 0;
	printf("聊天室中用户如下：\n");
	linkDisplayInchatPeople();
	
	while(p->next != NULL)		//检测用户是否在线
	{
		p = p->next;
		if((strcmp(p->name,msg->toName) == 0) && p->isInChat == 1)
		{
			flag = 1;			//不在线
			send(p->copyClientSocket,msg,sizeof(MsgData),0);
			break;
		}	
	}
	if(flag == 0)				//在线
	{
		msg->work = 4;			
		send(serverSocket,msg,sizeof(MsgData),0);
	}
}


void lookPeopleInChat(int clientSocket,MsgData *msg)//用户查看在群聊中的用户
{
	MyLink *p = pH;
	while(p->next != NULL)
	{
		p = p->next;
		if(p->isInChat == 1)
		{
			strcpy(msg->name,p->name);
			send(clientSocket,msg,sizeof(MsgData),0);
		}
	}
}


void sendFile(int clientSocket,MsgData *msg)
{
	int flag = 0;	
	char buf[1024] = {0};
    strcpy(buf, msg->mess);	
	MyLink *p = pH;
	while(p->next != NULL)				//找toname是否在群聊；
	{
		p = p->next;
		if((strcmp(p->name,msg->toName) == 0) && (p->isInChat == 1))
		{
			flag = 1;
			msg->work = 15;
			send(p->copyClientSocket,msg,sizeof(MsgData),0);	
		}
	}
	if(flag == 1)
	{
		msg->work = 12;
		send(clientSocket,msg,sizeof(MsgData),0);	
	}
	else//不在群聊不能发送文件
	{
		msg->work = 13;
		send(clientSocket,msg,sizeof(MsgData),0);
	}
}


void linkOffline(int clientSocket)
{
	int flog = 0;
	MyLink *p = pH;
	MyLink *pR = NULL;
	while(p->next!=NULL)
	{
		pR = p;
		p = p->next;
		if(p->copyClientSocket == clientSocket)
		{
			pR->next = p->next;
			printf("%s已下线！\n",p->name);
			flog = 1;
			free(p);
		}
	}
	if(flog == 0)//不会发生
	{
		printf("查无此人\n");
	}
}

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

int initSocket()
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


int myAccept(int serverSocket)//返回链接上的套接字
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
	int serverSocket = initSocket();				//初始化socket
	while(1)
	{		
		int clientSocket = myAccept(serverSocket);	//接收cli
		pthread_t id;							//创建线程
        pthread_create(&id, NULL, (void*)serviceThread, &clientSocket);
        pthread_detach(id); 						//待线程结束后回收其资源
	}	
	close(serverSocket);	
	return 0;
}

