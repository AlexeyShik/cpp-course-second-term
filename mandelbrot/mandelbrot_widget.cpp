#include "mandelbrot_widget.h"
#include "mainwindow.h"
#include <complex>
#include <QImage>
#include <QPainter>
#include <QWheelEvent>
#include <QDrag>

mandelbrot_widget::mandelbrot_widget(QWidget *parent) : QWidget(parent) {
    for (size_t i = 0; i < worker::N_THREADS; ++i) {
        worker_objs.emplace_back(std::make_unique<worker>());
        worker_threads.emplace_back(std::make_unique<QThread>());
        worker_objs[i]->moveToThread(worker_threads[i].get());
        worker_objs[i]->set_id(i);
        worker_objs[i]->set_mutex(&mutex_);
        connect(worker_objs[i].get(), &worker::calculation_finished, this, &mandelbrot_widget::calculation_finished);
        connect(this, &mandelbrot_widget::calculate, worker_objs[i].get(), &worker::calculate);
        worker_threads[i]->start();
    }

    QWidget::grabKeyboard();
}

mandelbrot_widget::~mandelbrot_widget() {
    for (size_t i = 0; i < worker::N_THREADS; ++i) {
        worker_objs[i]->cancel();
        worker_threads[i]->exit();
        worker_threads[i]->wait();
    }
}

void mandelbrot_widget::paintEvent(QPaintEvent *event) {
    init_image();
    QPainter p(this);
    p.drawImage(0, 0, *img_ptr.get());
}

void mandelbrot_widget::set_power(double power) {
    init_image();
    if (current_power != power) {
        current_power = power;
        size_t version = worker_objs[0]->set_power(current_power);
        for (size_t i = 1; i < worker::N_THREADS; ++i) {
            worker_objs[i]->set_power(current_power);
        }
        emit calculate(version);
    }
}

void mandelbrot_widget::reset() {
    init_image();
    size_t version = worker_objs[0]->reset();
    for (size_t i = 1; i < worker::N_THREADS; ++i) {
        worker_objs[i]->reset();
    }
    emit calculate(version);
}

void mandelbrot_widget::calculation_finished(std::shared_ptr<QImage> img_ptr) {
    this->img_ptr = img_ptr;
    update();
}

void mandelbrot_widget::wheelEvent(QWheelEvent *event) {
    init_image();
    double numDegrees = static_cast<double>(event->angleDelta().y()) / 8;
    double numSteps = numDegrees / 15.0;
    double scale = pow(0.8, numSteps);
    double new_x = event->position().x();
    double new_y = event->position().y();
    size_t version = worker_objs[0]->set_center_and_scale(new_x, new_y, scale);
    for (size_t i = 1; i < worker::N_THREADS; ++i) {
        worker_objs[i]->set_center_and_scale(new_x, new_y, scale);
    }
    emit calculate(version);
}

void mandelbrot_widget::resizeEvent(QResizeEvent *event) {
    init_image();
    dragStartPosition = QPoint(width() / 2, height() / 2);
    auto ptr = std::make_shared<QImage>(width(), height(), QImage::Format_RGB888);
    size_t version = worker_objs[0]->set_input(ptr);
    for (size_t i = 1; i < worker::N_THREADS; ++i) {
        worker_objs[i]->set_input(ptr);
    }
    emit calculate(version);
}

void mandelbrot_widget::keyPressEvent(QKeyEvent *event) {
    init_image();
    QPoint move;
    switch (event->key()) {
        case Qt::Key_Left :
        case Qt::Key_A : {
            move = QPoint(-MOVE_LENGTH, 0);
            break;
        }
        case Qt::Key_Right :
        case Qt::Key_D : {
            move = QPoint(MOVE_LENGTH, 0);
            break;
        }
        case Qt::Key_Up :
        case Qt::Key_W :{
            move = QPoint(0, -MOVE_LENGTH);
            break;
        }
        case Qt::Key_Down :
        case Qt::Key_S : {
            move = QPoint(0, MOVE_LENGTH);
            break;
        }
        default : return;
    }
    dragStartPosition += move;
    size_t version = worker_objs[0]->move_center(move.x(), move.y());
    for (size_t i = 1; i < worker::N_THREADS; ++i) {
        worker_objs[i]->move_center(move.x(), move.y());
    }
    emit calculate(version);
}

void mandelbrot_widget::mousePressEvent(QMouseEvent *event) {
    init_image();
    if (event->button() == Qt::LeftButton) {
        dragStartPosition = event->pos();
    }
}

void mandelbrot_widget::mouseMoveEvent(QMouseEvent *event) {
    init_image();
    if (!(event->buttons() == Qt::LeftButton))
        return;
    QPoint delta = event->pos() - dragStartPosition;
    dragStartPosition = event->pos();
    size_t version = worker_objs[0]->move_center(-delta.x(), -delta.y());
    for (size_t i = 1; i < worker::N_THREADS; ++i) {
        worker_objs[i]->move_center(-delta.x(), -delta.y());
    }
    emit calculate(version);
}

void mandelbrot_widget::init_image() {
    if (img_ptr.get() == nullptr) {
        auto ptr = std::shared_ptr<QImage>(new QImage(width(), height(), QImage::Format_RGB888));
        size_t version = worker_objs[0]->set_input(ptr);
        for (size_t i = 1; i < worker::N_THREADS; ++i) {
            worker_objs[i]->set_input(ptr);
        }
        emit calculate(version);
    }
}
