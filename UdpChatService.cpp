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

	//绑定服务信号槽
	QObject::connect(iocpServer, &IocpServer::serviceHandler, this, &UdpChatService::serviceDispatcher);

	return (iocpServer->serverStart() && mysqlHandler->connectDb());
}

//关闭服务
void UdpChatService::closeService() {
	iocpServer->serverStop();
	mysqlHandler->closeDb();
}

//服务处理器
void UdpChatService::serviceDispatcher(PER_IO_CONTEXT1* pIoContext, char* buff) {
	SERVICE_TYPE type = (SERVICE_TYPE)(int)buff[0];
	qDebug() << (int)buff[0] << endl;
	qDebug() << (int)buff[1] << endl;
	qDebug() << (int)buff[2] << endl;
	qDebug() << (int)buff[3] << endl;
	qDebug() << (int)buff[4] << endl;
	qDebug() << inet_ntoa(pIoContext->remoteAddr.sin_addr) << "!!!" << endl;
	switch (type) {
	case GET_PASSWORD:
		s_GetPassword(pIoContext, buff);
		break;
	case GET_RECORD:
		//s_GetRecord(buf);
		break;
	case POST_RECORD:
		//s_PostRecord(buf);
		break;
	case POST_REGIST:
		//s_PostRegist(buf);
		break;
	case CHECK_PASSWORD:
		//s_CheckPassword(buf);
		break;
	}
}

//buf[5]到buf[8] userId
bool UdpChatService::s_GetPassword(PER_IO_CONTEXT1* pIoContext, char* buff) {
	//char* buf = pIoContext->m_szBuffer;
	char* buf = buff;
	int userId;
	qDebug() << (int)buf[0] << endl;
	unsigned short s1, s2;
	memcpy(&userId,&buf[5],sizeof(userId));
	memcpy(&s1, &buf[1], sizeof(s1));
	memcpy(&s2, &buf[3], sizeof(s2));
	qDebug() << s1 << s2 << userId << "!" << endl;

	//查询数据库
	QString query;
	query = "select userPassword from users where userID=";
	query.append(QString::number(userId).append(";"));
	qDebug() << query << endl;
	QString password;
	mysqlHandler->queryDb(query, password);
	qDebug() << password << endl;

	//发送数据
	//iocpServer->

	return true;
}