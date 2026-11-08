# Energy Meter

Komponen C++ mandiri untuk menghitung runtime output, muatan listrik, dan
energi PSU dari pembacaan arus serta daya yang disediakan aplikasi. Komponen
ini tidak membaca sensor secara langsung dan tidak bergantung pada `DataShared`,
INA226, atau driver perangkat tertentu.

## Fitur

- Akumulasi muatan dalam ampere-jam (Ah).
- Akumulasi energi dalam watt-jam (Wh).
- Runtime output aktif dalam milidetik dan detik.
- Integrasi trapezoidal di antara sampel berurutan.
- Penggunaan waktu monotonik ESP Timer, bukan waktu kalender.
- Validasi nilai sensor dan reset baseline setelah sampel tidak valid.
- Snapshot data dan reset akumulator.

## Persyaratan dan instalasi

- ESP-IDF **5.3 atau lebih baru**.
- Proyek menggunakan C++.

Tambahkan dependency ke `idf_component.yml` proyek setelah komponen tersedia
di ESP Component Registry:

```yaml
dependencies:
  wahyu-widodo-dev/energy:
    version: "^1.0.0"
```

Jalankan `idf.py reconfigure` untuk mengambil dependency. Untuk pengembangan
lokal, letakkan direktori komponen ini di dalam `components/` proyek atau
tambahkan lokasinya ke `EXTRA_COMPONENT_DIRS`.

## Penggunaan

Panggil `update()` secara berkala setelah mendapatkan sampel arus dan daya.
Tandai `output_enabled` sesuai status output yang sebenarnya, bukan sekadar
status permintaan output.

```cpp
#include <cstdio>

#include "energy_meter.h"

static EnergyMeter energy_meter;

void control_step(bool output_is_enabled,
                  float measured_current_a,
                  float measured_power_w)
{
    const esp_err_t err = energy_meter.update(
        output_is_enabled,
        measured_current_a,
        measured_power_w);
    if (err != ESP_OK)
    {
        // Tangani sampel yang tidak valid sesuai kebijakan aplikasi.
        return;
    }

    const EnergyMeter::Data totals = energy_meter.get_data();
    printf("Runtime: %llu s | Charge: %.6f Ah | Energy: %.6f Wh\n",
           static_cast<unsigned long long>(totals.runtime_seconds),
           totals.ampere_hour,
           totals.watt_hour);
}
```

Sampel pertama hanya membentuk baseline. Akumulasi dimulai pada sampel enabled
berikutnya, menggunakan waktu aktual di antara kedua sampel. Untuk hasil yang
representatif, panggil `update()` dengan periode yang konsisten dan cukup cepat
terhadap perubahan beban.

## Perhitungan dan perilaku

Untuk setiap interval antara dua sampel aktif, komponen menggunakan rata-rata
dua titik:

```text
I_avg = (I_previous + I_current) / 2
P_avg = (P_previous + P_current) / 2
Ah += I_avg × elapsed_seconds / 3600
Wh += P_avg × elapsed_seconds / 3600
```

Runtime dihitung dari interval yang sama. Interval hanya dihitung jika kedua
sampel pembatasnya memiliki `output_enabled == true`. Memanggil `update()`
dengan output nonaktif menghentikan akumulasi dan memutus interval; sampel aktif
berikutnya menjadi baseline baru. Ini menghindari memasukkan waktu output mati
ke dalam totals.

`current_a` dan `power_w` harus finite dan tidak negatif ketika output aktif.
Nilai tidak valid menghasilkan `ESP_ERR_INVALID_ARG`, tidak menambah akumulator,
dan menghapus baseline sehingga interval berikutnya tidak memakai data lama.
Ketika output nonaktif, nilai sensor tidak dipakai atau divalidasi.

Energi dihitung dari nilai daya yang diberikan aplikasi. Jika sensor hanya
menyediakan tegangan dan arus, hitung daya dengan metode yang sesuai untuk
sistem Anda sebelum memanggil `update()`. Komponen ini tidak mengasumsikan
bahwa daya selalu sama dengan hasil perkalian tegangan DC dan arus.

## API

| API | Kegunaan |
|---|---|
| `update(enabled, current_a, power_w)` | Tambahkan satu sampel dan akumulasikan nilai bila output aktif. |
| `get_data()` | Ambil snapshot `Data` berisi Ah, Wh, runtime ms, dan runtime detik. |
| `reset()` | Hapus semua totals dan baseline sampel. |

`EnergyMeter::Data` menggunakan `double` untuk Ah/Wh agar presisi akumulasi
lebih baik dan `uint64_t` untuk runtime. Nilai tetap bersifat volatile dan
hilang saat perangkat restart. Aplikasi dapat menyimpan snapshot ke NVS atau
media persisten bila diperlukan.

## Catatan akurasi

- Akurasi bergantung pada kalibrasi sensor, periode pemanggilan, dan perubahan
  beban di antara sampel.
- Integrasi trapezoidal mengasumsikan perubahan linear di antara dua sampel.
- Jeda panjang antar sampel dapat mengurangi akurasi estimasi energi. Komponen
  menggunakan selang waktu aktual, sehingga tentukan periode pembacaan sesuai
  karakteristik PSU.
- Komponen tidak melakukan penyimpanan persisten, pemfilteran sensor,
  sinkronisasi antar-task, atau publikasi ke model data aplikasi.
- Jika satu instance diakses dari beberapa task, sinkronisasi harus disediakan
  oleh aplikasi.
