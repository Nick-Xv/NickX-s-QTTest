//#if defined(_MSC_VER) && (_MSC_VER >= 1600)
//# pragma execution_character_set("utf-8")
//#endif
#include "mainwindow.h"
#include "serialport.h"
#include "tcptest.h"
#include "udptest.h"
#include "multithreadtest.h"
#include "MySqlHandler.h"
#include "completionporttest.h"
#include "iocpserver.h"
#include "udpchatservice.h"

#include <QApplication>
#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStringList>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDesktopWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>

QString getIpTest();
UdpChatService* service;

void testButton() {
	service = new UdpChatService();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //窗口居中测试
    QDesktopWidget* pDesk = QApplication::desktop();

    MainWindow mw;
    mw.resize(500,500);
    mw.move((pDesk->width() - mw.width()) / 2, (pDesk->height() - mw.height()) / 2);

    QPushButton mwbut1(&mw);
    mwbut1.setGeometry(50,50,100,50);
    mwbut1.setText("布局测试");

    QPushButton mwbut2(&mw);
    mwbut2.setGeometry(50,100,100,50);
    mwbut2.setText("串口测试");

    QPushButton mwbut3(&mw);
    mwbut3.setGeometry(50,150,100,50);
    mwbut3.setText("tcp测试");

    QPushButton mwbut4(&mw);
    mwbut4.setGeometry(50,200,100,50);
    mwbut4.setText("udp测试");

    QPushButton mwbut5(&mw);
    mwbut5.setGeometry(50,250,100,50);
    mwbut5.setText("多线程测试");

    QPushButton mwbut6(&mw);
    mwbut6.setGeometry(50,300,100,50);
    mwbut6.setText("完成端口测试");

	QPushButton mwbut7(&mw);
	mwbut7.setGeometry(50, 350, 100, 50);
	mwbut7.setText("打开聊天室服务");

    mw.show();
    //QWidget widget;
    //widget.setWindowTitle("QVBoxLayout垂直布局");
    //widget.setGeometry(100,100,500,500);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setDirection(QBoxLayout::BottomToTop);

    //连续创建 3 个文本框，并设置它们的背景和字体大小
    QLabel lab1("Label1");
    lab1.setStyleSheet("QLabel{background:#dddddd;font:20px;}");
    lab1.setAlignment(Qt::AlignCenter);
    QLabel lab2("Label2");
    lab2.setStyleSheet("QLabel{background:#cccccc;font:20px;}");
    lab2.setAlignment(Qt::AlignCenter);
    QLabel lab3("Label3");
    lab3.setStyleSheet("QLabel{background:#ffffff;font:20px;}");
    lab3.setAlignment(Qt::AlignCenter);

    QPushButton but1("TEST table");

    layout->addStretch(2);
    layout->addWidget(&lab1,1);
    layout->addWidget(&lab2,2);
    layout->addWidget(&lab3,3);
    layout->addStretch(3);
    layout->addWidget(&but1,3);

    //widget.setLayout(layout);

    //show前移动到居中
    //widget.move((pDesk->width() - widget.width()) / 2, (pDesk->height() - widget.height()) / 2);

    //串口测试
    //SerialPort widget2;

    //tcp测试
    //TcpTest widget3;

    //udp测试
    //UdpTest widget4;

    //多线程测试
    //MultiThreadTest widget5;

    //完成端口测试
    //CompletionPortTest widget6;

    //添加信号和槽机制，当某个结点被点击时，调整所有子结点的选中状态，同时将所有子结点展开。
    //QObject::connect(&mwbut1,&QPushButton::clicked,&widget,&QWidget::show);
    //QObject::connect(&mwbut2,&QPushButton::clicked,&widget2,&QWidget::show);
    //QObject::connect(&mwbut3,&QPushButton::clicked,&widget3,&QWidget::show);
    //QObject::connect(&mwbut4,&QPushButton::clicked,&widget4,&QWidget::show);
    //QObject::connect(&mwbut5,&QPushButton::clicked,&widget5,&QWidget::show);
    //QObject::connect(&mwbut6,&QPushButton::clicked,&widget6,&QWidget::show);
	QObject::connect(&mwbut7, &QPushButton::clicked, &mw, &testButton);
	return a.exec();
}

QString getIpTest(){
    WSADATA WSAData;
    char hostName[256];
    if(!WSAStartup(MAKEWORD(2,0),&WSAData)){
        if(!gethostname(hostName,sizeof(hostName))){
            hostent* host = gethostbyname(hostName);
            if(host!=nullptr){
                return inet_ntoa(*(struct in_addr*)* host->h_addr_list);
            }
        }
    }
    return "failed";
}