#include "completionporttest.h"

//每个处理器上产生多少个线程()
#define WORKER_THREADS_PER_PROCESSOR 2
//同时投递的accept请求的数量(灵活设置)
#define MAX_POST_ACCEPT 10
//传递给Worker线程的退出信号
#define EXIT_CODE NULL

//释放指针和句柄资源的宏
//释放指针宏
#define RELEASE(x) {if((x)!=nullptr){delete(x);(x)=nullptr;}}
//释放句柄宏
#define RELEASE_HANDLE(x) {if((x)!=nullptr&&(x)!=INVALID_HANDLE_VALUE){CloseHandle(x);(x)=nullptr;}}
//释放Socket宏
#define RELEASE_SOCKET(x) {if((x)!=INVALID_SOCKET){closesocket(x);x=INVALID_SOCKET;}}

CIOCPModel::CIOCPModel(): 
    m_nThreads(0),
    m_hShutdownEvent(nullptr),
    m_hIOCompletionPort(nullptr),
    m_phWorkerThreads(nullptr),
    m_strIP(DEFAULT_IP),
    m_nPort(DEFAULT_PORT),
    m_lpfnAcceptEx(nullptr),
    m_pListenContext(nullptr)
{
}

CIOCPModel::~CIOCPModel()
{
    // 确保资源彻底释放
    this->Stop();
}

//WorkerThread
DWORD WINAPI CIOCPModel::_WorkerThread(LPVOID lpParam){
    THREADPARAMS_WORKER* pParam = static_cast<THREADPARAMS_WORKER*>(lpParam);
    CIOCPModel* pIOCPModel = static_cast<CIOCPModel*>(pParam->pIOCPModel);
    int nThreadNo = static_cast<int>(pParam->nThreadNo);
    //显示信息
    qDebug()<<"workerthread start "<<nThreadNo<<endl;

    OVERLAPPED* pOverlapped = nullptr;
    PER_SOCKET_CONTEXT* pSocketContext = nullptr;
    DWORD dwBytesTransfered = 0;

    //循环处理请求，直到接收到shutdown信息为止
    while(WAIT_OBJECT_0!=WaitForSingleObject(pIOCPModel->m_hShutdownEvent,0)){
        BOOL bReturn = GetQueuedCompletionStatus(
                    pIOCPModel->m_hIOCompletionPort,
                    &dwBytesTransfered,
                    (PULONG_PTR)&pSocketContext,
                    &pOverlapped,
                    INFINITE
                    );
		qDebug() << "!!!" << pSocketContext << endl;
		qDebug() << sizeof(DWORD) << endl;
		qDebug() << sizeof(pSocketContext) << endl;
        //收到退出标志则退出
        if(EXIT_CODE==reinterpret_cast<ULONG_PTR>(pSocketContext))break;
        //判断是否出现错误
        if(!bReturn){
            DWORD dwErr = GetLastError();

            //显示信息
            if(!pIOCPModel->HandleError(pSocketContext,dwErr))break;
            continue;
        }
        else{
			//qDebug() << pOverlapped << endl;
            //读取传入的参数
            PER_IO_CONTEXT* pIoContext = CONTAINING_RECORD(pOverlapped,PER_IO_CONTEXT,m_Overlapped);

            //判断是否有客户端断开了
            if((0==dwBytesTransfered)&&(RECV_POSTED==pIoContext->m_OpType||SEND_POSTED==pIoContext->m_OpType))
            {
                //显示信息
                qDebug()<<"客户端 "<<inet_ntoa(pSocketContext->m_ClientAddr.sin_addr)<<":"<<ntohs(pSocketContext->m_ClientAddr.sin_port)<<"断开连接."<<endl;
                //释放资源
                pIOCPModel->_RemoveContext(pSocketContext);
                continue;
            }
            else{
				//qDebug() << "!!!!" << pSocketContext->m_ClientAddr.sin_addr.S_un.S_addr << endl;
                switch(pIoContext->m_OpType){
                    //accept
                case ACCEPT_POSTED:{
                    pIOCPModel->_DoAccept(pSocketContext,pIoContext);
                }
                    break;
                case RECV_POSTED:{
                    pIOCPModel->_DoRecv(pSocketContext,pIoContext);
                }
                    break;
                case SEND_POSTED:{
                    //qDebug()<<"你有问题啊"<<endl;
                }
                    break;
                default:
                    qDebug()<<"_WorkThread中的 pIoContext->m_OpType 参数异常"<<endl;
                    break;
                }
            }
        }
    }
    qDebug()<<"工作者线程 "<<nThreadNo<<"号退出 "<<endl;
    RELEASE(lpParam);
    return 0;
}

//init winsock 2.2
bool CIOCPModel::LoadSocketLib(){
    WSADATA wsaData;
    int nResult = WSAStartup(MAKEWORD(2,2),&wsaData);
    if(NO_ERROR != nResult){
        qDebug()<<"初始化WinSock 2.2失败"<<endl;
        return false;
    }
    return true;
}

bool CIOCPModel::Start(){
    //初始化线程互斥量
    InitializeCriticalSection(&m_csContextList);
    //建立系统退出的事件通知
    m_hShutdownEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    //init IOCP
    if(false == _InitializeIOCP()){
        qDebug()<<"IOCP init fail"<<endl;
        return false;
    }
    else{
        qDebug()<<"IOCP init complete"<<endl;
    }
    //init socket
    if(false == _InitializeListenSocket()){
        qDebug()<<"init listen socket fail"<<endl;
        this->_DeInitialize();
        return false;
    }
    else{
        qDebug()<<"listen socket init complete"<<endl;
    }
    qDebug()<<"system init complete, waiting for connection"<<endl;
    return true;
}

//发送系统退出消息，退出完成端口和线程资源
void CIOCPModel::Stop(){
    if(m_pListenContext!=nullptr&&m_pListenContext->m_Socket!=INVALID_SOCKET){
        // 激活关闭消息通知
        SetEvent(m_hShutdownEvent);
        for(int i=0;i<m_nThreads;i++){
            // 通知所有的完成端口操作退出
            PostQueuedCompletionStatus(m_hIOCompletionPort,0,static_cast<DWORD>(EXIT_CODE),nullptr);
        }
        //等待所有的客户端资源退出
        WaitForMultipleObjects(m_nThreads,m_phWorkerThreads,TRUE,INFINITE);
        this->_ClearContextList();
        this->_DeInitialize();
        qDebug()<<"停止监听"<<endl;
    }
}

// 初始化完成端口
bool CIOCPModel::_InitializeIOCP()
{
    // 建立第一个完成端口
    m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0 );

    if ( nullptr == m_hIOCompletionPort)
    {
        qDebug()<<"建立完成端口失败！错误代码: "<<WSAGetLastError()<<endl;
        return false;
    }

    // 根据本机中的处理器数量，建立对应的线程数
    m_nThreads = WORKER_THREADS_PER_PROCESSOR * _GetNoOfProcessors();

    // 为工作者线程初始化句柄
    m_phWorkerThreads = new HANDLE[m_nThreads];

    // 根据计算出来的数量建立工作者线程
    DWORD nThreadID;
    for (int i = 0; i < m_nThreads; i++)
    {
        THREADPARAMS_WORKER* pThreadParams = new THREADPARAMS_WORKER;
        pThreadParams->pIOCPModel = this;
        pThreadParams->nThreadNo  = i+1;
        m_phWorkerThreads[i] = ::CreateThread(0, 0, _WorkerThread, (void *)pThreadParams, 0, &nThreadID);
    }

    qDebug()<<"建立 _WorkerThread "<<m_nThreads<<"个."<<endl;

    return true;
}

//init socket
bool CIOCPModel::_InitializeListenSocket(){
    //AcceptEx and GetAcceptExSockaddrs GUID，用于导出函数指针
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;

    //服务器地址信息，绑定socket
    struct sockaddr_in ServerAddress;

    //生成用于监听的socket的信息
    m_pListenContext = new PER_SOCKET_CONTEXT;

    //需要使用重叠IO，必须得使用WSASocket来建立socket
    m_pListenContext->m_Socket = WSASocket(AF_INET,SOCK_STREAM,0,nullptr,0,WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == m_pListenContext->m_Socket)
    {
        qDebug()<<"初始化Socket失败，错误代码: "<<WSAGetLastError()<<endl;
        return false;
    }
    else
    {
        qDebug()<<"WSASocket() 完成"<<endl;
    }

    //将Listen Socket绑定至完成端口中
    if(nullptr == CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_pListenContext->m_Socket), m_hIOCompletionPort,reinterpret_cast<ULONG_PTR>(m_pListenContext), 0))
    {
        qDebug()<<"绑定 Listen Socket至完成端口失败！错误代码: "<<WSAGetLastError()<<endl;
        RELEASE_SOCKET( m_pListenContext->m_Socket );
        return false;
    }
    else
    {
        qDebug()<<"Listen Socket绑定完成端口 完成."<<endl;
    }
    //填充地址信息
    ZeroMemory((char *)&ServerAddress, sizeof(ServerAddress));
    ServerAddress.sin_family = AF_INET;
    //这里可以绑定任何可用的IP地址，或者绑定一个指定的IP地址
    ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    ServerAddress.sin_port = htons(m_nPort);

    // 绑定地址和端口
    if(SOCKET_ERROR == bind(m_pListenContext->m_Socket, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress)))
    {
        qDebug()<<"bind()函数执行错误"<<endl;
        return false;
    }
    else
    {
        qDebug()<<"bind() 完成."<<endl;
    }

    // 开始进行监听
    if(SOCKET_ERROR == listen(m_pListenContext->m_Socket,SOMAXCONN))
    {
        qDebug()<<"Listen()函数执行出现错误."<<endl;
        return false;
    }
    else
    {
        qDebug()<<"Listen() 完成."<<endl;
    }

    // 使用AcceptEx函数，因为这个是属于WinSock2规范之外的微软另外提供的扩展函数
    // 所以需要额外获取一下函数的指针
    // 获取AcceptEx函数指针
    DWORD dwBytes = 0;
    if(SOCKET_ERROR == WSAIoctl(
                m_pListenContext->m_Socket,
                SIO_GET_EXTENSION_FUNCTION_POINTER,
                &GuidAcceptEx,
                sizeof(GuidAcceptEx),
                &m_lpfnAcceptEx,
                sizeof(m_lpfnAcceptEx),
                &dwBytes,
                nullptr,
                nullptr
                )){
        qDebug()<<"WSAIoctl 未能获取AcceptEx函数指针。错误代码: "<<WSAGetLastError()<<endl;
        this->_DeInitialize();
        return false;
    }

    // 获取GetAcceptExSockAddrs函数指针，也是同理
    if(SOCKET_ERROR == WSAIoctl(
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
        qDebug()<<"WSAIoctl 未能获取GuidGetAcceptExSockAddrs函数指针。错误代码: "<<WSAGetLastError()<<endl;
        this->_DeInitialize();
        return false;
    }

    // 为AcceptEx 准备参数，然后投递AcceptEx I/O请求
    for( int i=0;i<MAX_POST_ACCEPT;i++ )
    {
        // 新建一个IO_CONTEXT
        PER_IO_CONTEXT* pAcceptIoContext = m_pListenContext->GetNewIoContext();

        if( false==this->_PostAccept( pAcceptIoContext ) )
        {
            m_pListenContext->RemoveContext(pAcceptIoContext);
            return false;
        }
    }

    qDebug()<<"投递 "<<MAX_POST_ACCEPT<<" 个AcceptEx请求完毕"<<endl;

    return true;
}

//	最后释放掉所有资源
void CIOCPModel::_DeInitialize()
{
    // 删除客户端列表的互斥量
    DeleteCriticalSection(&m_csContextList);

    // 关闭系统退出事件句柄
    RELEASE_HANDLE(m_hShutdownEvent);

    // 释放工作者线程句柄指针
    for( int i=0;i<m_nThreads;i++ )
    {
        RELEASE_HANDLE(m_phWorkerThreads[i]);
    }

    RELEASE(m_phWorkerThreads);

    // 关闭IOCP句柄
    RELEASE_HANDLE(m_hIOCompletionPort);

    // 关闭监听Socket
    RELEASE(m_pListenContext);

    qDebug()<<"释放资源完毕."<<endl;
}

// 投递Accept请求
bool CIOCPModel::_PostAccept( PER_IO_CONTEXT* pAcceptIoContext )
{
    //ASSERT( INVALID_SOCKET!=m_pListenContext->m_Socket );

    // 准备参数
    DWORD dwBytes = 0;
    pAcceptIoContext->m_OpType = ACCEPT_POSTED;
    WSABUF *p_wbuf   = &pAcceptIoContext->m_wsaBuf;
    OVERLAPPED *p_ol = &pAcceptIoContext->m_Overlapped;

    // 为以后新连入的客户端先准备好Socket( 这个是与传统accept最大的区别 )
    pAcceptIoContext->m_sockAccept  = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if( INVALID_SOCKET==pAcceptIoContext->m_sockAccept )
    {
        qDebug()<<"创建用于Accept的Socket失败！错误代码: "<<WSAGetLastError()<<endl;
        return false;
    }

    // 投递AcceptEx
    if(FALSE == m_lpfnAcceptEx( m_pListenContext->m_Socket, pAcceptIoContext->m_sockAccept, p_wbuf->buf, p_wbuf->len - ((sizeof(SOCKADDR_IN)+16)*2),
                                sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, &dwBytes, p_ol))
    {
        if(WSA_IO_PENDING != WSAGetLastError())
        {
            qDebug()<<"投递 AcceptEx 请求失败，错误代码: "<<WSAGetLastError()<<endl;
            return false;
        }
    }

    return true;
}

// 在有客户端连入的时候，进行处理
bool CIOCPModel::_DoAccept( PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext ){
    SOCKADDR_IN* ClientAddr = nullptr;
    SOCKADDR_IN* LocalAddr = nullptr;
    int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);
    // 1. 首先取得连入客户端的地址信息
    this->m_lpfnGetAcceptExSockAddrs(pIoContext->m_wsaBuf.buf, pIoContext->m_wsaBuf.len - ((sizeof(SOCKADDR_IN)+16)*2),
            sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, (LPSOCKADDR*)&LocalAddr, &localLen, (LPSOCKADDR*)&ClientAddr, &remoteLen);
    qDebug()<<"客户端 "<<inet_ntoa(ClientAddr->sin_addr)<<":"<<ntohs(ClientAddr->sin_port)<<" 连入. "<<endl;
    qDebug()<<"客户端 "<<inet_ntoa(ClientAddr->sin_addr)<<":"<<ntohs(ClientAddr->sin_port)<<" 信息： "<<pIoContext->m_wsaBuf.buf<<endl;

    // 2. 这里需要注意，这里传入的这个是ListenSocket上的Context，这个Context我们还需要用于监听下一个连接
    // 所以我还得要将ListenSocket上的Context复制出来一份为新连入的Socket新建一个SocketContext
    PER_SOCKET_CONTEXT* pNewSocketContext = new PER_SOCKET_CONTEXT;
    pNewSocketContext->m_Socket = pIoContext->m_sockAccept;
    memcpy(&(pNewSocketContext->m_ClientAddr), ClientAddr, sizeof(SOCKADDR_IN));
    // 参数设置完毕，将这个Socket和完成端口绑定(这也是一个关键步骤)
    if(false==this->_AssociateWithIOCP(pNewSocketContext))
    {
        RELEASE(pNewSocketContext);
        return false;
    }

    // 3. 继续，建立其下的IoContext，用于在这个Socket上投递第一个Recv数据请求
    PER_IO_CONTEXT* pNewIoContext = pNewSocketContext->GetNewIoContext();
    pNewIoContext->m_OpType       = RECV_POSTED;
    pNewIoContext->m_sockAccept   = pNewSocketContext->m_Socket;
    // 如果Buffer需要保留，就自己拷贝一份出来
    //memcpy( pNewIoContext->m_szBuffer,pIoContext->m_szBuffer,MAX_BUFFER_LEN );

    // 绑定完毕之后，就可以开始在这个Socket上投递完成请求了
    if( false==this->_PostRecv( pNewIoContext) )
    {
        pNewSocketContext->RemoveContext( pNewIoContext );
        return false;
    }

    // 4. 如果投递成功，那么就把这个有效的客户端信息，加入到ContextList中去(需要统一管理，方便释放资源)
    this->_AddToContextList( pNewSocketContext );

    // 5. 使用完毕之后，把Listen Socket的那个IoContext重置，然后准备投递新的AcceptEx
        pIoContext->ResetBuffer();
        return this->_PostAccept( pIoContext );
}

// 投递接收数据请求
bool CIOCPModel::_PostRecv( PER_IO_CONTEXT* pIoContext )
{
    // 初始化变量
    DWORD dwFlags = 0;
    DWORD dwBytes = 0;
    WSABUF *p_wbuf   = &pIoContext->m_wsaBuf;
    OVERLAPPED *p_ol = &pIoContext->m_Overlapped;

    pIoContext->ResetBuffer();
    pIoContext->m_OpType = RECV_POSTED;

    // 初始化完成后，投递WSARecv请求
    int nBytesRecv = WSARecv( pIoContext->m_sockAccept, p_wbuf, 1, &dwBytes, &dwFlags, p_ol, nullptr );

    // 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
    if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
    {
        qDebug()<<"投递第一个WSARecv失败！ "<<endl;
        return false;
    }
    return true;
}

bool CIOCPModel::_PostSend(PER_IO_CONTEXT* pIoContext)
{
    // 初始化变量
    DWORD dwFlags = 0;
    DWORD dwBytes = 0;
    WSABUF *p_wbuf = &pIoContext->m_wsaBuf;
    OVERLAPPED *p_ol = &pIoContext->m_Overlapped;

    //pIoContext->ResetBuffer();
    pIoContext->m_OpType = SEND_POSTED;

    // 初始化完成后，投递WSASend请求
    int nBytesRecv = WSASend(pIoContext->m_sockAccept, p_wbuf, 1, &dwBytes, dwFlags, p_ol, nullptr);

    // 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
    if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
    {
        qDebug()<<"投递WSASend失败！ "<<endl;
        return false;
    }
    return true;
}

// 在有接收的数据到达的时候，进行处理
bool CIOCPModel::_DoRecv( PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext )
{
    // 先把上一次的数据显示出现，然后就重置状态，发出下一个Recv请求
	SOCKADDR_IN ClientAddr;
	qDebug() << pSocketContext << endl;
	qDebug() << pSocketContext->m_Socket << endl;
	qDebug() << pSocketContext->m_ClientAddr.sin_port << endl;
	ClientAddr = pSocketContext->m_ClientAddr;
    qDebug()<<"收到  "<<inet_ntoa(ClientAddr.sin_addr)<<":"<<ntohs(ClientAddr.sin_port)<<" 信息： "<<pIoContext->m_wsaBuf.buf<<endl;

    // 然后开始投递下一个WSARecv请求
    return _PostRecv( pIoContext );
}

// 将句柄(Socket)绑定到完成端口中
bool CIOCPModel::_AssociateWithIOCP( PER_SOCKET_CONTEXT *pContext )
{
    // 将用于和客户端通信的SOCKET绑定到完成端口中
    HANDLE hTemp = CreateIoCompletionPort((HANDLE)pContext->m_Socket, m_hIOCompletionPort, (ULONG_PTR)pContext, 0);

    if (nullptr == hTemp)
    {
        qDebug()<<"执行CreateIoCompletionPort()出现错误.错误代码： "<<GetLastError()<<endl;
        return false;
    }

    return true;
}

CompletionPortTest::CompletionPortTest(QWidget *parent) : QWidget(parent)
{
    this->resize(500,700);
    but1 = new QPushButton(this);
    but1->setText("开始监听");
    but2 = new QPushButton(this);
    but2->setText("结束监听");
    but3 = new QPushButton(this);
    but3->setText("发送数据");
    text1 = new QTextEdit(this);
    text2 = new QTextEdit(this);
    layout1 = new QVBoxLayout(this);
    layout2 = new QHBoxLayout;

    layout2->setDirection(QBoxLayout::LeftToRight);
    layout1->setDirection(QBoxLayout::TopToBottom);

    layout2->addWidget(but1,1);
    layout2->addWidget(but2,1);
    layout2->addWidget(but3,1);
    layout1->addLayout(layout2,1);
    layout1->addWidget(text2,1);
    layout1->addWidget(text1,5);

    if( false==m_IOCP.LoadSocketLib() ){
        qDebug()<<"加载Winsock 2.2失败，服务器端无法运行！"<<endl;
    }
    else{
        qDebug()<<"加载Winsock 2.2"<<endl;
    }

    connect(but1,&QPushButton::clicked,this,&CompletionPortTest::but1OnClick);
    connect(but2,&QPushButton::clicked,this,&CompletionPortTest::but2OnClick);
    connect(but3,&QPushButton::clicked,this,&CompletionPortTest::but3OnClick);
}

CompletionPortTest::~CompletionPortTest(){
    m_IOCP.Stop();
}

// 显示并处理完成端口上的错误
bool CIOCPModel::HandleError( PER_SOCKET_CONTEXT *pContext,const DWORD& dwErr )
{
    // 如果是超时了，就再继续等吧
    if(WAIT_TIMEOUT == dwErr)
    {
        // 确认客户端是否还活着...
        if( !_IsSocketAlive( pContext->m_Socket) )
        {
            qDebug()<<"检测到客户端异常退出！"<<endl;
            this->_RemoveContext( pContext );
            return true;
        }
        else
        {
            qDebug()<<"网络操作超时！重试中..."<<endl;
            return true;
        }
    }

    // 可能是客户端异常退出了
    else if( ERROR_NETNAME_DELETED==dwErr )
    {
        qDebug()<<"检测到客户端异常退出！"<<endl;
        this->_RemoveContext( pContext );
        return true;
    }

    else
    {
        qDebug()<<"完成端口操作出现错误，线程退出。错误代码："<<dwErr<<endl;
        return false;
    }
}

// 判断客户端Socket是否已断开，在一个无效的Socket上投递WSARecv操作会出现异常
// 使用的方法是尝试向这个socket发送数据，判断这个socket调用的返回值
// 因为如果客户端网络异常断开(例如客户端崩溃或者拔掉网线等)的时候，服务器端是无法收到客户端断开的通知的
bool CIOCPModel::_IsSocketAlive(SOCKET s)
{
    int nByteSent=send(s,"",0,0);
    if (-1 == nByteSent) return false;
    return true;
}

// 获得本机中处理器的数量
int CIOCPModel::_GetNoOfProcessors()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
}

// 获得本机的IP地址
QString CIOCPModel::GetLocalIP()
{
    // 获得本机主机名
    char hostname[MAX_PATH] = {0};
    gethostname(hostname,MAX_PATH);
    struct hostent FAR* lpHostEnt = gethostbyname(hostname);
    if(lpHostEnt == NULL)
    {
        return DEFAULT_IP;
    }
    // 取得IP地址列表中的第一个为返回的IP(因为一台主机可能会绑定多个IP)
    LPSTR lpAddr = lpHostEnt->h_addr_list[0];
    // 将IP地址转化成字符串形式
    struct in_addr inAddr;
    memmove(&inAddr,lpAddr,4);
    m_strIP = QString( inet_ntoa(inAddr) );
    return m_strIP;
}

// 清空客户端信息
void CIOCPModel::_ClearContextList()
{
    EnterCriticalSection(&m_csContextList);

    m_arrayClientContext.clear();
    std::vector<PER_SOCKET_CONTEXT*>(m_arrayClientContext).swap(m_arrayClientContext);

    LeaveCriticalSection(&m_csContextList);
}

//	移除某个特定的Context
void CIOCPModel::_RemoveContext( PER_SOCKET_CONTEXT *pSocketContext )
{
    EnterCriticalSection(&m_csContextList);

    for(std::vector<PER_SOCKET_CONTEXT*>::iterator it = m_arrayClientContext.begin(); it!=m_arrayClientContext.end();){
        if(*it == pSocketContext){
            it = m_arrayClientContext.erase(it);
            delete pSocketContext;
            pSocketContext = nullptr;
            break;
        }else{
            ++it;
        }
    }

    LeaveCriticalSection(&m_csContextList);
}

// 将客户端的相关信息存储到数组中
void CIOCPModel::_AddToContextList( PER_SOCKET_CONTEXT *pHandleData )
{
    EnterCriticalSection(&m_csContextList);

    m_arrayClientContext.push_back(pHandleData);

    LeaveCriticalSection(&m_csContextList);
}



//....................................................................
void CompletionPortTest::appendTextSlot(QString str){
    text1->append(str);
}

void CompletionPortTest::but1OnClick(){
    if(false == m_IOCP.Start()){
        qDebug()<<"server start fail"<<endl;
        return;
    }
}

void CompletionPortTest::but2OnClick(){
    m_IOCP.Stop();
}

void CompletionPortTest::but3OnClick(){
    m_IOCP.SendData();
}

void CIOCPModel::SendData(){
    //取一个socket
    PER_SOCKET_CONTEXT* temp = nullptr;
    if(!m_arrayClientContext.empty()){
        temp = m_arrayClientContext[0];
    }
    else{
        qDebug()<<"no socket"<<endl;
        return;
    }

    //创建一个iocontext
    PER_IO_CONTEXT* pNewIoContext = temp->GetNewIoContext();
    pNewIoContext->m_OpType       = SEND_POSTED;
    pNewIoContext->m_sockAccept   = temp->m_Socket;
    strcpy(pNewIoContext->m_szBuffer,"test");
    qDebug()<<pNewIoContext->m_szBuffer<<endl;
    qDebug()<<"!!!"<<endl;
    // 绑定完毕之后，就可以开始在这个Socket上投递完成请求了
    if( false==this->_PostSend(pNewIoContext) )
    {
        temp->RemoveContext( pNewIoContext );
        return;
    }
}
