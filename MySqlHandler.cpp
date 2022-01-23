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

bool MySqlHandler::queryDb(QString query) {
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
	for (int i = 0; i < 2; i++) {
		str_field[i] = mysql_fetch_field(res)->name;
	}
	qDebug() << str_field[0] << str_field[1] << endl;
	while (column = mysql_fetch_row(res)) {
		qDebug() << column[0] << column[1] << endl;
	}
	return true;
}

void MySqlHandler::closeDb() {
	if (mysql != nullptr) {
		mysql_close(mysql);
	}
}