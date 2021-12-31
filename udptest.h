#ifndef UDPTEST_H
#define UDPTEST_H

#include <QWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <winsock2.h>

class UdpTest : public QWidget
{
    Q_OBJECT
public:
    UdpTest(QWidget* parent=nullptr);
    ~UdpTest();
private:
    QPushButton* sendBut;
    QPushButton* sendBut10;
    QTextEdit* infoText;
    QTextEdit* inputText;
    QVBoxLayout* layout1;
    QHBoxLayout* layout2;
};

#endif // UDPTEST_H
