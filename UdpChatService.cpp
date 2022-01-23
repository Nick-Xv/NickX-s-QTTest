#include "UdpChatService.h"



UdpChatService::UdpChatService()
{
	if (initService()) {
		QMessageBox::information(NULL, "信息", "服务器成功开启", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
	}
	else {
		QMessageBox::critical(NULL, "错误", "服务器打开失败", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
	}
}


UdpChatService::~UdpChatService()
{
	closeService();
	delete iocpServer;
	delete mysqlHandler;
}

//打开服务
bool UdpChatService::initService() {
	iocpServer = new IocpServer();
	mysqlHandler = new MySqlHandler();
	return (iocpServer->serverStart() && mysqlHandler->connectDb());
}

//关闭服务
void UdpChatService::closeService() {
	iocpServer->serverStop();
	mysqlHandler->closeDb();
}

void UdpChatService::serviceDispatcher(SERVICE_TYPE type) {
	switch (type) {
	case GET_PASSWORD:
		break;
	case GET_RECORD:
		break;
	case POST_RECORD:
		break;
	case POST_REGIST:
		break;
	case CHECK_PASSWORD:
		break;
	}
}