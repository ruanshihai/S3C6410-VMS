#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

extern bool is_thread_running;

void Widget::on_beginButton_clicked()
{
    if (!is_thread_running) {
        is_thread_running = true;
        capture = new TCapture();
        capture->start();
    }
}

void Widget::on_stopButton_clicked()
{
    if (is_thread_running) {
        is_thread_running = false;
        capture->wait();
        delete(capture);
    }
}
