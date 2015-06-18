#include <QApplication>

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "widget.h"
#include "treceiver.h"

bool is_recording = false;
bool receive_enable = true;
FILE* encoded_fp;

int main(int argc, char *argv[])
{
    TReceiver receiver;

    QApplication a(argc, argv);
    Widget w;

    receiver.start();
    w.showFullScreen();
    a.exec();
    receiver.wait();

    return 0;
}
