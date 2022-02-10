#pragma once
#include "mysqlhandler.h"
#include "iocpserver.h"
#include <winsock2.h>
#include <QMessageBox>
#include <map>
#include <utility>
using std::pair;
using std::map;
enum SERVICE_TYPE {
	NO_SERVICE,
	//GET
	GET_PASSWORD,
	GET_RECORD,
	//POST
	POST_REGIST,
	POST_RECORD,
	//CHECK
	CHECK_PASSWORD,
	CHECK_HEARTBEAT,
	//SEND
	SEND_RECORD,
	SEND_RECORDS,
	SEND_ACK
};

class UdpChatService : public QObject
{
	Q_OBJECT
public:
	UdpChatService();
	~UdpChatService();
	bool initService();
	void closeService();

protected:
	static DWORD WINAPI _CheckHeartbeatThread(LPVOID lpParam);//心跳线程函数

	//bool get
public slots:
void serviceDispatcher(PER_IO_CONTEXT1* pIoContext, char* buf, MySqlHandler* mysqlHandler);

private:
	IocpServer* iocpServer;

	HANDLE* HeartbeatThreadHandle;

	map<int, pair<PER_IO_CONTEXT1*,int>>* m_arrayClientContext[1000];//客户端vector指针数组

	//找回密码
	//void s_GetPassword(PER_IO_CONTEXT1* pIoContext, char* buf);
	//获取聊天记录***未测试
	void s_GetRecord(PER_IO_CONTEXT1* pIoContext, char* buf, MySqlHandler* mysqlHandler);
	//发送消息***未测试
	void s_PostRecord(PER_IO_CONTEXT1* pIoContext, char* buf, MySqlHandler* mysqlHandler);
	//用户注册tested
	void s_PostRegist(PER_IO_CONTEXT1* pIoContext, char* buf, MySqlHandler* mysqlHandler);
	//检查密码***未测试
	void s_CheckPassword(PER_IO_CONTEXT1* pIoContext, char* buf, MySqlHandler* mysqlHandler);
	//心跳监测tested
	void s_CheckHeartbeat(PER_IO_CONTEXT1* pIoContext, char* buf, MySqlHandler* mysqlHandler);

	//发送答复报文tested
	void s_PostACK(PER_IO_CONTEXT1* pIoContext, int result, SERVICE_TYPE type);
};

