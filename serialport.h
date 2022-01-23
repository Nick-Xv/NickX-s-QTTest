#ifndef SERIALPORT_H
#define SERIALPORT_H
//#if defined(_MSC_VER) && (_MSC_VER >= 1600)
//# pragma execution_character_set("utf-8")
//#endif
#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QPushButton>
#include <QLineEdit>
class SerialPort : public QWidget{
    Q_OBJECT

public:
    SerialPort(QWidget* parent=nullptr);
    ~SerialPort();

    QStringList getPortNameList();
    void openPort();
    void writeBytes();

signals:
    void sendData();

public slots:
    void receiveInfo();
private:
    QSerialPort* m_serialPort;
    QSerialPort* m_serialPort2;

    QStringList m_portNameList;
    QPushButton* m_OpenPortButton;
    QPushButton* m_WriteDataButton;
    QLineEdit* m_SendDataText;
};

#endif // SERIALPORT_H
