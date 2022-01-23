#pragma once

#include <winsock2.h>
#include <windows.h>
#include <mysql.h>
#include <QDebug>

class MySqlHandler
{
public:
	MySqlHandler();
	~MySqlHandler();
	bool connectDb();
	bool queryDb(QString query);
	void closeDb();
private:
	MYSQL* mysql;//mysql连接
	MYSQL_FIELD* fd;//字段列数组
	char field[32][32];//存字段名
	MYSQL_RES* res;//返回行的一个查询结果集
	MYSQL_ROW column;//表示数据行的列
	char query[150];//查询语句
};
