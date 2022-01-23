#include "desktop.h"

desktop::desktop()
{
    QApplication a();
    pDesk  = QApplication::desktop();
}
