#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QString>
#include <cstring>

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent)
        , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    connect(ui->horizontalSlider, &QSlider::sliderMoved, this, &MainWindow::slider_moved);
    connect(ui->pushButton, &QPushButton::clicked, ui->widget, &mandelbrot_widget::reset);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::slider_moved(int value) {
    int diff = value - ui->horizontalSlider->minimum();
    int range = ui->horizontalSlider->maximum() - ui->horizontalSlider->minimum();
    double pos = static_cast<double>(diff) / range;
    double power = pos + 2.;
    ui->widget->set_power(power);
    ui->label_number->setText(QString::number(power));
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
}
