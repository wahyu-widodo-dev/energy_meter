#include "energy_meter.h"

#include <cmath>

#include "esp_timer.h"

esp_err_t EnergyMeter::update(bool output_enabled, float current_a, float power_w)
{
    const int64_t timer_us = esp_timer_get_time();
    if (timer_us < 0)
    {
        has_previous_sample_ = false;
        previous_output_enabled_ = false;
        return ESP_FAIL;
    }

    const uint64_t now_us = static_cast<uint64_t>(timer_us);
    if (!output_enabled)
    {
        last_update_us_ = now_us;
        has_previous_sample_ = true;
        previous_output_enabled_ = false;
        return ESP_OK;
    }

    if (!std::isfinite(current_a) || current_a < 0.0f ||
        !std::isfinite(power_w) || power_w < 0.0f)
    {
        last_update_us_ = now_us;
        has_previous_sample_ = false;
        previous_output_enabled_ = false;
        return ESP_ERR_INVALID_ARG;
    }

    if (has_previous_sample_ && previous_output_enabled_ &&
        now_us > last_update_us_)
    {
        const uint64_t elapsed_us = now_us - last_update_us_;
        const double elapsed_seconds =
            static_cast<double>(elapsed_us) / kMicrosecondsPerSecond;
        const double average_current_a =
            (previous_current_a_ + static_cast<double>(current_a)) * 0.5;
        const double average_power_w =
            (previous_power_w_ + static_cast<double>(power_w)) * 0.5;

        data_.ampere_hour +=
            average_current_a * elapsed_seconds / kSecondsPerHour;
        data_.watt_hour +=
            average_power_w * elapsed_seconds / kSecondsPerHour;
        runtime_us_ += elapsed_us;
    }

    last_update_us_ = now_us;
    previous_current_a_ = current_a;
    previous_power_w_ = power_w;
    has_previous_sample_ = true;
    previous_output_enabled_ = true;

    data_.runtime_ms = runtime_us_ / 1000ULL;
    data_.runtime_seconds = runtime_us_ / 1000000ULL;
    return ESP_OK;
}

void EnergyMeter::reset()
{
    data_ = {};
    last_update_us_ = 0;
    runtime_us_ = 0;
    previous_current_a_ = 0.0;
    previous_power_w_ = 0.0;
    has_previous_sample_ = false;
    previous_output_enabled_ = false;
}

EnergyMeter::Data EnergyMeter::get_data() const
{
    return data_;
}
