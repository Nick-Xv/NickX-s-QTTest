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

//数据库写入
bool MySqlHandler::queryDb(QString query, int resultNum) {
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