#include "mainwindow.h"
#include "serialport.h"
#include "tcptest.h"
#include "udptest.h"

#include <QApplication>
#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStringList>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDesktopWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>

//自定义槽函数
class MyWidget:public QWidget{
    Q_OBJECT
public slots:
    void treeWidgetClicked(QTreeWidgetItem * item);
};
//接收 itemClicked() 信号函数传递过来的 item 参数
void MyWidget::treeWidgetClicked(QTreeWidgetItem *item){
    //遍历 item 结点所有的子结点
    for(int i=0;i<item->childCount();i++){
        //找到每个子结点
        QTreeWidgetItem *childItem = item->child(i);
        //将子结点的选中状态调整为和父结点相同
        childItem->setCheckState(0,item->checkState(0));
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //窗口居中测试
    QDesktopWidget* pDesk = QApplication::desktop();

    MainWindow mw;
    mw.resize(500,500);
    mw.move((pDesk->width() - mw.width()) / 2, (pDesk->height() - mw.height()) / 2);

    QPushButton mwbut1(&mw);
    mwbut1.setGeometry(50,50,100,50);
    mwbut1.setText("布局测试");

    QPushButton mwbut2(&mw);
    mwbut2.setGeometry(50,100,100,50);
    mwbut2.setText("串口测试");

    QPushButton mwbut3(&mw);
    mwbut3.setGeometry(50,150,100,50);
    mwbut3.setText("tcp测试");

    QPushButton mwbut4(&mw);
    mwbut4.setGeometry(50,200,100,50);
    mwbut4.setText("udp测试");

    mw.show();
    QWidget widget;
    widget.setWindowTitle("QVBoxLayout垂直布局");
    widget.setGeometry(100,100,500,500);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setDirection(QBoxLayout::BottomToTop);

    //连续创建 3 个文本框，并设置它们的背景和字体大小
    QLabel lab1("Label1");
    lab1.setStyleSheet("QLabel{background:#dddddd;font:20px;}");
    lab1.setAlignment(Qt::AlignCenter);
    QLabel lab2("Label2");
    lab2.setStyleSheet("QLabel{background:#cccccc;font:20px;}");
    lab2.setAlignment(Qt::AlignCenter);
    QLabel lab3("Label3");
    lab3.setStyleSheet("QLabel{background:#ffffff;font:20px;}");
    lab3.setAlignment(Qt::AlignCenter);

    QPushButton but1("TEST table");

    layout->addStretch(2);
    layout->addWidget(&lab1,1);
    layout->addWidget(&lab2,2);
    layout->addWidget(&lab3,3);
    layout->addStretch(3);
    layout->addWidget(&but1,3);

    widget.setLayout(layout);

    //show前移动到居中
    widget.move((pDesk->width() - widget.width()) / 2, (pDesk->height() - widget.height()) / 2);

    MyWidget widget1;
    widget1.setWindowTitle("MyTreeWidget控件");
    widget1.resize(600,300);
    QTreeWidget treeWidget(&widget1);
    //设置列数
    treeWidget.setColumnCount(3);
    treeWidget.resize(600,300);

    //添加顶层结点
    QTreeWidgetItem topItem;
    topItem.setText(0,"教程");
    topItem.setCheckState(0,Qt::Unchecked);
    treeWidget.addTopLevelItem(&topItem);

    //添加子结点
    QStringList c;
    c << "C语言教程" << "http://c.biancheng.net/c/" <<"已完成";
    QTreeWidgetItem childItem1(&topItem,c);
    childItem1.setCheckState(0,Qt::Unchecked);

    QStringList qt;
    qt << "Qt教程" << "http://c.biancheng.net/qt/" <<"未完成";
    QTreeWidgetItem childItem2(&topItem,qt);
    childItem2.setCheckState(0,Qt::Unchecked);

    QStringList python;
    python << "Python教程" << "http://c.biancheng.net/python/" <<"已完成";
    QTreeWidgetItem childItem3(&topItem,python);
    childItem3.setCheckState(0,Qt::Unchecked);

    //串口测试
    SerialPort widget2;

    //tcp测试
    TcpTest widget3;

    //udp测试
    UdpTest widget4;


    //添加信号和槽机制，当某个结点被点击时，调整所有子结点的选中状态，同时将所有子结点展开。
    QObject::connect(&treeWidget,&QTreeWidget::itemClicked,&widget1,&MyWidget::treeWidgetClicked);
    QObject::connect(&treeWidget,&QTreeWidget::itemClicked,&treeWidget,&QTreeWidget::expandItem);
    QObject::connect(&but1,&QPushButton::clicked,&widget1,&QWidget::show);
    QObject::connect(&mwbut1,&QPushButton::clicked,&widget,&QWidget::show);
    QObject::connect(&mwbut2,&QPushButton::clicked,&widget2,&QWidget::show);
    QObject::connect(&mwbut3,&QPushButton::clicked,&widget3,&QWidget::show);
    QObject::connect(&mwbut4,&QPushButton::clicked,&widget4,&QWidget::show);
    return a.exec();
}
//MyWidget类的定义应该放到 .h 文件中，本例中将其写到 main.cpp 中，程序最后需要添加 #include "当前源文件名.moc" 语句，否则无法通过编译。
#include "main.moc"
