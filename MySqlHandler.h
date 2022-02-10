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

	//通用查询
	//ack:0执行失败,1成功
	bool queryDb(QString query, int colNum, int& resultNum, int& ack);
	
	//查询最后一条记录的编号
	bool queryDb(QString query, int colNum, int& resultNum, int& ack, int& maxRecordId);
	//查询聊天记录
	bool queryDb(QString query, int colNum, int& resultNum, int& ack, char* recordData );
	bool queryDb(QString query, int& ack);
	bool queryDb(QString query, QString& result);
	void closeDb();
private:
	MYSQL* mysql;//mysql连接
	MYSQL_FIELD* fd;//字段列数组
	char field[32][32];//存字段名
	MYSQL_RES* res;//返回行的一个查询结果集
	MYSQL_ROW column;//表示数据行的列
	char query[150];//查询语句
};
