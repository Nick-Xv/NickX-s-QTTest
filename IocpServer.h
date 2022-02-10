#pragma once
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <vector>
#include <mswsock.h>
#include <QDebug>
#include <QMetaType>
#include <malloc.h>
#include "mysqlhandler.h"
using std::string;
using std::vector;

extern enum SERVICE_TYPE;

const int MAX_BUFFER_LEN1 = 8192;//缓冲区长度
const int BUFFER_TEMP_LEN1 = 50;
const int DEFAULT_PORT1 = 1000;//默认端口

//const string DEFAULT_IP1 = "127.0.0.1";//默认IP地址

// 在完成端口上投递的I/O操作的类型
typedef enum _OPERATION_TYPE1 {
	//tcp
	ACCEPT_POST,//accept
	SEND_POST,//send
	RECV_POST,//recv
	//udp
	SENDTO_POST,//sendto
	RECVFROM_POST,//recvfrom

	NULL_POST//init
}OPERATION_TYPE1;

//单IO数据结构体定义(用于每一个重叠操作的参数)
typedef struct _PER_IO_CONTEXT1 {
	OVERLAPPED m_Overlapped;//每个socket的每个操作都要有一个
	SOCKET m_sockAccept; //该网络操作所使用的SOCKET
	WSABUF m_wsaBuf;//WSA类型的缓冲区，用于给重叠操作传参？
	char m_szBuffer[MAX_BUFFER_LEN1];//WSABUF里具体存字符的缓冲区
	OPERATION_TYPE1 m_OpType; //网络操作类型的标识

	//udp
	unsigned long recvBytes;//存储接收到的字节数
	SOCKADDR_IN remoteAddr; //存储数据来源IP地址
	int remoteAddrLen;      //存储数据来源IP地址长度

	//init
	_PER_IO_CONTEXT1() {
		ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
		ZeroMemory(m_szBuffer, MAX_BUFFER_LEN1);
		m_sockAccept = INVALID_SOCKET;
		m_wsaBuf.buf = m_szBuffer;
		m_wsaBuf.len = MAX_BUFFER_LEN1;
		m_OpType = NULL_POST;
		//udp
		recvBytes = 0;
		//ZeroMemory(&remoteAddr, sizeof(remoteAddr));
		remoteAddrLen = sizeof(remoteAddr);
	}

	//析构
	~_PER_IO_CONTEXT1() {
		if (m_sockAccept != INVALID_SOCKET) {
			closesocket(m_sockAccept);
			m_sockAccept = INVALID_SOCKET;
		}
	}

	//重置缓冲区
	void ResetBuffer() {
		ZeroMemory(m_szBuffer, MAX_BUFFER_LEN1);
	}
}PER_IO_CONTEXT1, *PPER_IO_CONTEXT1;

//单句柄数据结构体定义(每一个Socket的参数)
typedef struct _PER_SOCKET_CONTEXT1 {
	SOCKET m_Socket;//每个客户端连接的socket
	SOCKADDR_IN m_ClientAddr;//客户端地址
	vector<_PER_IO_CONTEXT1*> m_arrayIoContext;//客户端网络操作的上下文数据
	//init
	_PER_SOCKET_CONTEXT1() {
		m_Socket = INVALID_SOCKET;
		memset(&m_ClientAddr, 0, sizeof(m_ClientAddr));
	}

	//析构
	~_PER_SOCKET_CONTEXT1() {
		if (m_Socket != INVALID_SOCKET) {
			closesocket(m_Socket);
			m_Socket = INVALID_SOCKET;
		}
		//释放所有IO上下文数据
		m_arrayIoContext.clear();
		std::vector<_PER_IO_CONTEXT1*>(m_arrayIoContext).swap(m_arrayIoContext);
	}

	//获取一个新的IoContext
	_PER_IO_CONTEXT1* GetNewIoContext() {
		_PER_IO_CONTEXT1* p = new _PER_IO_CONTEXT1;
		m_arrayIoContext.push_back(p);
		return p;
	}

	//移除一个指定IoContext
	void RemoveContext(_PER_IO_CONTEXT1* pContext) {
		for (std::vector<_PER_IO_CONTEXT1*>::iterator it = m_arrayIoContext.begin(); it != m_arrayIoContext.end();) {
			if (*it == pContext) {
				it = m_arrayIoContext.erase(it);
				delete pContext;
				pContext = nullptr;
				break;
			}
			else {
				++it;
			}
		}
	}
}PER_SOCKET_CONTEXT1, *PPER_SOCKET_CONTEXT1;

class IocpServer : public QObject
{
	Q_OBJECT
public:
	IocpServer();
	~IocpServer();
	bool serverStart();
	void serverStop();
	void SendDataTo(PER_IO_CONTEXT1* pIoContext);//发送数据

	class CIOCPModel1 {
	public:
		CIOCPModel1(IocpServer* parent);
		~CIOCPModel1();

		bool Start();//启动服务器
		void Stop();//停止服务器
		bool LoadSocketLib();//加载Socket库
		void UnloadSocketLib() { WSACleanup(); }//卸载Socket库
												//string GetLocalIP();//获取本机ip
		void SetPort(const int& nPort) { m_nPort = nPort; }
		static int _GetNoOfProcessors();//本机处理器数量

		void SendDataTo(PER_IO_CONTEXT1* pIoContext);//发送数据

	protected:
		bool _InitializeIOCP();//init iocp
		bool _InitializeListenSocket();//init socket
		void _DeInitialize();// release
		bool _PostAccept(PER_IO_CONTEXT1* pAcceptIoContext);//投递accept请求
		bool _PostRecv(PER_IO_CONTEXT1* pIoContext);//投递接收数据请求
		bool _PostSend(PER_IO_CONTEXT1* pIoContext);//投递发送tcp数据请求

		bool _PostRecvFrom(PER_IO_CONTEXT1* pIoContext);//投递udp接收数据请求
		bool _DoRecvFrom(PER_IO_CONTEXT1* pIoContext, int threadNo, int curBufNo, MySqlHandler* mysqlHandler);//处理udp数据到达请求
		bool _PostSendTo(PER_IO_CONTEXT1* pIoContext);//投递发送udp数据请求

		bool _DoAccept(PER_SOCKET_CONTEXT1* pSocketContext, PER_IO_CONTEXT1* pIoContext);//客户端连入处理
		bool _DoRecv(PER_SOCKET_CONTEXT1* pSocketContext, PER_IO_CONTEXT1* pIoContext);//数据到达处理
		void _AddToContextList(PER_SOCKET_CONTEXT1* pSocketContext);//客户端信息存储到数组中
		void _RemoveContext(PER_SOCKET_CONTEXT1* pSocketContext);//客户端信息从数组中移除
		void _RemoveADDR(IN_ADDR inaddr);//移除特定udp客户端地址
		void _ClearContextList();//清空客户端信息
		bool _AssociateWithIOCP(PER_SOCKET_CONTEXT1* pContext);//句柄绑定到完成端口中
		bool HandleError(PER_SOCKET_CONTEXT1* pContext, const DWORD& dwErr);//处理完成端口上的错误
		static DWORD WINAPI _WorkerThread(LPVOID lpParam);//线程函数
		bool _IsSocketAlive(SOCKET s);//判断socket是否已断开

	private:
		HANDLE m_hShutdownEvent;//通知线程系统退出的事件，为了更好的退出线程
		HANDLE m_hIOCompletionPort;//完成端口的句柄
		HANDLE* m_phWorkerThreads;//工作者线程的句柄指针
		int m_nThreads; //线程数量
						//string m_strIP;//服务器端的ip
		int m_nPort;//服务器端的监听端口
		CRITICAL_SECTION m_csContextList;//worker线程同步互斥量
		vector<PER_SOCKET_CONTEXT1*> m_arrayClientContext;//客户端socket的context
		PER_SOCKET_CONTEXT1* m_pListenContext;//用于监听的socket的context信息
		PER_SOCKET_CONTEXT1* m_pListenContextUdp;//测试：udpsocket
		LPFN_ACCEPTEX m_lpfnAcceptEx;//AcceptEx和GetAcceptExSockaddrs的函数指针，
									 //用于调用这两个扩展函数
		LPFN_GETACCEPTEXSOCKADDRS m_lpfnGetAcceptExSockAddrs;

		//存储udp客户的地址列表
		vector<IN_ADDR> m_arrayUdpClientContext;

		//指向外部类的指针
		IocpServer* parent;

		//缓冲区三维数组指针
		char*** bufferPtr;

		//数据库连接指针
		MySqlHandler** mysqlHandler;
	};
signals:
	void serviceHandler(PER_IO_CONTEXT1* pIoContext, char* buff, MySqlHandler* mysqlHandler);
private:
	CIOCPModel1* m_IOCP;//完成端口模型
};

typedef struct _tagThreadParams_WORKER1 {
	IocpServer::CIOCPModel1* pIOCPModel;//类指针，用于调用类中的函数
	int nThreadNo;//线程编号
	MySqlHandler* mysqlHandler;
}THREADPARAMS_WORKER1, *PTHREADPARAM_WORKER1;
