//CMD_SEND_TO_SERVER
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


//CMD_RECV_FROM_SERVER
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


#define PORT			7777

//Socket数据包
typedef struct MsgData
{
	int  m_cmd;					//cli工作指令
	int  m_flag;					//回执flag
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

typedef struct ChatRecord
{
	char m_mess[1024];
	char m_fromName[30];
	char m_toName[30];
	char m_time[30];
	int m_flag;//判断私聊还是群聊 1私聊，0群聊
}ChatRecord;


char IP[15];//服务器的IP
//short PORT;//服务器服务端口
int iClientSocket;
char cUsrName[30];
char cUsrAccount[30];
int isChatOneOnline;
int rootFlag;//判断管理员flag
int bossFlag;


void recvThread(void* _ClientSocket);
void vSaveChatRecord(MsgData *_msg,int _flag);
char *pcGetTime();
void vSaveFile(MsgData *_msg);
void vFirstMenu();
void vRegisterNewAccount();
void vCheckOnlineUsr();
void vChatToOne();
void vCheckChatRecord();
void vSendFile();
void vThirdChatMenu();
void vChatToAll();
int  iUserLogin();
void vSecondMenuAction();
void vSendLogoutMsg();
void vFirstMenuActions();
char *pcMyfgets(char *data, int n);
int  iSocketInit();
int  iGetRand();
void vReadOfflineRecord(MsgData *_msg);
void vCheckOfflineRecord();


