//CMD_RECV_FROM_CLIENT
#define SER_CMD_REGISTER	1
#define SER_CMD_LOGIN		2
#define SER_CMD_GOURPC		3
#define SER_CMD_PERSONC		5
#define SER_CMD_STATE		4
#define SER_CMD_CHK_USR		6
#define SER_CMD_FILE_SEND	13
#define SER_CMD_LOGOUT		15
#define SER_CMD_RETURN		16
#define SER_CMD_CHK_OFFLINEMSG	14


//CMD_SEND_TO_CLIENT
#define CLI_CMD_GROUPCHAT			3
#define CLI_CMD_USR_OFFLINE			4
#define CLI_CMD_PERSONCHAT			5
#define CLI_CMD_CHK_OL_USER			6
#define CLI_CMD_SEND_FILE			12
#define CLI_CMD_FILE_ERROR			13
#define CLI_CMD_SAVE_FILE			15
#define CLI_CMD_ERROR				-1
#define CLI_CMD_USEROFFLINE			21
#define CLI_CMD_SENF_OFFLINEMSG		7



//Socket数据包
typedef struct MsgData	
{
	int  m_cmd;					//cli工作指令
	int  m_flag;					//用于登录和注册函数的回执
	char m_mess[1024];			//消息
	char m_account[30];			//用户登入账号
	char m_name[30];				//用户昵称
	char m_pass[30];				//用户密码
	char m_online[30];			//在线情况
	char m_time[30];	
	int  m_root;					//root权限？	
	char m_toName[30];			
	char m_fromName[30];
	char m_fileName[30];

}MsgData;

typedef struct MyLink				//链表
{
	char m_name[30];
	int  iCopyClientSocket;		//拷贝每个client的套接字
	int  isInChat;				//在群聊否  1在0不在,是on chat??

	struct MyLink *next;
}MyLink;

void vServiceThread(void* _clientSocket);
void vLinkDeleteNode(int _clientSocket,MsgData *_msg);
void vSqlCreateTable();
void vSqlDisplayAccount();
void vRegisterNewAccount(int _clientSocket, MsgData *_msg);
int  iCheckAccountState(MsgData *_msg);
int  iCheckPassword(int _clientSocket,MsgData *_msg);
int  iLinkInsertOnlineUser(int _clientSocket,MsgData *_msg);
void vLoginAccount(int _clientSocket,MsgData *_msg);
void vLinkQuitChat(int _clientSocket,MsgData *_msg);
void vChatToAll(int _clientSocket,MsgData *_msg);
void vCheckInchatUser(int _clientSocket,MsgData *_msg);
void vServerDisplayInchatUser();
void vChatToOne(int _serverSocket, MsgData *_msg);
void vClientCheckInChatUser(int clientSocket,MsgData *msg);
void vSendFile(int clientSocket,MsgData *msg);
void vLinkOffline(int clientSocket);
MyLink * initLink();
int  iSocketInit();
int  iSocketAccept(int serverSocket);
void vCheckOfflineRecord(int _serverSocket, MsgData *_msg);
void vSaveOfflineRecord(int _serverSocket, MsgData * _msg);
char *pcGetTime();






char IP[15];					
short PORT;
MyLink *pH;//在线链表
