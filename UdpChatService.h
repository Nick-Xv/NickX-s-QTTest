#pragma once
#include "mysqlhandler.h"
#include "iocpserver.h"
#include <QMessageBox>
enum SERVICE_TYPE {
	//GET
	GET_PASSWORD,
	GET_RECORD,
	//POST
	POST_REGIST,
	POST_RECORD,
	//CHECK
	CHECK_PASSWORD
};

class UdpChatService
{
public:
	UdpChatService();
	~UdpChatService();
	bool initService();
	void closeService();

	//bool get
public slots:
void serviceDispatcher(SERVICE_TYPE type);

private:
	MySqlHandler* mysqlHandler;
	IocpServer* iocpServer;


};

