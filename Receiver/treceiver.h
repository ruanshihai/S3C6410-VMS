#ifndef TRECEIVER_H
#define TRECEIVER_H

#include <QThread>
#include <QtNetwork>

class TH264Decoder;

class TReceiver : public QThread
{
    Q_OBJECT

public:
    TReceiver();
    void run();
private:
    QUdpSocket *receiver;
    TH264Decoder *Decoder;
    bool first_frame;

//信号槽
private slots:
    void readPendingDatagrams();
};

#endif // TRECEIVER_H
