#include "IocpServer.h"

//每个处理器上产生多少个线程()
const int WORKER_THREADS_PER_PROCESSOR1 = 2;
//计算线程数量
const int THREAD_NUMBER = WORKER_THREADS_PER_PROCESSOR1 * IocpServer::CIOCPModel1::_GetNoOfProcessors() + 2;
//同时投递的accept请求的数量(灵活设置)
const int MAX_POST_ACCEPT1 = (THREAD_NUMBER-2) / 2;
//传递给Worker线程的退出信号
#define EXIT_CODE NULL

//释放指针和句柄资源的宏
//释放指针宏
#define RELEASE(x) {if((x)!=nullptr){delete(x);(x)=nullptr;}}
//释放句柄宏
#define RELEASE_HANDLE(x) {if((x)!=nullptr&&(x)!=INVALID_HANDLE_VALUE){CloseHandle(x);(x)=nullptr;}}
//释放Socket宏
#define RELEASE_SOCKET(x) {if((x)!=INVALID_SOCKET){closesocket(x);x=INVALID_SOCKET;}}

char*** createBufferArray() {
	char*** arr;
	arr = new char**[THREAD_NUMBER];
	//分配空间
	for (int i = 0; i < THREAD_NUMBER; i++) {
		arr[i] = new char*[BUFFER_TEMP_LEN1];
		for (int j = 0; j < BUFFER_TEMP_LEN1; j++) {
			arr[i][j] = new char[MAX_BUFFER_LEN1];
		}
	}
	//初始化
	for (int i = 0; i < THREAD_NUMBER; i++) {
		for (int j = 0; j < BUFFER_TEMP_LEN1; j++) {
			for (int k = 0; k < MAX_BUFFER_LEN1; k++) {
				*(*(*(arr + i) + j) + k) = 0;
			}
		}
	}
	return arr;
}

void deleteBufferArray(char*** arr) {
	for (int i = 0; i < THREAD_NUMBER; i++) {
		for (int j = 0; j < BUFFER_TEMP_LEN1; j++) {
			delete[]arr[i][j];
		}
	}
	for (int i = 0; i < THREAD_NUMBER; i++) {
		delete[]arr[i];
	}
	delete[]arr;
}

IocpServer::CIOCPModel1::CIOCPModel1(IocpServer* parent) :
	m_nThreads(THREAD_NUMBER),
	m_hShutdownEvent(nullptr),
	m_hIOCompletionPort(nullptr),
	m_phWorkerThreads(nullptr),
	//m_strIP(DEFAULT_IP1),
	m_nPort(DEFAULT_PORT1),
	m_lpfnAcceptEx(nullptr),
	m_pListenContext(nullptr),
	m_pListenContextUdp(nullptr)
{
	this->parent = parent;
	qRegisterMetaType<SERVICE_TYPE>("SERVICE_TYPE");
	qRegisterMetaType<PER_IO_CONTEXT1>("PER_IO_CONTEXT1");
	qRegisterMetaType<WSABUF>("WSABUF");
	bufferPtr = createBufferArray();
}

IocpServer::CIOCPModel1::~CIOCPModel1()
{
	// 确保资源彻底释放
	this->Stop();
	deleteBufferArray(bufferPtr);
}

//WorkerThread
DWORD WINAPI IocpServer::CIOCPModel1::_WorkerThread(LPVOID lpParam) {
	THREADPARAMS_WORKER1* pParam = static_cast<THREADPARAMS_WORKER1*>(lpParam);
	CIOCPModel1* pIOCPModel = static_cast<CIOCPModel1*>(pParam->pIOCPModel);
	//线程编号
	int nThreadNo = static_cast<int>(pParam->nThreadNo);
	//当前缓冲区编号
	int curBufNo = 0;
	//显示信息
	qDebug() << "workerthread start " << nThreadNo << endl;

	OVERLAPPED* pOverlapped = nullptr;
	PER_SOCKET_CONTEXT1* pSocketContext = nullptr;
	DWORD dwBytesTransfered = 0;

	//每个线程一个数据库连接
	MySqlHandler* mysqlHandler = pParam->mysqlHandler;

	//循环处理请求，直到接收到shutdown信息为止
	while (WAIT_OBJECT_0 != WaitForSingleObject(pIOCPModel->m_hShutdownEvent, 0)) {
		BOOL bReturn = GetQueuedCompletionStatus(
			pIOCPModel->m_hIOCompletionPort,
			&dwBytesTransfered,
			reinterpret_cast<PULONG_PTR>(&pSocketContext),
			&pOverlapped,
			INFINITE
		);
		//收到退出标志则退出
		if (EXIT_CODE == reinterpret_cast<ULONG_PTR>(pSocketContext))break;
		//判断是否出现错误
		if (!bReturn) {
			DWORD dwErr = GetLastError();

			//显示信息
			if (!pIOCPModel->HandleError(pSocketContext, dwErr))break;
			continue;
		}
		else {
			//读取传入的参数
			PER_IO_CONTEXT1* pIoContext = CONTAINING_RECORD(pOverlapped, PER_IO_CONTEXT1, m_Overlapped);
			//qDebug() << pIoContext->m_OpType << endl;
			//判断是否有客户端断开了
			if ((0 == dwBytesTransfered) && (RECV_POST == pIoContext->m_OpType || SEND_POST == pIoContext->m_OpType))
			{
				//显示信息
				qDebug() << "客户端 " << inet_ntoa(pSocketContext->m_ClientAddr.sin_addr) << ":" << ntohs(pSocketContext->m_ClientAddr.sin_port) << "断开连接." << endl;
				//释放资源
				pIOCPModel->_RemoveContext(pSocketContext);
				continue;
			}
			else {
				//拷贝当前缓冲区内容到三维数组特定位置
				memcpy(pIOCPModel->bufferPtr[nThreadNo-1][curBufNo], pIoContext->m_szBuffer, MAX_BUFFER_LEN1);
				switch (pIoContext->m_OpType) {
					//accept
				case ACCEPT_POST: {
					pIOCPModel->_DoAccept(pSocketContext, pIoContext);
				}
				break;
				case RECV_POST: {
					pIOCPModel->_DoRecv(pSocketContext, pIoContext);
				}
				break;
				case SEND_POST: {
					qDebug()<<"SEND 数据发出"<<endl;
				}
				break;
				case RECVFROM_POST: {
					pIOCPModel->_DoRecvFrom(pIoContext, nThreadNo-1, curBufNo, mysqlHandler);
				}
				break;
				case SENDTO_POST: {
					qDebug() << "SEND_TO 数据发出" << endl;
				}
				break;
				default:
				qDebug() << "_WorkThread中的 pIoContext->m_OpType 参数异常" << endl;
				break;
				}
				//当前缓冲区第二维指针+1
				curBufNo = (curBufNo + 1) % BUFFER_TEMP_LEN1;
			}
		}
	}
	//退出前关闭数据库连接
	mysqlHandler->closeDb();
	qDebug() << "工作者线程 " << nThreadNo << "号退出 " << endl;
	RELEASE(lpParam);
	return 0;
}

//init winsock 2.2
bool IocpServer::CIOCPModel1::LoadSocketLib() {
	WSADATA wsaData;
	int nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (NO_ERROR != nResult) {
		qDebug() << "初始化WinSock 2.2失败" << endl;
		return false;
	}
	return true;
}

bool IocpServer::CIOCPModel1::Start() {
	//初始化线程互斥量
	InitializeCriticalSection(&m_csContextList);
	//建立系统退出的事件通知
	m_hShutdownEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	//init IOCP
	if (false == _InitializeIOCP()) {
		qDebug() << "IOCP init fail" << endl;
		return false;
	}
	else {
		qDebug() << "IOCP init complete" << endl;
	}
	//init socket
	if (false == _InitializeListenSocket()) {
		qDebug() << "init listen socket fail" << endl;
		this->_DeInitialize();
		return false;
	}
	else {
		qDebug() << "listen socket init complete" << endl;
	}
	qDebug() << "system init complete, waiting for connection" << endl;
	return true;
}

//发送系统退出消息，退出完成端口和线程资源
void IocpServer::CIOCPModel1::Stop() {
	if (m_pListenContext != nullptr&&m_pListenContext->m_Socket != INVALID_SOCKET) {
		// 激活关闭消息通知
		SetEvent(m_hShutdownEvent);
		for (int i = 0; i<m_nThreads; i++) {
			// 通知所有的完成端口操作退出
			PostQueuedCompletionStatus(m_hIOCompletionPort, 0, static_cast<DWORD>(EXIT_CODE), nullptr);
		}
		//等待所有的客户端资源退出
		WaitForMultipleObjects(m_nThreads, m_phWorkerThreads, TRUE, INFINITE);
		this->_ClearContextList();
		this->_DeInitialize();
		qDebug() << "停止监听" << endl;
	}
	if (m_pListenContextUdp != nullptr&&m_pListenContextUdp->m_Socket != INVALID_SOCKET) {
		// 激活关闭消息通知
		SetEvent(m_hShutdownEvent);
		for (int i = 0; i<m_nThreads; i++) {
			// 通知所有的完成端口操作退出
			PostQueuedCompletionStatus(m_hIOCompletionPort, 0, static_cast<DWORD>(EXIT_CODE), nullptr);
		}
		//等待所有的客户端资源退出
		WaitForMultipleObjects(m_nThreads, m_phWorkerThreads, TRUE, INFINITE);
		this->_ClearContextList();
		this->_DeInitialize();
		qDebug() << "停止监听" << endl;
	}
}

// 初始化完成端口
bool IocpServer::CIOCPModel1::_InitializeIOCP()
{
	// 建立第一个完成端口
	m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

	if (nullptr == m_hIOCompletionPort)
	{
		qDebug() << "建立完成端口失败！错误代码: " << WSAGetLastError() << endl;
		return false;
	}

	// 为工作者线程初始化句柄
	m_phWorkerThreads = new HANDLE[m_nThreads];

	//先建立每个线程的数据库连接
	mysqlHandler = new MySqlHandler*[m_nThreads];
	for (int i = 0; i < m_nThreads; i++) {
		mysqlHandler[i] = new MySqlHandler();
		if (mysqlHandler[i]->connectDb()) {
			qDebug() << "数据库连接成功" << endl;
		}
		else {
			qDebug() << "数据库连接失败" << endl;
		}
	}

	// 根据计算出来的数量建立工作者线程
	DWORD nThreadID;
	for (int i = 0; i < m_nThreads; i++)
	{
		THREADPARAMS_WORKER1* pThreadParams = new THREADPARAMS_WORKER1;
		pThreadParams->pIOCPModel = this;
		pThreadParams->nThreadNo = i + 1;
		pThreadParams->mysqlHandler = mysqlHandler[i];
		m_phWorkerThreads[i] = ::CreateThread(0, 0, _WorkerThread, (void *)pThreadParams, 0, &nThreadID);
	}

	qDebug() << "建立 _WorkerThread " << m_nThreads << "个." << endl;

	return true;
}

//init socket
bool IocpServer::CIOCPModel1::_InitializeListenSocket() {
	//AcceptEx and GetAcceptExSockaddrs GUID，用于导出函数指针
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;

	//服务器地址信息，绑定socket
	struct sockaddr_in ServerAddress;

	//生成用于监听的socket的信息
	m_pListenContext = new PER_SOCKET_CONTEXT1;
	m_pListenContextUdp = new PER_SOCKET_CONTEXT1;

	//需要使用重叠IO，必须得使用WSASocket来建立socket
	m_pListenContext->m_Socket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_pListenContext->m_Socket)
	{
		qDebug() << "初始化Socket失败，错误代码: " << WSAGetLastError() << endl;
		return false;
	}
	else
	{
		qDebug() << "WSASocket() 完成" << endl;
	}
	//使用WSASocket建立udp socket
	m_pListenContextUdp->m_Socket = WSASocket(AF_INET, SOCK_DGRAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_pListenContextUdp->m_Socket)
	{
		qDebug() << "初始化Udp Socket失败，错误代码: " << WSAGetLastError() << endl;
		return false;
	}
	else
	{
		qDebug() << "Udp WSASocket() 完成" << endl;
	}

	//将Listen Socket绑定至完成端口中
	if (nullptr == CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_pListenContext->m_Socket), m_hIOCompletionPort, reinterpret_cast<ULONG_PTR>(m_pListenContext), 0))
	{
		qDebug() << "绑定 Listen Socket至完成端口失败！错误代码: " << WSAGetLastError() << endl;
		RELEASE_SOCKET(m_pListenContext->m_Socket);
		return false;
	}
	else
	{
		qDebug() << "Listen Socket绑定完成端口 完成." << endl;
	}

	//填充地址信息
	ZeroMemory((char *)&ServerAddress, sizeof(ServerAddress));
	ServerAddress.sin_family = AF_INET;
	//这里可以绑定任何可用的IP地址，或者绑定一个指定的IP地址
	ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	ServerAddress.sin_port = htons(m_nPort);

	// 绑定地址和端口
	if (SOCKET_ERROR == bind(m_pListenContext->m_Socket, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress)))
	{
		qDebug() << "bind()函数执行错误" << endl;
		return false;
	}
	else
	{
		qDebug() << "bind() 完成." << endl;
	}

	// 绑定UDP地址和端口
	if (SOCKET_ERROR == bind(m_pListenContextUdp->m_Socket, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress)))
	{
		qDebug() << "UDP bind()函数执行错误" << endl;
		return false;
	}
	else
	{
		qDebug() << "UDP bind() 完成." << endl;
	}

	//将Udp Listen Socket绑定至完成端口中
	if (nullptr == CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_pListenContextUdp->m_Socket), m_hIOCompletionPort, reinterpret_cast<ULONG_PTR>(m_pListenContextUdp), 0))
	{
		qDebug() << "绑定Udp Listen Socket至完成端口失败！错误代码: " << WSAGetLastError() << endl;
		RELEASE_SOCKET(m_pListenContextUdp->m_Socket);
		return false;
	}
	else
	{
		qDebug() << "Udp Listen Socket绑定完成端口 完成." << endl;
	}
	//开始重叠recvfrom
	qDebug() << "马上开始recv" << endl;
	qDebug() << m_nThreads - MAX_POST_ACCEPT1 << endl;
	for (int i = 0; i < m_nThreads - MAX_POST_ACCEPT1; i++) {
		PER_IO_CONTEXT1* pRecvFromIoContext = m_pListenContextUdp->GetNewIoContext();
		pRecvFromIoContext->m_sockAccept = m_pListenContextUdp->m_Socket;
		if (false == this->_PostRecvFrom(pRecvFromIoContext))
		{
			m_pListenContextUdp->RemoveContext(pRecvFromIoContext);
			return false;
		}
	}

	// 开始进行监听(udp没有)
	if (SOCKET_ERROR == listen(m_pListenContext->m_Socket, SOMAXCONN))
	{
		qDebug() << "Listen()函数执行出现错误." << endl;
		return false;
	}
	else
	{
		qDebug() << "Listen() 完成." << endl;
	}

	// 使用AcceptEx函数，因为这个是属于WinSock2规范之外的微软另外提供的扩展函数
	// 所以需要额外获取一下函数的指针
	// 获取AcceptEx函数指针
	DWORD dwBytes = 0;
	if (SOCKET_ERROR == WSAIoctl(
		m_pListenContext->m_Socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx,
		sizeof(GuidAcceptEx),
		&m_lpfnAcceptEx,
		sizeof(m_lpfnAcceptEx),
		&dwBytes,
		nullptr,
		nullptr
	)) {
		qDebug() << "WSAIoctl 未能获取AcceptEx函数指针。错误代码: " << WSAGetLastError() << endl;
		this->_DeInitialize();
		return false;
	}

	// 获取GetAcceptExSockAddrs函数指针，也是同理
	if (SOCKET_ERROR == WSAIoctl(
		m_pListenContext->m_Socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockAddrs,
		sizeof(GuidGetAcceptExSockAddrs),
		&m_lpfnGetAcceptExSockAddrs,
		sizeof(m_lpfnGetAcceptExSockAddrs),
		&dwBytes,
		nullptr,
		nullptr))
	{
		qDebug() << "WSAIoctl 未能获取GuidGetAcceptExSockAddrs函数指针。错误代码: " << WSAGetLastError() << endl;
		this->_DeInitialize();
		return false;
	}

	// 为AcceptEx 准备参数，然后投递AcceptEx I/O请求
	for (int i = 0; i<MAX_POST_ACCEPT1; i++)
	{
		// 新建一个IO_CONTEXT
		PER_IO_CONTEXT1* pAcceptIoContext = m_pListenContext->GetNewIoContext();

		if (false == this->_PostAccept(pAcceptIoContext))
		{
			m_pListenContext->RemoveContext(pAcceptIoContext);
			return false;
		}
	}

	qDebug() << "投递 " << MAX_POST_ACCEPT1 << " 个AcceptEx请求完毕" << endl;

	return true;
}

//	最后释放掉所有资源
void IocpServer::CIOCPModel1::_DeInitialize()
{
	// 删除客户端列表的互斥量
	DeleteCriticalSection(&m_csContextList);

	// 关闭系统退出事件句柄
	RELEASE_HANDLE(m_hShutdownEvent);

	// 释放工作者线程句柄指针
	for (int i = 0; i<m_nThreads; i++)
	{
		RELEASE_HANDLE(m_phWorkerThreads[i]);
	}

	RELEASE(m_phWorkerThreads);

	// 关闭IOCP句柄
	RELEASE_HANDLE(m_hIOCompletionPort);

	// 关闭监听Socket
	RELEASE(m_pListenContext);
	RELEASE(m_pListenContextUdp);

	qDebug() << "释放资源完毕." << endl;
}

/*********************************************************************/

// 投递Accept请求
bool IocpServer::CIOCPModel1::_PostAccept(PER_IO_CONTEXT1* pAcceptIoContext)
{
	// 准备参数
	DWORD dwBytes = 0;
	pAcceptIoContext->m_OpType = ACCEPT_POST;
	WSABUF *p_wbuf = &pAcceptIoContext->m_wsaBuf;
	OVERLAPPED *p_ol = &pAcceptIoContext->m_Overlapped;

	// 为以后新连入的客户端先准备好Socket( 这个是与传统accept最大的区别 )
	pAcceptIoContext->m_sockAccept = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pAcceptIoContext->m_sockAccept)
	{
		qDebug() << "创建用于Accept的Socket失败！错误代码: " << WSAGetLastError() << endl;
		return false;
	}

	// 投递AcceptEx
	if (FALSE == m_lpfnAcceptEx(m_pListenContext->m_Socket, pAcceptIoContext->m_sockAccept, p_wbuf->buf, p_wbuf->len - ((sizeof(SOCKADDR_IN) + 16) * 2),
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwBytes, p_ol))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			qDebug() << "投递 AcceptEx 请求失败，错误代码: " << WSAGetLastError() << endl;
			return false;
		}
	}
	return true;
}

// 在有客户端连入的时候，进行处理
bool IocpServer::CIOCPModel1::_DoAccept(PER_SOCKET_CONTEXT1* pSocketContext, PER_IO_CONTEXT1* pIoContext) {
	SOCKADDR_IN* ClientAddr = nullptr;
	SOCKADDR_IN* LocalAddr = nullptr;
	int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);
	// 1. 首先取得连入客户端的地址信息
	this->m_lpfnGetAcceptExSockAddrs(pIoContext->m_wsaBuf.buf, pIoContext->m_wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&LocalAddr, &localLen, (LPSOCKADDR*)&ClientAddr, &remoteLen);

	qDebug() << "客户端 " << inet_ntoa(ClientAddr->sin_addr) << ":" << ntohs(ClientAddr->sin_port) << " 连入. " << endl;
	qDebug() << "客户端 " << inet_ntoa(ClientAddr->sin_addr) << ":" << ntohs(ClientAddr->sin_port) << " 信息： " << pIoContext->m_wsaBuf.buf << endl;

	// 2. 这里需要注意，这里传入的这个是ListenSocket上的Context，这个Context我们还需要用于监听下一个连接
	// 所以我还得要将ListenSocket上的Context复制出来一份为新连入的Socket新建一个SocketContext
	PER_SOCKET_CONTEXT1* pNewSocketContext = new PER_SOCKET_CONTEXT1;
	pNewSocketContext->m_Socket = pIoContext->m_sockAccept;
	memcpy(&(pNewSocketContext->m_ClientAddr), ClientAddr, sizeof(SOCKADDR_IN));
	// 参数设置完毕，将这个Socket和完成端口绑定(这也是一个关键步骤)
	if (false == this->_AssociateWithIOCP(pNewSocketContext))
	{
		RELEASE(pNewSocketContext);
		return false;
	}

	// 3. 继续，建立其下的IoContext，用于在这个Socket上投递第一个Recv数据请求
	PER_IO_CONTEXT1* pNewIoContext = pNewSocketContext->GetNewIoContext();
	pNewIoContext->m_OpType = RECV_POST;
	pNewIoContext->m_sockAccept = pNewSocketContext->m_Socket;
	// 如果Buffer需要保留，就自己拷贝一份出来
	//memcpy( pNewIoContext->m_szBuffer,pIoContext->m_szBuffer,MAX_BUFFER_LEN );

	// 绑定完毕之后，就可以开始在这个Socket上投递完成请求了
	if (false == this->_PostRecv(pNewIoContext))
	{
		pNewSocketContext->RemoveContext(pNewIoContext);
		return false;
	}

	// 4. 如果投递成功，那么就把这个有效的客户端信息，加入到ContextList中去(需要统一管理，方便释放资源)
	this->_AddToContextList(pNewSocketContext);

	// 5. 使用完毕之后，把Listen Socket的那个IoContext重置，然后准备投递新的AcceptEx
	pIoContext->ResetBuffer();
	return this->_PostAccept(pIoContext);
}

// 投递接收数据请求
bool IocpServer::CIOCPModel1::_PostRecv(PER_IO_CONTEXT1* pIoContext)
{
	// 初始化变量
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	WSABUF *p_wbuf = &pIoContext->m_wsaBuf;
	OVERLAPPED *p_ol = &pIoContext->m_Overlapped;

	pIoContext->ResetBuffer();
	pIoContext->m_OpType = RECV_POST;

	// 初始化完成后，投递WSARecv请求
	int nBytesRecv = WSARecv(pIoContext->m_sockAccept, p_wbuf, 1, &dwBytes, &dwFlags, p_ol, nullptr);

	// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		qDebug() << "投递第一个WSARecv失败！ " << endl;
		return false;
	}
	return true;
}

bool IocpServer::CIOCPModel1::_PostSend(PER_IO_CONTEXT1* pIoContext)
{
	// 初始化变量
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	WSABUF *p_wbuf = &pIoContext->m_wsaBuf;
	OVERLAPPED *p_ol = &pIoContext->m_Overlapped;

	//pIoContext->ResetBuffer();
	pIoContext->m_OpType = SEND_POST;

	// 初始化完成后，投递WSASend请求
	int nBytesRecv = WSASend(pIoContext->m_sockAccept, p_wbuf, 1, &dwBytes, dwFlags, p_ol, nullptr);

	// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		qDebug() << "投递WSASend失败！ " << endl;
		return false;
	}
	return true;
}

// 在有接收的数据到达的时候，进行处理
bool IocpServer::CIOCPModel1::_DoRecv(PER_SOCKET_CONTEXT1* pSocketContext, PER_IO_CONTEXT1* pIoContext)
{
	// 先把上一次的数据显示出现，然后就重置状态，发出下一个Recv请求
	SOCKADDR_IN* ClientAddr = &pIoContext->remoteAddr;
	qDebug() << "收到  " << inet_ntoa(ClientAddr->sin_addr) << ":" << ntohs(ClientAddr->sin_port) << " 信息： " << pIoContext->m_wsaBuf.buf << endl;

	// 然后开始投递下一个WSARecv请求
	return _PostRecv(pIoContext);
}

//UDP
//投递RecvFrom请求
bool IocpServer::CIOCPModel1::_PostRecvFrom(PER_IO_CONTEXT1* pIoContext) {
	// 初始化变量
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	WSABUF *p_wbuf = &pIoContext->m_wsaBuf;
	OVERLAPPED *p_ol = &pIoContext->m_Overlapped;

	pIoContext->ResetBuffer();
	pIoContext->m_OpType = RECVFROM_POST;

	// 初始化完成后，投递WSARecv请求
	int nBytesRecv = WSARecvFrom(pIoContext->m_sockAccept, p_wbuf, 1, &dwBytes, &dwFlags, (SOCKADDR*)& (pIoContext->remoteAddr), &(pIoContext->remoteAddrLen), p_ol, nullptr);

	//qDebug() << pIoContext->remoteAddr.sin_addr.S_un.S_addr << endl;

	// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		qDebug() << "投递第一个WSARecvFrom失败！ " << WSAGetLastError() << endl;
		return false;
	}
	return true;
}

bool IocpServer::CIOCPModel1::_DoRecvFrom(PER_IO_CONTEXT1* pIoContext, int threadNo, int curBufNo, MySqlHandler* mysqlHandler ) {
	// 先把上一次的数据显示出现，然后就重置状态，发出下一个Recv请求
	SOCKADDR_IN* ClientAddr = &pIoContext->remoteAddr;
	qDebug() << "收到" << inet_ntoa(ClientAddr->sin_addr) << ":" << ntohs(ClientAddr->sin_port) << " 信息： " << this->bufferPtr[threadNo][curBufNo] << endl;
	//qDebug() << (int)*pIoContext->m_wsaBuf.buf << endl;

	//qDebug() << (int)pIoContext->m_wsaBuf.buf[0] << endl;
	//qDebug() << (int)pIoContext->m_wsaBuf.buf[1] << endl;
	//qDebug() << (int)pIoContext->m_wsaBuf.buf[2] << endl;
	//qDebug() << (int)pIoContext->m_wsaBuf.buf[3] << endl;
	//qDebug() << (int)pIoContext->m_wsaBuf.buf[4] << endl;
	//对收到的数据进行处理
	//PER_IO_CONTEXT1 temp = *pIoContext;
	//m_tempContextArray.push_back(temp);
	//char temp[8192];
	//memcpy(temp, pIoContext->m_szBuffer, 8192);
	//WSABUF temp = (WSABUF)pIoContext->m_wsaBuf;
	qDebug() << "emit" << endl;
	emit parent->serviceHandler(pIoContext, this->bufferPtr[threadNo][curBufNo], mysqlHandler);

	// 然后开始投递下一个WSARecv请求
	//return _PostRecvFrom(pIoContext);

	PER_IO_CONTEXT1* pRecvFromIoContext = m_pListenContextUdp->GetNewIoContext();
	pRecvFromIoContext->m_sockAccept = m_pListenContextUdp->m_Socket;
	if (false == this->_PostRecvFrom(pRecvFromIoContext))
	{
		m_pListenContextUdp->RemoveContext(pRecvFromIoContext);
		return false;
	}
}

//
bool IocpServer::CIOCPModel1::_PostSendTo(PER_IO_CONTEXT1* pIoContext) {
	// 初始化变量
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	WSABUF *p_wbuf = &pIoContext->m_wsaBuf;
	OVERLAPPED *p_ol = &pIoContext->m_Overlapped;

	// pIoContext->ResetBuffer();
	pIoContext->m_OpType = SENDTO_POST;

	// 初始化完成后，投递WSASendTo请求
	int nBytesRecv = WSASendTo(pIoContext->m_sockAccept, p_wbuf, 1, &dwBytes, dwFlags, (SOCKADDR*)&pIoContext->remoteAddr,pIoContext->remoteAddrLen, p_ol, nullptr);

	// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		qDebug() << "投递WSASendTo失败！ " << endl;
		return false;
	}
	return true;
}

// 将句柄(Socket)绑定到完成端口中
bool IocpServer::CIOCPModel1::_AssociateWithIOCP(PER_SOCKET_CONTEXT1 *pContext)
{
	// 将用于和客户端通信的SOCKET绑定到完成端口中
	HANDLE hTemp = CreateIoCompletionPort((HANDLE)pContext->m_Socket, m_hIOCompletionPort, (ULONG_PTR)pContext, 0);

	if (nullptr == hTemp)
	{
		qDebug() << "执行CreateIoCompletionPort()出现错误.错误代码： " << GetLastError() << endl;
		return false;
	}

	return true;
}

// 显示并处理完成端口上的错误
bool IocpServer::CIOCPModel1::HandleError(PER_SOCKET_CONTEXT1 *pContext, const DWORD& dwErr)
{
	// 如果是超时了，就再继续等吧
	if (WAIT_TIMEOUT == dwErr)
	{
		// 确认客户端是否还活着...
		if (!_IsSocketAlive(pContext->m_Socket))
		{
			qDebug() << "检测到客户端异常退出！" << endl;
			this->_RemoveContext(pContext);
			return true;
		}
		else
		{
			qDebug() << "网络操作超时！重试中..." << endl;
			return true;
		}
	}

	// 可能是客户端异常退出了
	else if (ERROR_NETNAME_DELETED == dwErr)
	{
		qDebug() << "检测到客户端异常退出！" << endl;
		this->_RemoveContext(pContext);
		return true;
	}

	else
	{
		qDebug() << "完成端口操作出现错误，线程退出。错误代码：" << dwErr << endl;
		return false;
	}
}

// 判断客户端Socket是否已断开，在一个无效的Socket上投递WSARecv操作会出现异常
// 使用的方法是尝试向这个socket发送数据，判断这个socket调用的返回值
// 因为如果客户端网络异常断开(例如客户端崩溃或者拔掉网线等)的时候，服务器端是无法收到客户端断开的通知的
bool IocpServer::CIOCPModel1::_IsSocketAlive(SOCKET s)
{
	int nByteSent = send(s, "", 0, 0);
	if (-1 == nByteSent) return false;
	return true;
}

// 获得本机中处理器的数量
int IocpServer::CIOCPModel1::_GetNoOfProcessors()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
}

// 获得本机的IP地址
//string CIOCPModel1::GetLocalIP()
//{
//	// 获得本机主机名
//	char hostname[MAX_PATH] = { 0 };
//	gethostname(hostname, MAX_PATH);
//	struct hostent FAR* lpHostEnt = gethostbyname(hostname);
//	if (lpHostEnt == NULL)
//	{
//		return DEFAULT_IP1;
//	}
//	// 取得IP地址列表中的第一个为返回的IP(因为一台主机可能会绑定多个IP)
//	LPSTR lpAddr = lpHostEnt->h_addr_list[0];
//	// 将IP地址转化成字符串形式
//	struct in_addr inAddr;
//	memmove(&inAddr, lpAddr, 4);
//	//m_strIP = string(inet_ntoa(inAddr));
//	//qDebug(m_strIP);
//	return m_strIP;
//}

// 清空客户端信息
void IocpServer::CIOCPModel1::_ClearContextList()
{
	EnterCriticalSection(&m_csContextList);

	m_arrayClientContext.clear();
	vector<PER_SOCKET_CONTEXT1*>(m_arrayClientContext).swap(m_arrayClientContext);

	m_arrayUdpClientContext.clear();
	vector<IN_ADDR>(m_arrayUdpClientContext).swap(m_arrayUdpClientContext);
	
	LeaveCriticalSection(&m_csContextList);
}

//	移除某个特定的Context
void IocpServer::CIOCPModel1::_RemoveContext(PER_SOCKET_CONTEXT1 *pSocketContext)
{
	EnterCriticalSection(&m_csContextList);

	for (vector<PER_SOCKET_CONTEXT1*>::iterator it = m_arrayClientContext.begin(); it != m_arrayClientContext.end();) {
		if (*it == pSocketContext) {
			it = m_arrayClientContext.erase(it);
			delete pSocketContext;
			pSocketContext = nullptr;
			break;
		}
		else {
			++it;
		}
	}

	LeaveCriticalSection(&m_csContextList);
}

//移除某个udp用户
void IocpServer::CIOCPModel1::_RemoveADDR(IN_ADDR inADDR) {
	EnterCriticalSection(&m_csContextList);
	for (vector<IN_ADDR>::iterator it = m_arrayUdpClientContext.begin(); it != m_arrayUdpClientContext.end();) {
		IN_ADDR temp = (IN_ADDR)(*it);
		if (temp.S_un.S_addr == inADDR.S_un.S_addr) {
			it = m_arrayUdpClientContext.erase(it);
			break;
		}
		else {
			++it;
		}
	}
	LeaveCriticalSection(&m_csContextList);
}

// 将客户端的相关信息存储到数组中
void IocpServer::CIOCPModel1::_AddToContextList(PER_SOCKET_CONTEXT1 *pHandleData)
{
	EnterCriticalSection(&m_csContextList);

	m_arrayClientContext.push_back(pHandleData);

	LeaveCriticalSection(&m_csContextList);
}

//void IocpServer::CIOCPModel1::SendData() {
//	//取一个socket
//	PER_SOCKET_CONTEXT1* temp = nullptr;
//	if (!m_arrayClientContext.empty()) {
//		temp = m_arrayClientContext[0];
//	}
//	else {
//		qDebug() << "no socket" << endl;
//		return;
//	}
//
//	//创建一个iocontext
//	PER_IO_CONTEXT1* pNewIoContext = temp->GetNewIoContext();
//	pNewIoContext->m_OpType = SEND_POST;
//	pNewIoContext->m_sockAccept = temp->m_Socket;
//	strcpy(pNewIoContext->m_szBuffer, "test");
//	qDebug() << pNewIoContext->m_szBuffer << endl;
//	qDebug() << "!!!" << endl;
//	// 绑定完毕之后，就可以开始在这个Socket上投递完成请求了
//	if (false == this->_PostSend(pNewIoContext))
//	{
//		temp->RemoveContext(pNewIoContext);
//		return;
//	}
//}

void IocpServer::SendDataTo(PER_IO_CONTEXT1* pIoContext) {
	pIoContext->m_OpType = SENDTO_POST;
	qDebug() << pIoContext->m_szBuffer << endl;
	m_IOCP->SendDataTo(pIoContext);
}

void IocpServer::CIOCPModel1::SendDataTo(PER_IO_CONTEXT1* pIoContext) {
	_PostSendTo(pIoContext);
}

IocpServer::IocpServer()
{
	m_IOCP = new CIOCPModel1(this);
}

IocpServer::~IocpServer()
{
	m_IOCP->Stop();
}

bool IocpServer::serverStart() {
	if (false == m_IOCP->LoadSocketLib()) {
		qDebug() << "*** 加载Winsock 2.2失败，服务器端无法运行！ ***" << endl;
		return false;
	}
	else {
		qDebug() << "加载Winsock 2.2" << endl;
	}
	if (false == m_IOCP->Start()) {
		qDebug() << "*** server start fail ***" << endl;
		return false;
	}
	else {
		qDebug() << "server start" << endl;
		return true;
	}
}

void IocpServer::serverStop() {
	m_IOCP->Stop();
}