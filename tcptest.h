#ifndef TCPTEST_H
#define TCPTEST_H
//#if defined(_MSC_VER) && (_MSC_VER >= 1600)
//# pragma execution_character_set("utf-8")
//#endif
#include <QWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVBoxLayout>

class TcpTest : public QWidget
{
    Q_OBJECT
public:
    TcpTest(QWidget* parent=nullptr);
    ~TcpTest();

    void sendData();

private:
    QPushButton* sendButton;
    QTextEdit* inputText;
    QTextEdit* infoText;

    QTcpServer* tcpServer;//监听
    QTcpSocket* tcpSocket;//通信

    QVBoxLayout* testLayout;
};

#endif // TCPTEST_H
