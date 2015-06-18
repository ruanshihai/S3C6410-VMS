#include <QApplication>

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tcapture.h"
#include "widget.h"

bool is_thread_running;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;

    w.showFullScreen();
    a.exec();

    return 0;
}
