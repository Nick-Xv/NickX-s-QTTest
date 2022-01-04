#ifndef WORKTHREAD_H
#define WORKTHREAD_H
#include <QThread>
#include <winsock2.h>

class WorkThread : public QThread
{
    Q_OBJECT
public:
    WorkThread();
    ~WorkThread();
private:
    virtual void run();
    WSADATA wsd;//存放windows socket初始化信息
    SOCKET s;//通信socket句柄
    SOCKADDR_IN sRecvAddr, sSendAddr;
    USHORT uPort;//通信端口
    char* dataBuf;//通信数据缓冲区
    int nBufLen, nResult, nSenderAddrSize;

    fd_set rfd;//描述符集，用来测试有没有一个可用的连接
    struct timeval timeout;
    int SelectRcv;

    QString tempstr;
signals:
    void appendText(QString str);
};

#endif // WORKTHREAD_H
