#include "completionporttest.h"

CompletionPortTest::CompletionPortTest(QWidget *parent) : QWidget(parent)
{
    //建立一个完成端口
    HANDLE m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int m_nProcessors = static_cast<int>(si.dwNumberOfProcessors);
//    qDebug()<<m_nProcessors<<endl;
    //根据CPU数量创建两倍的线程
    int m_nThreads = 2 * m_nProcessors;
    HANDLE* m_phWorkerThreads = new HANDLE[m_nThreads];
//    for(int i = 0; i < m_nThreads; i++){
//        m_phWorkerThreads[i] = CreateThread(0,0,_WorkerThread,nullptr,0,nullptr);
//    }
    //创建一个用于监听的socket，绑定到完成端口上，在指定的端口上监听连接请求
}

CompletionPortTest::~CompletionPortTest(){

}
