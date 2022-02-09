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
	mysql_init(mysql);
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

//查询是否存在用户名
//resultNum：结果列数
//ack：查询结果
bool MySqlHandler::queryDb(QString query, int resultNum, int& ack) {
	sprintf_s(this->query, query.toUtf8());
	//mysql_query(mysql, )
	if (mysql_query(mysql, this->query)) {
		qDebug() << "query failed " << mysql_error(mysql) << endl;
		ack = 2;
		return false;
	}
	else {
		qDebug() << "query success" << endl;
	}
	//获取结果集
	if (!(res = mysql_store_result(mysql))) {
		qDebug() << "can't get result" << endl;
		ack = 2;
		return false;
	}

	qDebug() << "number of dataline returned: " << mysql_affected_rows(mysql) << endl;
	if (mysql_affected_rows(mysql)!=0) {
		ack = 1;
		return false;
	}
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


//插入数据库(用户名和密码)
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