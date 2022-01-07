#ifndef COMPLETIONPORTTEST_H
#define COMPLETIONPORTTEST_H

#include <QWidget>
#include <QDebug>
#include <windows.h>
#include <winsock2.h>
#include <vector>
#include <QString>
#include <mswsock.h>
#include <QDebug>
#include <ntdef.h>
#include <QPushButton>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>

// 缓冲区长度 (1024*8)
#define MAX_BUFFER_LEN 8192
// 默认端口
#define DEFAULT_PORT 1000
// 默认IP地址
#define DEFAULT_IP ("127.0.0.1")

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

class CIOCPModel{
public:
    CIOCPModel();
    ~CIOCPModel();

    bool Start();//启动服务器
    void Stop();//停止服务器
    bool LoadSocketLib();//加载Socket库
    void UnloadSocketLib(){WSACleanup();}//卸载Socket库
    QString GetLocalIP();//获取本机ip
    void SetPort(const int& nPort){m_nPort = nPort;}

protected:
    bool _InitializeIOCP();//init iocp
    bool _InitializeListenSocket();//init socket
    void _DeInitialize();// release
    bool _PostAccept(PER_IO_CONTEXT* pAcceptIoContext);//投递accept请求
    bool _PostRecv(PER_IO_CONTEXT* ploContext);//投递接收数据请求
    bool _PostSend(PER_IO_CONTEXT* ploContext);
    bool _DoAccept(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext);//客户端连入处理
    bool _DoRecv(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext);//数据到达处理
    void _AddToContextList(PER_SOCKET_CONTEXT* pSocketContext);//客户端信息存储到数组中
    void _RemoveContext(PER_SOCKET_CONTEXT* pSocketContext);//客户端信息从数组中移除
    void _ClearContextList();//清空客户端信息
    bool _AssociateWithIOCP(PER_SOCKET_CONTEXT* pContext);//句柄绑定到完成端口中
    bool HandleError(PER_SOCKET_CONTEXT* pContext, const DWORD& dwErr);//处理完成端口上的错误
    static DWORD WINAPI _WorkerThread(LPVOID lpParam);//线程函数
    int _GetNoOfProcessors();//本机处理器数量
    bool _IsSocketAlive(SOCKET s);//判断socket是否已断开

private:
    HANDLE m_hShutdownEvent;//通知线程系统退出的事件，为了更好的退出线程
    HANDLE m_hIOCompletionPort;//完成端口的句柄
    HANDLE* m_phWorkerThreads;//工作者线程的句柄指针
    int m_nThreads; //线程数量
    QString m_strIP;//服务器端的ip
    int m_nPort;//服务器端的监听端口
    CRITICAL_SECTION m_csContextList;//worker线程同步互斥量
    std::vector<PER_SOCKET_CONTEXT*> m_arrayClientContext;//客户端socket的context
    PER_SOCKET_CONTEXT* m_pListenContext;//用于监听的socket的context信息
    LPFN_ACCEPTEX m_lpfnAcceptEx;//AcceptEx和GetAcceptExSockaddrs的函数指针，
    //用于调用这两个扩展函数
    LPFN_GETACCEPTEXSOCKADDRS m_lpfnGetAcceptExSockAddrs;
};

typedef struct _tagThreadParams_WORKER{
    CIOCPModel* pIOCPModel;//类指针，用于调用类中的函数
    int nThreadNo;//线程编号
}THREADPARAMS_WORKER, *PTHREADPARAM_WORKER;

class CompletionPortTest : public QWidget
{
    Q_OBJECT
public:
    explicit CompletionPortTest(QWidget *parent = nullptr);
    ~CompletionPortTest();
private:
    QPushButton* but1;
    QPushButton* but2;
    QTextEdit* text1;
    QVBoxLayout* layout1;
    QHBoxLayout* layout2;
    CIOCPModel m_IOCP;//完成端口模型

signals:

public slots:
    void appendTextSlot(QString str);
    void but1OnClick();
    void but2OnClick();
};

#endif // COMPLETIONPORTTEST_H
