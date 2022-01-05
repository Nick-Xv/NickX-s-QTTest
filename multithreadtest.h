#ifndef MULTITHREADTEST_H
#define MULTITHREADTEST_H
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
