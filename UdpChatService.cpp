#include "UdpChatService.h"

//构造
UdpChatService::UdpChatService()
{
	if (initService()) {
		QMessageBox::information(NULL, "信息", "服务器成功开启", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
	}
	else {
		QMessageBox::critical(NULL, "错误", "服务器打开失败", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
	}
}

//析构
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
	QObject::connect(iocpServer, &IocpServer::serviceHandler,
		this, &UdpChatService::serviceDispatcher);

	return (iocpServer->serverStart() && mysqlHandler->connectDb());
}

//关闭服务
void UdpChatService::closeService() {
	iocpServer->serverStop();
	mysqlHandler->closeDb();
}

//服务分配处理
void UdpChatService::serviceDispatcher(PER_IO_CONTEXT1* pIoContext, char* buff) {
	SERVICE_TYPE type = (SERVICE_TYPE)(int)buff[0];
	/*
	qDebug() << (int)buff[0] << endl;
	qDebug() << (int)buff[1] << endl;
	qDebug() << (int)buff[2] << endl;
	qDebug() << (int)buff[3] << endl;
	qDebug() << (int)buff[4] << endl;
	qDebug() << (int)buff[5] << endl;
	qDebug() << (int)buff[6] << endl;
	qDebug() << (int)buff[7] << endl;
	qDebug() << (int)buff[8] << endl;
	qDebug() << (int)buff[9] << endl;
	qDebug() << (int)buff[10] << endl;
	qDebug() << (int)buff[11] << endl;
	qDebug() << (int)buff[12] << endl;
	*/
	qDebug() << inet_ntoa(pIoContext->remoteAddr.sin_addr) << "!!!" << endl;
	switch (type) {
	/*case GET_PASSWORD:
		s_GetPassword(pIoContext, buff);
		break;*/
	case GET_RECORD:
		s_GetRecord(pIoContext, buff);
		break;
	case POST_RECORD:
		s_PostRecord(pIoContext, buff);
		break;
	case POST_REGIST:
		s_PostRegist(pIoContext, buff);
		break;
	case CHECK_PASSWORD:
		s_CheckPassword(pIoContext, buff);
		break;
	}
}

//发送返回ack值
void UdpChatService::s_PostACK(PER_IO_CONTEXT1* pIoContext, int result) {
	pIoContext->m_szBuffer[0] = result;
	iocpServer->SendDataTo(pIoContext);
}
/*
//buf[5]到buf[8] userId
void UdpChatService::s_GetPassword(PER_IO_CONTEXT1* pIoContext, char* buff) {
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
}
*/

//用户注册 包括查询用户名和插入用户名密码
//buf[5]到第一个0，username
//后面到0，password
//ACK值：0-数据错误
//		 1-用户名已存在
//		 2-数据库错误
//		 3-插入成功
void UdpChatService::s_PostRegist(PER_IO_CONTEXT1* pIoContext, char* buf) {
	//获取username和password
	bool judge = false;
	int i = 5;
	int usernamePtr;
	int passwordPtr;
	int ack = -1;
	while (buf[i]!=0 || judge==false) {
		if (buf[i] == 0) {
			usernamePtr = i;
			judge = true;
		}
		i++;
	}
	passwordPtr = i;
	//数据不完整直接返回false
	if (usernamePtr == 5 || usernamePtr == passwordPtr - 1) {
		s_PostACK(pIoContext, 0);
		return;
	}
	qDebug() << usernamePtr << passwordPtr << endl;
	char* username = new char[usernamePtr - 4];
	char* password = new char[passwordPtr - usernamePtr];
	memset(username,0,usernamePtr-4);
	memset(password, 0, passwordPtr - usernamePtr);
	memcpy(username, &buf[5], usernamePtr - 5);
	memcpy(password, &buf[usernamePtr+1], passwordPtr - usernamePtr - 1);

	//查询数据库的username
	QString query;
	query = "select userName from users where userName='";
	query.append(username);
	query.append("';");
	qDebug() << query << endl;
	mysqlHandler -> queryDb(query, 1, ack);
	//出现异常直接返回
	if (ack != -1) {
		s_PostACK(pIoContext, ack);
	}
	else {
		//插入用户名和密码
		query = "insert into users (userName,userPassword) values('";
		query.append(username);
		query.append("','");
		query.append(password);
		query.append("');");
		qDebug() << query << endl;
		mysqlHandler->queryDb(query, ack);
		if (ack == -1) ack = 2;
		s_PostACK(pIoContext, ack);
	}
}

//获取聊天记录 
//一次查10条
//通过提供的roomid和当前查询过的最后一条idrecords
//buf[5]到第一个0，roomid
//再到0，idrecords
//ACK值：0-数据错误
//		 1-查询成功
//		 2-数据库错误
//缓冲区结尾标记两字节0
void UdpChatService::s_GetRecord(PER_IO_CONTEXT1* pIoContext, char* buf) {

}


//发送消息，并且发给当前聊天室中的所有用户处
//提供roomid, userid, content
//buf[5]到第一个0，roomid
//再到0，userid
//再到0，content
//ACK值：0-数据错误
//		 1-插入成功
//		 2-数据库错误
//缓冲区结尾标记两字节0
void UdpChatService::s_PostRecord(PER_IO_CONTEXT1* pIoContext, char* buf) {

}

//检查密码正确
//提供userName, userPassword
//buf[5]到第一个0，userName
//再到0，userPassword
//ACK值：0-数据错误
//		 1-密码正确
//		 2-数据库错误
//		 3-密码错误
//缓冲区结尾标记两字节0
void UdpChatService::s_CheckPassword(PER_IO_CONTEXT1* pIoContext, char* buf) {

}