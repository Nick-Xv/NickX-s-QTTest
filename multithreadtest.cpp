#include "multithreadtest.h"

const int NAME_LINE = 40;

//线程函数传入参数的结构体
typedef struct __THREAD_DATA{
    int nMaxNum;
    char strThreadName[NAME_LINE];

    __THREAD_DATA():nMaxNum(0){
        memset(strThreadName, 0, NAME_LINE * sizeof(char));
    }
}THREAD_DATA;

HANDLE g_hMutex = nullptr; //互斥量

MultiThreadTest::MultiThreadTest(QWidget* parent):
    QWidget(parent)
{
    //创建一个互斥量
    g_hMutex = CreateMutex(nullptr, FALSE, nullptr);

    //初始化线程数据
    THREAD_DATA threadData1, threadData2;
    threadData1.nMaxNum = 5;
    threadData2.nMaxNum = 10;
    strcpy(threadData1.strThreadName,"线程1");
    strcpy(threadData2.strThreadName,"线程2");

    HANDLE thread1 = CreateThread(nullptr, 0, threadFun, &threadData1, 0, nullptr);
    HANDLE thread2 = CreateThread(nullptr, 0, threadFun, &threadData2, 0, nullptr);

    CloseHandle(thread1);
    CloseHandle(thread2);

    for(int i=0;i<10;i++){
        //请求一个互斥量锁
        WaitForSingleObject(g_hMutex,INFINITE);
        qDebug()<<"主线程 === "<<i<<endl;
        Sleep(100);
        //释放互斥量锁
        ReleaseMutex(g_hMutex);
    }
}
MultiThreadTest::~MultiThreadTest(){

}

DWORD WINAPI MultiThreadTest::threadFun(LPVOID lpParameter){
    THREAD_DATA* pThreadData = reinterpret_cast<THREAD_DATA*>(lpParameter);
    int maxNum = pThreadData->nMaxNum;
    for(int i=0;i<maxNum;i++){
        WaitForSingleObject(g_hMutex,INFINITE);
        qDebug()<<pThreadData->strThreadName<<" --- "<<i<<endl;
        Sleep(100);
        ReleaseMutex(g_hMutex);
    }
    return 0L;
}
