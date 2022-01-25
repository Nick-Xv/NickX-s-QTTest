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

	void s_GetPassword(PER_IO_CONTEXT1* pIoContext, char* buf);
	//void s_GetRecord(WSABUF* buf);
	//void s_PostRecord(WSABUF* buf);
	void s_PostRegist(PER_IO_CONTEXT1* pIoContext, char* buf);
	//void s_CheckPassword(WSABUF* buf);

	//发送答复报文
	void s_PostACK(PER_IO_CONTEXT1* pIoContext, int result);
};

