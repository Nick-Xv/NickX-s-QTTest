#include "udptest.h"


UdpTest::UdpTest(QWidget* parent):QWidget(parent)
{
    this->setGeometry(50,50,500,500);
    inputText = new QTextEdit(this);
    infoText = new QTextEdit(this);
    layout1 = new QVBoxLayout(this);

    layout2 = new QHBoxLayout;
    layout1->setDirection(QBoxLayout::TopToBottom);
    layout2->setDirection(QBoxLayout::LeftToRight);

    sendBut = new QPushButton(this);
    sendBut10 = new QPushButton(this);
    sendBut->setText("开始接收");
    sendBut10->setText("发送10");
    inputText->setPlaceholderText("在此输入信息以发送");

    layout1->addLayout(layout2,1);
    layout1->addWidget(inputText,3);
    layout1->addWidget(infoText,6);
    layout2->addWidget(sendBut,1);
    layout2->addWidget(sendBut10,1);
    this->setLayout(layout1);
    task = new WorkThread;
    connect(task,&WorkThread::appendText,this,&UdpTest::appendTextSlot);
    connect(sendBut,&QPushButton::clicked,this,&UdpTest::threadTest);
}

UdpTest::~UdpTest(){

}

void UdpTest::threadTest(){
    qDebug("线程测试");
    task->start();
    qDebug("线程测试end");
}

void UdpTest::appendTextSlot(QString str){
    infoText->append(str);
}
