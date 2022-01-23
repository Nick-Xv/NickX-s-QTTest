#include "tcptest.h"

TcpTest::TcpTest(QWidget* parent):QWidget(parent)
{
    testLayout = new QVBoxLayout(this);
    this->setLayout(testLayout);
    this->setGeometry(0,0,500,550);
    sendButton = new QPushButton(this);
    inputText = new QTextEdit(this);
    infoText = new QTextEdit(this);
    sendButton->setGeometry(50,50,100,50);
    inputText->setGeometry(50,100,400,200);
    infoText->setGeometry(50,300,400,200);
    sendButton->setText("发送");
    infoText->setReadOnly(true);

    testLayout->setDirection(QBoxLayout::TopToBottom);
    testLayout->addWidget(sendButton,1);
    testLayout->addWidget(inputText,4);
    testLayout->addWidget(infoText,4);

    tcpServer = nullptr;
    tcpSocket = nullptr;

    setWindowTitle("服务器");
    tcpServer = new QTcpServer(this);
    tcpServer->listen(QHostAddress::Any,666);
    connect(tcpServer,&QTcpServer::newConnection,
            [=](){
                tcpSocket = tcpServer->nextPendingConnection();
                QString ip = tcpSocket->peerAddress().toString();
                quint16 port = tcpSocket->peerPort();
                QString temp = QString("[%1:%2]:成功连接").arg(ip).arg(port);
                infoText->setText(temp);

                connect(tcpSocket,&QTcpSocket::readyRead,
                        [=]()
                {
                    QByteArray array = tcpSocket->readAll();
                    infoText->append(array);
                });
            }
    );

    connect(sendButton,&QPushButton::clicked,this,&TcpTest::sendData);
}

TcpTest::~TcpTest(){
    if(tcpSocket == nullptr){
        return;
    }
    tcpSocket->disconnectFromHost();
    tcpSocket->close();
    tcpServer->close();

}

void TcpTest::sendData(){
    if(tcpSocket == nullptr){
        return;
    }
    QString str = inputText->toPlainText();
    tcpSocket->write(str.toUtf8().data());
}
