#include "MySqlHandler.h"



MySqlHandler::MySqlHandler()
{
}


MySqlHandler::~MySqlHandler()
{
}

bool MySqlHandler::connectDb() {
	//init
	mysql = new MYSQL;
	qDebug() << "1" << endl;
	mysql_init(mysql);
	qDebug() << mysql << endl;
	//false fail true success
	if (!mysql_real_connect(mysql, "localhost", "root", "123456", "QtUdpChat", 3306, nullptr, 0)) {
		qDebug() << "error connecting to database:" << mysql_error(mysql) << endl;
		return false;
	}
	else {
		qDebug() << "mysql connected" << endl;
		return true;
	}
	return true;
}

//通用查询
//colNum: 结果列数
//resultNum：结果行数
//ack：查询结果状态
//0:查询失败
//1:查询成功
bool MySqlHandler::queryDb(QString query, int colNum, int& resultNum, int& ack) {
	sprintf_s(this->query, query.toUtf8());
	//mysql_query(mysql, )
	if (mysql_query(mysql, this->query)) {
		qDebug() << "query failed " << mysql_error(mysql) << endl;
		ack = 0;
		return false;
	}
	else {
		qDebug() << "query success" << endl;
	}
	//获取结果集
	if (!(res = mysql_store_result(mysql))) {
		qDebug() << "can't get result" << endl;
		ack = 0;
		return false;
	}

	qDebug() << "number of dataline returned: " << mysql_affected_rows(mysql) << endl;
	resultNum = mysql_affected_rows(mysql);
	ack = 1;
	/*
	char* str_field[32];
	for (int i = 0; i < resultNum; i++) {
		str_field[i] = mysql_fetch_field(res)->name;
		qDebug() << str_field[i] << endl;
	}
	while (column = mysql_fetch_row(res)) {
		for (int i = 0; i < resultNum; i++) {
			qDebug() << column[i] << endl;
		}
	}
	*/
	return true;
}

//查询最后一条聊天记录的id
//colNum: 结果列数
//resultNum：结果行数
//ack：查询结果状态
//maxRecordId:最后一条记录的id
//0:查询失败
//1:查询成功
bool MySqlHandler::queryDb(QString query, int colNum, int& resultNum, int& ack, int& maxRecordId) {
	sprintf_s(this->query, query.toUtf8());
	//mysql_query(mysql, )
	if (mysql_query(mysql, this->query)) {
		qDebug() << "query failed " << mysql_error(mysql) << endl;
		ack = 0;
		return false;
	}
	else {
		qDebug() << "query success" << endl;
	}
	//获取结果集
	if (!(res = mysql_store_result(mysql))) {
		qDebug() << "can't get result" << endl;
		ack = 0;
		return false;
	}

	qDebug() << "number of dataline returned: " << mysql_affected_rows(mysql) << endl;
	resultNum = mysql_affected_rows(mysql);
	ack = 1;

	if (resultNum != 0) {//查到了聊天记录
		column = mysql_fetch_row(res);
		qDebug() << column[0] << endl;
		maxRecordId = atoi(column[0]);
	}
	return true;
}

//查询聊天记录
//colNum: 结果列数
//resultNum：结果行数
//ack：查询结果状态
//recordData: 查询到的记录
//0:查询失败
//1:查询成功
bool MySqlHandler::queryDb(QString query, int colNum, int& resultNum, int& ack, char* recordData) {
	sprintf_s(this->query, query.toUtf8());
	//mysql_query(mysql, )
	if (mysql_query(mysql, this->query)) {
		qDebug() << "query failed " << mysql_error(mysql) << endl;
		ack = 0;
		return false;
	}
	else {
		qDebug() << "query success" << endl;
	}
	//获取结果集
	if (!(res = mysql_store_result(mysql))) {
		qDebug() << "can't get result" << endl;
		ack = 0;
		return false;
	}

	qDebug() << "number of dataline returned: " << mysql_affected_rows(mysql) << endl;
	resultNum = mysql_affected_rows(mysql);
	ack = 1;

	//查到了数据
	int tempPtr = 2;//从2开始，第一位type,第二位ack
	if (resultNum >= 1) {
		while (column = mysql_fetch_row(res)) {
			for (int i = 0; i < colNum; i++) {
				qDebug() << column[i] << endl;
				memcpy(&recordData[tempPtr], column[i],strlen(column[i]));
				tempPtr += (strlen(column[i]) + 1);
			}
		}
	}
	return true;
}


//插入数据库
//ack：	2失败
//		3成功
bool MySqlHandler::queryDb(QString query, int& ack) {
	sprintf_s(this->query, query.toUtf8());
	//mysql_query(mysql, )
	if (mysql_query(mysql, this->query)) {
		qDebug() << "query failed " << mysql_error(mysql) << endl;
		ack = 2;
		return false;
	}
	else {
		ack = 3;
		qDebug() << "query success" << endl;
	}
	return true;
}

//查询一个字符串
bool MySqlHandler::queryDb(QString query, QString& result) {
	sprintf_s(this->query, query.toUtf8());
	//mysql_query(mysql, )
	if (mysql_query(mysql, this->query)) {
		qDebug() << "query failed " << mysql_error(mysql) << endl;
		return false;
	}
	else {
		qDebug() << "query success" << endl;
	}
	//获取结果集
	if (!(res = mysql_store_result(mysql))) {
		qDebug() << "can't get result" << endl;
		return false;
	}

	qDebug() << "number of dataline returned: " << mysql_affected_rows(mysql) << endl;

	if (mysql_affected_rows(mysql)) {
		column = mysql_fetch_row(res);
		result = column[0];
	}
	else {
		return false;
	}
	return true;
}

void MySqlHandler::closeDb() {
	if (mysql != nullptr) {
		mysql_close(mysql);
	}
}