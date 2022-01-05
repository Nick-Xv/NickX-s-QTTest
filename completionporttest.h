#ifndef COMPLETIONPORTTEST_H
#define COMPLETIONPORTTEST_H

#include <QWidget>
#include <QDebug>
#include <windows.h>

class CompletionPortTest : public QWidget
{
    Q_OBJECT
public:
    explicit CompletionPortTest(QWidget *parent = nullptr);
    ~CompletionPortTest();

signals:

public slots:
};

#endif // COMPLETIONPORTTEST_H
