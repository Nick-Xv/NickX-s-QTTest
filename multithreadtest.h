#ifndef MULTITHREADTEST_H
#define MULTITHREADTEST_H
//#if defined(_MSC_VER) && (_MSC_VER >= 1600)
//# pragma execution_character_set("utf-8")
//#endif
#include <QWidget>
#include <QPushButton>
#include <QDebug>
#include <windows.h>

class MultiThreadTest : public QWidget
{
    Q_OBJECT
public:
    MultiThreadTest(QWidget* parent=nullptr);
    ~MultiThreadTest();

    static DWORD WINAPI threadFun(LPVOID lpParameter);

private:

};

#endif // MULTITHREADTEST_H
