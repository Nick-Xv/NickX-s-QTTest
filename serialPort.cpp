#include "serialport.h"

#include <QDebug>
#include <QLayout>


SerialPort::SerialPort(QWidget* parent):
    QWidget(parent){
    m_serialPort = new QSerialPort();
    m_serialPort2 = new QSerialPort();
    this->setGeometry(0,0,500,400);
    m_OpenPortButton = new QPushButton(this);
    m_WriteDataButton = new QPushButton(this);
    m_SendDataText = new QLineEdit(this);
    m_SendDataText->setGeometry(50,150,300,50);
    m_WriteDataButton->setGeometry(50,100,100,50);
    m_WriteDataButton->setText("send data");
    m_OpenPortButton->setGeometry(50,50,100,50);
    m_OpenPortButton->setText("open port");
    //信号槽
    connect(m_OpenPortButton,&QPushButton::clicked,this,&SerialPort::openPort);
    connect(m_WriteDataButton,&QPushButton::clicked,this,&SerialPort::writeBytes);
}

SerialPort::~SerialPort(){
    if(m_serialPort->isOpen()){
        m_serialPort->close();
    }
    if(m_serialPort2->isOpen()){
        m_serialPort2->close();
    }
    delete m_serialPort;
    delete m_serialPort2;
}


QStringList SerialPort::getPortNameList(){
    QStringList m_serialPortName;
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        m_serialPortName << info.portName();
        qDebug()<<"serialPortName:"<<info.portName();
    }
    return m_serialPortName;
}

void SerialPort::openPort(){
    if(m_serialPort->isOpen()){
        m_serialPort->clear();
        m_serialPort->close();
    }
    if(m_serialPort2->isOpen()){
        m_serialPort2->clear();
        m_serialPort2->close();
    }
    m_portNameList = getPortNameList();
    m_serialPort->setPortName(m_portNameList[0]);
    m_serialPort2->setPortName(m_portNameList[1]);

    if(!m_serialPort->open(QIODevice::ReadWrite)){
        qDebug()<<m_portNameList[0]<<"open failed!!";
        return;
    }
    if(!m_serialPort2->open(QIODevice::ReadWrite)){
        qDebug()<<m_portNameList[1]<<"open failed!!";
        return;
    }
    //m_serialPort->setDataTerminalReady(true);
    qDebug()<<"串口打开成功!";
    //打开成功
    //波特率和读写方向
    m_serialPort->setBaudRate(QSerialPort::Baud115200, QSerialPort::AllDirections);
    m_serialPort2->setBaudRate(QSerialPort::Baud115200, QSerialPort::AllDirections);
    //数据位8位
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort2->setDataBits(QSerialPort::Data8);
    //无流控制
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
    m_serialPort2->setFlowControl(QSerialPort::NoFlowControl);
    //无校验位
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort2->setParity(QSerialPort::NoParity);
    //一位停止位
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort2->setStopBits(QSerialPort::OneStop);

    connect(m_serialPort2,&QSerialPort::readyRead,this,&SerialPort::receiveInfo);

}

void SerialPort::receiveInfo(){
    qDebug()<<"receive info!!!!!!!!!!!";
    QByteArray info = m_serialPort2->readAll();

    qDebug()<<"receive info:"<<info;
}

void SerialPort::writeBytes(){
    QString str = m_SendDataText->text();
    if(str.length()<1){
        return;
    }
    QByteArray Data = str.toLatin1();
    if(m_serialPort->isOpen()){
        m_serialPort->write(Data);
        qDebug()<<Data;
    }else{
        qDebug()<<"串口未打开!";
    }
}
