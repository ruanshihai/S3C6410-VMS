#ifndef TCAPTURE_H
#define TCAPTURE_H

#include <QThread>

class TCapture : public QThread {
    Q_OBJECT

public:
    void run();
};

#endif // TCAPTURE_H
