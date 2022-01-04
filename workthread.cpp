#include "workthread.h"
#include <QDebug>
#include "udptest.h"

WorkThread::WorkThread()
{

}

WorkThread::~WorkThread(){
    delete dataBuf;
    //关闭socket连接
    nResult = closesocket(s);
    if(nResult == SOCKET_ERROR){
        qDebug("closesocket failed:%d\n", WSAGetLastError());
        return;
    }
    //清理socket
    WSACleanup();
}

void WorkThread::run(){
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    qDebug("recvTest");
    //init
    uPort = 1000;
    dataBuf = new char[1024];
    memset(dataBuf, 0, sizeof(char) * 1024);
    nBufLen = 1024;
    nResult = 0;
    nSenderAddrSize = sizeof(sSendAddr);

    nResult = WSAStartup(MAKEWORD(2,2), &wsd);//初始化Socket2.2版本
    if(nResult != NO_ERROR){
        qDebug("WSAStartup failed:%d\n", WSAGetLastError());
        tempstr = "WSAStartup failed:";
        emit appendText(tempstr + QString::number(WSAGetLastError()) + static_cast<QString>("\n"));
        return;
    }

    int imode = 1;//非阻塞模式？

    //创建一个Socket,SOCK_DGRAM表示UDP类型
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(s == INVALID_SOCKET){
        qDebug("socket failed:%d\n", WSAGetLastError());
        tempstr = "socket failed:";
        emit appendText(tempstr + QString::number(WSAGetLastError()) + static_cast<QString>("\n"));
        return;
    }

    nResult = ioctlsocket(s, static_cast<long>(FIONBIO), reinterpret_cast<u_long*>(&imode));
    if(nResult == SOCKET_ERROR){
        qDebug("ioctlsocket failed");
        closesocket(s);
        WSACleanup();
        return;
    }

    //填充Socket接口
    sRecvAddr.sin_family = AF_INET;//地址协议
    sRecvAddr.sin_port = htons(uPort);
    sRecvAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//接收任意地址数据

    //绑定socket至本机
    nResult = bind(s, reinterpret_cast<SOCKADDR*>(&sRecvAddr), sizeof(sRecvAddr));
    if(nResult != 0){
        qDebug("bind failed:%d\n", WSAGetLastError());
        tempstr = "bind failed:";
        emit appendText(tempstr + QString::number(WSAGetLastError()) + static_cast<QString>("\n"));
        return;
    }

    qDebug("Waiting recv data...\n");
    //非阻塞式接收数据
    while(1){
        //udp数据接收
        FD_ZERO(&rfd);//清空描述符集
        FD_SET(s,&rfd);//将s放入要测试的描述符集
        SelectRcv = select(s+1,&rfd,0,0,&timeout);//检查该套接字是否可读
        if(SelectRcv<0){
            qDebug("监听失败");
        }
        if(SelectRcv>0){
            memset(dataBuf, 0, sizeof(char) * 1024);
            nResult = recvfrom(s,dataBuf,nBufLen,0,reinterpret_cast<SOCKADDR*>(&sRecvAddr),
                               &nSenderAddrSize);
            if(nResult == SOCKET_ERROR){
                qDebug("recvfrom failed:%d\n", WSAGetLastError());
            }
            else{
                qDebug("SenderIP  :%s\n", inet_ntoa(sSendAddr.sin_addr));
                qDebug("SenderData:%s\n", dataBuf);
                tempstr = "SenderIP  :";
                emit appendText(tempstr + inet_ntoa(sSendAddr.sin_addr) + static_cast<QString>("\n"));
                tempstr = "SenderData:";
                emit appendText(tempstr + dataBuf + static_cast<QString>("\n"));
            }
        }
    }
}
