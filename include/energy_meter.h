#pragma once

#include <cstdint>

#include "esp_err.h"

/**
 * @brief Mengintegrasikan arus dan daya menjadi muatan serta energi.
 *
 * Komponen ini tidak bergantung pada driver sensor atau model data aplikasi.
 * Aplikasi memasok sampel dan membaca hasil akumulasinya.
 */
class EnergyMeter final
{
public:
    struct Data
    {
        double ampere_hour = 0.0; /*!< Muatan terakumulasi, dalam Ah. */
        double watt_hour = 0.0;   /*!< Energi terakumulasi, dalam Wh. */
        uint64_t runtime_ms = 0;  /*!< Runtime output aktif, dalam ms. */
        uint64_t runtime_seconds = 0; /*!< Runtime output aktif, dalam detik. */
    };

    EnergyMeter() = default;
    ~EnergyMeter() = default;

    EnergyMeter(const EnergyMeter &) = delete;
    EnergyMeter &operator=(const EnergyMeter &) = delete;
    EnergyMeter(EnergyMeter &&) = delete;
    EnergyMeter &operator=(EnergyMeter &&) = delete;

    /**
     * @brief Tambahkan satu sampel arus dan daya.
     *
     * Hanya interval dengan kedua sampel berstatus aktif yang diintegrasikan.
     * Integrasi trapezoidal digunakan di antara sampel; sampel aktif pertama
     * menjadi baseline.
     *
     * Saat output nonaktif, pemanggilan menetapkan baseline baru tanpa
     * memvalidasi atau mengintegrasikan nilai sensor.
     *
     * @param output_enabled true jika output PSU aktif.
     * @param current_a Arus terukur non-negatif, dalam ampere.
     * @param power_w Daya terukur non-negatif, dalam watt.
     * @return ESP_OK jika berhasil atau ESP_ERR_INVALID_ARG jika sampel aktif
     *         tidak finite atau negatif. Sampel invalid menghapus baseline
     *         untuk mencegah integrasi menggunakan data lama.
     */
    esp_err_t update(bool output_enabled, float current_a, float power_w);

    /** Hapus akumulasi muatan, energi, runtime, dan baseline sampel. */
    void reset();

    /** Ambil salinan data hasil akumulasi. */
    Data get_data() const;

private:
    static constexpr double kMicrosecondsPerSecond = 1000000.0;
    static constexpr double kSecondsPerHour = 3600.0;

    Data data_{};
    uint64_t last_update_us_ = 0;
    uint64_t runtime_us_ = 0;
    double previous_current_a_ = 0.0;
    double previous_power_w_ = 0.0;
    bool has_previous_sample_ = false;
    bool previous_output_enabled_ = false;
};
