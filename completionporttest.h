#ifndef COMPLETIONPORTTEST_H
#define COMPLETIONPORTTEST_H

#include <QWidget>
#include <QDebug>
#include <windows.h>
#include <winsock2.h>
#include <vector>

// 缓冲区长度 (1024*8)
#define MAX_BUFFER_LEN 8192
// 默认端口
#define DEFAULT_PORT 1000
// 默认IP地址
#define DEFAULT_IP _T("127.0.0.1")

// 在完成端口上投递的I/O操作的类型
typedef enum _OPERATION_TYPE{
    ACCEPT_POSTED,//accept
    SEND_POSTED,//send
    RECV_POSTED,//recv
    NULL_POSTED//init
}OPERATION_TYPE;

//单IO数据结构体定义(用于每一个重叠操作的参数)
typedef struct _PER_IO_CONTEXT{
    OVERLAPPED m_Overlapped;//每个socket的每个操作都要有一个
    SOCKET m_sockAccept;//该网络操作所使用的SOCKET
    WSABUF m_wsaBuf;//WSA类型的缓冲区，用于给重叠操作传参？
    char m_szBuffer[MAX_BUFFER_LEN];//WSABUF里具体存字符的缓冲区
    OPERATION_TYPE m_OpType;//网络操作类型的标识

    //init
    _PER_IO_CONTEXT(){
        ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
        ZeroMemory(m_szBuffer, MAX_BUFFER_LEN);
        m_sockAccept = INVALID_SOCKET;
        m_wsaBuf.buf = m_szBuffer;
        m_wsaBuf.len = MAX_BUFFER_LEN;
        m_OpType = NULL_POSTED;
    }

    //析构
    ~_PER_IO_CONTEXT(){
        if(m_sockAccept!=INVALID_SOCKET){
            closesocket(m_sockAccept);
            m_sockAccept = INVALID_SOCKET;
        }
    }

    //重置缓冲区
    void ResetBuffer(){
        ZeroMemory(m_szBuffer, MAX_BUFFER_LEN);
    }
}PER_IO_CONTEXT, *PPER_IO_CONTEXT;

//单句柄数据结构体定义(用于每一个完成端口，也就是每一个Socket的参数)
typedef struct _PER_SOCKET_CONTEXT{
    SOCKET m_Socket;//每个客户端连接的socket
    SOCKADDR_IN m_ClientAddr;//客户端地址
    std::vector<_PER_IO_CONTEXT*> m_arrayIoContext;//客户端网络操作的上下文数据
    //对于每一个客户端Socket，是可以在上面同时投递多个IO请求的

    //init
    _PER_SOCKET_CONTEXT(){
        m_Socket = INVALID_SOCKET;
        memset(&m_ClientAddr, 0, sizeof(m_ClientAddr));
    }

    //析构
    ~_PER_SOCKET_CONTEXT(){
        if(m_Socket != INVALID_SOCKET){
            closesocket(m_Socket);
            m_Socket = INVALID_SOCKET;
        }
        //释放所有IO上下文数据
        m_arrayIoContext.clear();
        std::vector<_PER_IO_CONTEXT*>(m_arrayIoContext).swap(m_arrayIoContext);
    }

    //获取一个新的IoContext
    _PER_IO_CONTEXT* GetNewIoContext(){
        _PER_IO_CONTEXT* p = new _PER_IO_CONTEXT;
        m_arrayIoContext.push_back(p);
        return p;
    }

    //移除一个指定IoContext
    void RemoveContext(_PER_IO_CONTEXT* pContext){
        for(std::vector<_PER_IO_CONTEXT*>::iterator it = m_arrayIoContext.begin(); it!=m_arrayIoContext.end();){
            if(*it == pContext){
                it = m_arrayIoContext.erase(it);
                delete pContext;
                pContext = nullptr;
                break;
            }else{
                ++it;
            }
        }
    }
}PER_SOCKET_CONTEXT, *PPER_SOCKET_CONTEXT;

class CompletionPortTest : public QWidget
{
    Q_OBJECT
public:
    explicit CompletionPortTest(QWidget *parent = nullptr);
    ~CompletionPortTest();

signals:

public slots:
};

#endif // COMPLETIONPORTTEST_H
