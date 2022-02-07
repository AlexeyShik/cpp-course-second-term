#include <QWidget>
#include <QThread>
#include <QGraphicsView>
#include <vector>
#include <memory>
#include "worker.h"

#pragma once

class mandelbrot_widget : public QWidget {

    Q_OBJECT

public:
    mandelbrot_widget(QWidget *parent = nullptr);
    ~mandelbrot_widget();
    void paintEvent(QPaintEvent* event) override;
    void set_power(double power);

public slots:
            void calculation_finished(std::shared_ptr<QImage> img_ptr);
    void reset();

    signals:
            void calculate(size_t version);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void init_image();

    const static int MOVE_LENGTH{50};

    std::shared_ptr<QImage> img_ptr{};
    double current_power{0};
    QPoint dragStartPosition{};

    std::vector<std::unique_ptr<QThread>> worker_threads;
    std::vector<std::unique_ptr<worker>> worker_objs;

    std::mutex mutex_{};
};
