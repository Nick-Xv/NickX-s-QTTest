#ifndef MAINWINDOW_H
#define MAINWINDOW_H
//#if defined(_MSC_VER) && (_MSC_VER >= 1600)
//# pragma execution_character_set("utf-8")
//#endif

#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};
#endif // MAINWINDOW_H
