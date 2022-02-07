#include "worker.h"
#include <complex>
#include <vector>
#include <algorithm>

const unsigned int worker::N_THREADS = std::thread::hardware_concurrency();  // hardware_concurrency() не constexpr, а жаль

void worker::calculate(size_t local_version) {
    calculate_image(local_version, FIRST_PRECISION);
}

void worker::calculate_image(size_t local_version, size_t precision) {
    std::unique_lock lock(mutex_);
    bool first_iteration = precision == FIRST_PRECISION;
    size_t h = img_ptr->height();
    size_t w = img_ptr->width();
    unsigned char *data = img_ptr->bits();
    size_t stride = img_ptr->bytesPerLine();
    if (!first_iteration) lock.unlock(); // first iteration always fast, hold lock all iteration

    size_t block_size = (h / N_THREADS) - (h / N_THREADS) % precision;
    size_t begin = id * block_size;
    size_t end = id == N_THREADS - 1 ? h : std::min(h, (id + 1) * block_size);

    for (size_t y = begin; y < end; y += precision) {
        std::vector<unsigned char> line;
        for (size_t y_ = y; y_ < end && y_ < y + precision; ++y_) {
            std::vector<unsigned char> calculated_bits;
            calculated_bits.resize(3 * w);
            unsigned char *p = calculated_bits.data();

            for (size_t x = 0; x < w; x += precision) {
                if (!first_iteration) lock.lock();
                if (local_version != version.load()) {
                    return;
                }

                unsigned char color;
                if (y_ % precision == 0) {
                    color = static_cast<unsigned char>(value(x, y_, w, h, power) * 255);
                    line.push_back(color);
                } else {
                    color = line[x / precision];
                }

                for (size_t x_ = x; x_ < w && x_ < x + precision; ++x_) {
                    if (color == 0.0) {
                        *p++ = 0.0;
                        *p++ = 0.0;
                        *p++ = 0.0;
                    } else {
                        *p++ = 80.0;
                        *p++ = color;
                        *p++ = 120.0;
                    }
                }
                if (!first_iteration) lock.unlock();
            }

            std::lock_guard<std::mutex> guard(*publish_mutex);
            memcpy(data + y_ * stride, calculated_bits.data(), 3 * w  * sizeof(char));
        }
    }
    if (local_version != version.load()) {
        return;
    }
    emit calculation_finished(img_ptr);
    if (precision > 1) {
        if (first_iteration) lock.unlock();
        calculate_image(local_version, precision / STEP_PRECISION);
    }
}

void worker::set_id(size_t id) {
    this->id = id;
}

void worker::set_mutex(std::mutex *publish_mutex) {
    this->publish_mutex = publish_mutex;
}

size_t worker::set_input(std::shared_ptr<QImage> img_ptr) {
    std::lock_guard guard(mutex_);
    this->img_ptr = img_ptr;
    return ++version;
}

size_t worker::set_center_and_scale(double new_x, double new_y, double factor) {
    std::lock_guard guard(mutex_);
    double old_scale = scale;
    scale *= factor;
    double delta_x;
    double delta_y;
    size_t w = img_ptr->width();
    size_t h = img_ptr->height();

    if (factor <= 1) {
        delta_x = new_x - w / 2;
        delta_y = -(new_y - h / 2);
    } else {
        delta_x = -(new_x - w / 2);
        delta_y = new_y - h / 2;
    }
    double delta_r = abs(hypot(delta_x, delta_y));
    double cos_a = delta_x / delta_r;
    double sin_a = delta_y / delta_r;
    if (factor <= 1) {
        x_center += delta_r * (1 - factor) * cos_a * old_scale;
        y_center -= delta_r * (1 - factor) * sin_a * old_scale;
    } else {
        x_center -= delta_r * (1 - factor) * cos_a * old_scale;
        y_center += delta_r * (1 - factor) * sin_a * old_scale;
    }
    return ++version;
}

size_t worker::move_center(double delta_x, double delta_y) {
    std::lock_guard guard(mutex_);
    x_center += delta_x * scale;
    y_center += delta_y * scale;
    return ++version;
}

size_t worker::set_power(double power) {
    std::lock_guard guard(mutex_);
    this->power = power;
    return ++version;
}

size_t worker::reset() {
    std::lock_guard guard(mutex_);
    x_center = 0;
    y_center = 0;
    scale = 0.005;
    return ++version;
}

void worker::cancel() {
    std::lock_guard guard(mutex_);
    ++version;
}

double worker::value(int x, int y, int w, int h, double power) const {

    std::complex<double> c(x - w / 2, y - h / 2);
    c = c * scale + std::complex(x_center, y_center);

    std::complex<double> z = 0;

    size_t const MAX_STEPS = 100;
    size_t step = 0;
    for (;;) {
        if (std::abs(z) >= 2.) {
            double s = 1.0 * step / MAX_STEPS;
            return (1 - s) * 200;
        }
        if (step == MAX_STEPS) {
            return 0;
        }
        z = std::pow(z, power) + c;
        ++step;
    }
}
