#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void on_SnapButton_clicked();

    void on_StopButton_clicked();

    void on_PlaybackButton_clicked();

    void on_RecordButton_clicked();

private:
    Ui::Widget *ui;
};

#endif // WIDGET_H
