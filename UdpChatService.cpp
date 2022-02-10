#include "UdpChatService.h"
//释放指针宏
#define RELEASE(x) {if((x)!=nullptr){delete(x);(x)=nullptr;}}
//释放句柄宏
#define RELEASE_HANDLE(x) {if((x)!=nullptr&&(x)!=INVALID_HANDLE_VALUE){CloseHandle(x);(x)=nullptr;}}

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
}

//打开服务
bool UdpChatService::initService() {
	iocpServer = new IocpServer();

	for (int i = 0; i < 1000; i++) {
		m_arrayClientContext[i] = nullptr;
	}
	//打开心跳检测线程
	HeartbeatThreadHandle = new HANDLE;
	*HeartbeatThreadHandle = ::CreateThread(0, 0, _CheckHeartbeatThread, this, 0, nullptr);
	

	//绑定服务信号槽
	QObject::connect(iocpServer, &IocpServer::serviceHandler,
		this, &UdpChatService::serviceDispatcher);

	return (iocpServer->serverStart());
}

//关闭服务
void UdpChatService::closeService() {
	iocpServer->serverStop();
	RELEASE_HANDLE(*HeartbeatThreadHandle);
	RELEASE(HeartbeatThreadHandle);
}

//服务分配处理
void UdpChatService::serviceDispatcher(PER_IO_CONTEXT1* pIoContext, char* buff, MySqlHandler* mysqlHandler) {
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
		s_GetRecord(pIoContext, buff, mysqlHandler);
		break;
	case POST_RECORD:
		s_PostRecord(pIoContext, buff, mysqlHandler);
		break;
	case POST_REGIST:
		s_PostRegist(pIoContext, buff, mysqlHandler);
		break;
	case CHECK_PASSWORD:
		s_CheckPassword(pIoContext, buff, mysqlHandler);
		break;
	case CHECK_HEARTBEAT:
		s_CheckHeartbeat(pIoContext, buff, mysqlHandler);
		break;
	}
}

//发送返回ack值
void UdpChatService::s_PostACK(PER_IO_CONTEXT1* pIoContext, int result, SERVICE_TYPE type) {
	pIoContext->m_szBuffer[0] = type;
	pIoContext->m_szBuffer[1] = result;
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
void UdpChatService::s_PostRegist(PER_IO_CONTEXT1* pIoContext, char* buf, MySqlHandler* mysqlHandler) {
	//获取username和password
	int i = 5;
	int beginPtr[2] = { 0 };
	int endPtr[2] = { 0 };
	int num = 0;
	beginPtr[num] = i;
	while (buf[i] != 0 || buf[i + 1] != 0) {
		if (buf[i] == 0) {
			endPtr[num] = i;
			beginPtr[++num] = i + 1;
		}
		i++;
	}
	endPtr[num] = i;

	//数据不完整直接返回false
	if (beginPtr[1] == 0) {
		s_PostACK(pIoContext, 0, SEND_ACK);
		return;
	}

	char* username = new char[endPtr[0] - beginPtr[0] + 1];
	char* password = new char[endPtr[1] - beginPtr[1] + 1];
	memset(username, 0, endPtr[0] - beginPtr[0] + 1);
	memset(password, 0, endPtr[1] - beginPtr[1] + 1);
	memcpy(username, &buf[beginPtr[0]], endPtr[0] - beginPtr[0]);
	memcpy(password, &buf[beginPtr[1]], endPtr[1] - beginPtr[1]);

	//查询数据库的username
	QString query;
	query = "select userName from users where userName='";
	query.append(username);
	query.append("';");
	qDebug() << query << endl;

	int ack = -1;
	int resultNum = -1;
	mysqlHandler -> queryDb(query, 1, resultNum, ack);
	//出现异常直接返回
	if (ack == 0) {
		s_PostACK(pIoContext, 2, SEND_ACK);
	}
	//用户名已存在
	if (resultNum >= 1) {
		s_PostACK(pIoContext, 1, SEND_ACK);
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
		s_PostACK(pIoContext, ack, SEND_ACK);
	}
}

//获取聊天记录 
//一次扩展20条
//通过提供的roomid和当前查询过的最后一条idrecords
//buf[5]到第一个0，roomid
//再到0，idrecords
//ACK值：0-数据错误
//		 1-查询成功
//		 2-数据库错误
//缓冲区除ack以外后面的都是数据
//缓冲区结尾标记两字节0
void UdpChatService::s_GetRecord(PER_IO_CONTEXT1* pIoContext, char* buf, MySqlHandler* mysqlHandler) {
	//获取roomid和idrecords
	int i = 5;
	int beginPtr[2] = { 0 };
	int endPtr[2] = { 0 };
	int num = 0;
	beginPtr[num] = i;
	while (buf[i] != 0 || buf[i + 1] != 0) {
		if (buf[i] == 0) {
			endPtr[num] = i;
			beginPtr[++num] = i + 1;
		}
		i++;
	}
	endPtr[num] = i;

	//数据不完整直接返回false
	if (beginPtr[1] == 0) {
		s_PostACK(pIoContext, 0, SEND_ACK);
		return;
	}

	char* roomid = new char[endPtr[0] - beginPtr[0] + 1];
	char* idrecords = new char[endPtr[1] - beginPtr[1] + 1];
	memset(roomid, 0, endPtr[0] - beginPtr[0] + 1);
	memset(idrecords, 0, endPtr[1] - beginPtr[1] + 1);
	memcpy(roomid, &buf[beginPtr[0]], endPtr[0] - beginPtr[0]);
	memcpy(idrecords, &buf[beginPtr[1]], endPtr[1] - beginPtr[1]);

	int roomID = atoi(roomid);
	int idRecords = atoi(idrecords);
	//数据错误，房间号不能为0
	if (roomID == 0) {
		s_PostACK(pIoContext, 1, SEND_ACK);
		return;
	}

	//存储所有查询到的聊天记录
	char recordData[8192];
	memset(recordData, 0, 8192);

	QString query;
	int resultNum = -1, ack = -1;
	//如果当前没有获取聊天记录，首先获取最后的20条
	//先查询最后一条的idrecords
	int maxRecordId = -1;
	if (idRecords == 0) {
		query = "select max(idrecords) from records where roomid=";
		query.append(roomID);
		query.append(";");
		qDebug() << query << endl;
		mysqlHandler->queryDb(query, 1, resultNum, ack, maxRecordId);
		//查到了最大记录编号
		if (maxRecordId != -1) {
			idRecords = maxRecordId;
		}
	}
	//此时表示没有聊天记录
	if (idRecords == 0) {
		//先把缓冲区置0
		memset(pIoContext->m_szBuffer, 0, sizeof(pIoContext->m_szBuffer));
		s_PostACK(pIoContext, 1, SEND_ACK);
		return;
	}
	//开始查询
	query = "select * from records where roomid=";
	query.append(roomID);
	query.append(" and (idrecords between ");
	query.append(idRecords - 20);
	query.append(" and ");
	query.append(idRecords);
	query.append(");");
	qDebug() << query << endl;
	mysqlHandler->queryDb(query, 5, resultNum, ack, &recordData[0]);
	memcpy(pIoContext->m_szBuffer, &recordData[0], sizeof(pIoContext->m_szBuffer));
	s_PostACK(pIoContext, 1, SEND_RECORDS);
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
void UdpChatService::s_PostRecord(PER_IO_CONTEXT1* pIoContext, char* buf, MySqlHandler* mysqlHandler) {
	//获取roomid, userid, content
	int i = 5;
	int beginPtr[3] = { 0 };
	int endPtr[3] = { 0 };
	int num = 0;
	beginPtr[num] = i;
	while (buf[i] != 0 || buf[i + 1] != 0) {
		if (buf[i] == 0) {
			endPtr[num] = i;
			beginPtr[++num] = i + 1;
		}
		i++;
	}
	endPtr[num] = i;

	//数据不完整直接返回false
	if (beginPtr[1] == 0 || beginPtr[2] == 0) {
		s_PostACK(pIoContext, 0, SEND_ACK);
		return;
	}

	char* roomid = new char[endPtr[0] - beginPtr[0] + 1];
	char* userid = new char[endPtr[1] - beginPtr[1] + 1];
	char* content = new char[endPtr[2] - beginPtr[2] + 1];
	memset(roomid, 0, endPtr[0] - beginPtr[0] + 1);
	memset(userid, 0, endPtr[1] - beginPtr[1] + 1);
	memset(content, 0, endPtr[2] - beginPtr[2] + 1);
	memcpy(roomid, &buf[beginPtr[0]], endPtr[0] - beginPtr[0]);
	memcpy(userid, &buf[beginPtr[1]], endPtr[1] - beginPtr[1]);
	memcpy(content, &buf[beginPtr[2]], endPtr[2] - beginPtr[2]);

	int roomID = atoi(roomid);
	int userID = atoi(userid);

	//插入数据库
	QString query;
	query = "insert into records (roomid, userid, content) values (";
	query.append(roomid);
	query.append(",");
	query.append(userid);
	query.append(",'");
	query.append(content);
	query.append("');");
	qDebug() << query << endl;
	int ack = -1;
	mysqlHandler->queryDb(query, ack);
	//插入失败
	if (ack != 3) {
		s_PostACK(pIoContext, 2, SEND_ACK);
	}
	else {
		s_PostACK(pIoContext, 1, SEND_ACK);
		//开始发送给其他用户
		//首先获取刚才插入的消息
		QString lastID = nullptr;
		mysqlHandler->queryDb("SELECT LAST_INSERT_ID();", lastID);
		qDebug() << lastID << endl;
		//然后获取刚才插入的完整内容
		query = "select * from records where idrecords=";
		query.append(lastID);
		query.append(";");
		qDebug() << query << endl;
		int ack = -1;
		int resultNum = -1;
		char recordData[8192];
		memset(&recordData[0], 0, 8192);
		mysqlHandler->queryDb(query, 5, resultNum, ack, &recordData[0]);
		map<int, pair<PER_IO_CONTEXT1*, int>>::iterator iter;
		if (m_arrayClientContext[roomID]->size() >= 2) {//超过两人在线
			for (iter = m_arrayClientContext[roomID]->begin(); iter != m_arrayClientContext[roomID]->end(); iter++) {
				if (iter->first == userID) {//跳过本人
					continue;
				}
				else { 
					//发送SENDTO
					memcpy(iter->second.first->m_szBuffer, &recordData[0], 8192);
					s_PostACK(iter->second.first, 1, SEND_RECORD); 
				}
			}
		}
	}
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
void UdpChatService::s_CheckPassword(PER_IO_CONTEXT1* pIoContext, char* buf, MySqlHandler* mysqlHandler) {
	//获取userName和userPassword
	int i = 5;
	int beginPtr[2] = { 0 };
	int endPtr[2] = { 0 };
	int num = 0;
	beginPtr[num] = i;
	while (buf[i] != 0 || buf[i + 1] != 0) {
		if (buf[i] == 0) {
			endPtr[num] = i;
			beginPtr[++num] = i + 1;
		}
		i++;
	}
	endPtr[num] = i;

	//数据不完整直接返回false
	if (beginPtr[1] == 0) {
		s_PostACK(pIoContext, 0, SEND_ACK);
		return;
	}
	char* userName = new char[endPtr[0] - beginPtr[0] + 1];
	char* userPassword = new char[endPtr[1] - beginPtr[1] + 1];
	memset(userName, 0, endPtr[0] - beginPtr[0] + 1);
	memset(userPassword, 0, endPtr[1] - beginPtr[1] + 1);
	memcpy(userName, &buf[beginPtr[0]], endPtr[0] - beginPtr[0]);
	memcpy(userPassword, &buf[beginPtr[1]], endPtr[1] - beginPtr[1]);

	//查询用户名密码对应关系是否正确
	QString query;
	query = "select * from users where userName='";
	query.append(userName);
	query.append("' and userPassword='");
	query.append(userPassword);
	query.append("';");
	qDebug() << query << endl;
	int resultNum = -1, ack = -1;
	mysqlHandler->queryDb(query, 1, resultNum, ack);

	//出现异常直接返回
	if (ack == 0) {
		s_PostACK(pIoContext, 2, SEND_ACK);
	}
	//密码正确
	if (resultNum >= 1) {
		s_PostACK(pIoContext, 1, SEND_ACK);
	}
	else {//用户名密码有一个错误
		s_PostACK(pIoContext, 3, SEND_ACK);
	}
}

//心跳监测
//提供userid,roomid
//buf[5]到第一个0，userid
//再到0，roomid
//ACK值：0-数据错误
//		 1-正确接收
//缓冲区结尾标记两字节0
void UdpChatService::s_CheckHeartbeat(PER_IO_CONTEXT1* pIoContext, char* buf, MySqlHandler* mysqlHandler) {
	//获取userid和roomid
	int i = 5;
	int beginPtr[2] = { 0 };
	int endPtr[2] = { 0 };
	int num = 0;
	beginPtr[num] = i;
	while (buf[i] != 0 || buf[i+1] != 0) {
		if (buf[i] == 0) {
			endPtr[num] = i;
			beginPtr[++num] = i + 1;
		}
		i++;
	}
	endPtr[num] = i;

	//数据不完整直接返回false
	if (beginPtr[1] == 0) {
		s_PostACK(pIoContext, 0, SEND_ACK);
		return;
	}
	char* userid = new char[endPtr[0] - beginPtr[0] + 1];
	char* roomid = new char[endPtr[1] - beginPtr[1] + 1];
	memset(userid, 0, endPtr[0] - beginPtr[0] + 1);
	memset(roomid, 0, endPtr[1] - beginPtr[1] + 1);
	memcpy(userid, &buf[beginPtr[0]], endPtr[0] - beginPtr[0]);
	memcpy(roomid, &buf[beginPtr[1]], endPtr[1] - beginPtr[1]);

	int roomID = atoi(roomid);
	int userID = atoi(userid);
	//数据获取错误，房间号不能为0
	if (roomID == 0) {
		s_PostACK(pIoContext, 0, SEND_ACK);
		return;
	}
	//空指针时新建map
	if (m_arrayClientContext[roomID] == nullptr) {
		m_arrayClientContext[roomID] = new map<int, pair<PER_IO_CONTEXT1*,int>>;
	}
	//空map就插入数据
	if (m_arrayClientContext[roomID]->size() == 0) {
		QString temp = "user ";
		temp.append(userid);
		temp.append(" checkin");
		qDebug() << temp << endl;
		m_arrayClientContext[roomID]->insert(map<int, pair<PER_IO_CONTEXT1*, int>>::value_type(userID, pair<PER_IO_CONTEXT1*, int>(pIoContext,0)));
	}
	else {
		map<int, pair<PER_IO_CONTEXT1*, int>>::iterator iter;
		iter = m_arrayClientContext[roomID]->find(userID);
		if (iter == m_arrayClientContext[roomID]->end()) {//没查到,插入
			QString temp = "user ";
			temp.append(userid);
			temp.append(" checkin");
			qDebug() << temp << endl;
			m_arrayClientContext[roomID]->insert(map<int, pair<PER_IO_CONTEXT1*, int>>::value_type(userID, pair<PER_IO_CONTEXT1*, int>(pIoContext, 0)));
		}
		else {
			QString temp = "user ";
			temp.append(userid);
			temp.append(" setZero");
			qDebug() << temp << endl;
			//查到了，将延时清零
			iter->second.second = 0;
		}
	}
	s_PostACK(pIoContext, 1, SEND_ACK);
}

//心跳线程函数
//每20秒给所有注册用户+1
//加到2（40秒）就注销用户
DWORD WINAPI UdpChatService::_CheckHeartbeatThread(LPVOID lpParam) {
	UdpChatService* pParam = static_cast<UdpChatService*>(lpParam);
	//每20秒检测一次
	while (1) {
		Sleep(20000);
		for (int i = 0; i < 1000; i++) {
			if (pParam->m_arrayClientContext[i] != nullptr) {
				if (pParam->m_arrayClientContext[i]->size() > 0) {
					map<int, pair<PER_IO_CONTEXT1*, int>>::iterator iter;
					for (iter = pParam->m_arrayClientContext[i]->begin(); iter != pParam->m_arrayClientContext[i]->end(); ) {
						iter->second.second++;
						//超时删除
						if (iter->second.second >= 2) {
							pParam->m_arrayClientContext[i]->erase(iter++);
							qDebug() << "删除用户" << endl;
							//iter--;
						}
						else {
							iter++;
						}
					}
				}
			}
		}
	}
	return 0;
}