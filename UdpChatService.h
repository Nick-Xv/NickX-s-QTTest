#pragma once
#include "mysqlhandler.h"
#include "iocpserver.h"
#include <winsock2.h>
#include <QMessageBox>
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
	CHECK_HEARTBEAT
};

class UdpChatService : public QObject
{
	Q_OBJECT
public:
	UdpChatService();
	~UdpChatService();
	bool initService();
	void closeService();

	//bool get
public slots:
void serviceDispatcher(PER_IO_CONTEXT1* pIoContext, char* buf);

private:
	MySqlHandler* mysqlHandler;
	IocpServer* iocpServer;

	bool s_GetPassword(PER_IO_CONTEXT1* pIoContext, char* buf);
	//bool s_GetRecord(WSABUF* buf);
	//bool s_PostRecord(WSABUF* buf);
	//bool s_PostRegist(WSABUF* buf);
	//bool s_CheckPassword(WSABUF* buf);
};

