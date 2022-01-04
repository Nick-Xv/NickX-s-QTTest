#ifndef UDPTEST_H
#define UDPTEST_H

#include <QWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QString>
#include <QThread>
#include "workthread.h"

class UdpTest : public QWidget
{
    Q_OBJECT
public:
    UdpTest(QWidget* parent=nullptr);
    ~UdpTest();
    void recvTest();
    void threadTest();
private:
    QPushButton* sendBut;
    QPushButton* sendBut10;
    QTextEdit* infoText;
    QTextEdit* inputText;
    QVBoxLayout* layout1;
    QHBoxLayout* layout2;
    WorkThread* task;//testThread
public slots:
    void appendTextSlot(QString str);
};

#endif // UDPTEST_H
