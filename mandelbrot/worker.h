#pragma once
#include <QObject>
#include <QImage>
#include <mutex>
#include <atomic>
#include <memory>
#include <thread>

class worker : public QObject {
    Q_OBJECT

public:
    const static unsigned int N_THREADS;

    worker() = default;
    ~worker() = default;
    void set_id(size_t id);
    void set_mutex(std::mutex *publish_mutex);
    size_t set_input(std::shared_ptr<QImage> img_ptr);
    size_t set_center_and_scale(double new_x, double new_y, double new_scale);
    size_t move_center(double delta_x, double delta_y);
    size_t set_power(double power);
    size_t reset();
    void cancel();

public slots:
            void calculate(size_t local_version);

    signals:
            void calculation_finished(std::shared_ptr<QImage> img_ptr);

private:
    void calculate_image(size_t local_version, size_t precision);
    double value(int x, int y, int w, int h, double power) const;

    constexpr static size_t FIRST_PRECISION = 9;
    constexpr static double STEP_PRECISION = 3;

    double scale{0.005};
    double x_center{0};
    double y_center{0};
    double power{2};

    std::atomic_size_t version{0};
    std::mutex mutex_{};

    size_t id;
    std::mutex *publish_mutex;
    std::shared_ptr<QImage> img_ptr;
};
