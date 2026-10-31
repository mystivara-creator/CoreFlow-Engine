import type { PolicyFormValues } from '../bridge/daemonBridge';
export const SETTINGS_GUIDE: Record<string, { label: string; purpose: string; tradeoff: string }> = {
  monitor_interval: { label: 'Interval pengamatan', purpose: 'Jarak antar siklus utama engine; 5 detik cocok sebagai titik awal.', tradeoff: 'Lebih kecil: lebih sering membaca kondisi, tetapi overhead naik. Lebih besar: lebih hemat, respons lebih lambat. Saat idle engine dapat menunggu lebih lama.' },
  min_confidence: { label: 'Kepercayaan minimum', purpose: 'Ambang kualitas data sebelum engine boleh menyesuaikan resource. 0.60 berarti 60%, bukan target penggunaan CPU.', tradeoff: 'Lebih tinggi: lebih selektif dan bisa lebih sering menunggu. Lebih rendah bukan jaminan performa lebih baik; mulai dari 0.60.' },
  launch_window_ms: { label: 'Jendela pembukaan aplikasi', purpose: 'Lama fase pembukaan setelah perubahan proses foreground terdeteksi; 2500 ms = 2,5 detik.', tradeoff: 'Memperpanjangnya menambah waktu bantuan mungkin dipertimbangkan, bukan mempercepat aplikasi secara otomatis. Deteksi proses bukan ukuran waktu buka aplikasi.' },
  jank_miss_rate: { label: 'Ambang gangguan presentasi', purpose: 'Proporsi interval frame yang melewati periode layar; 0.05 = 5%. Harus ada bukti frame dan tekanan resource yang sesuai.', tradeoff: 'Lebih kecil: lebih sensitif. Lebih besar: lebih selektif. Aplikasi 30 FPS pada layar 60 Hz dapat melewati ambang tanpa benar-benar macet.' },
  boost_window_ms: { label: 'Durasi bantuan sementara', purpose: 'Batas waktu satu boost; 3000 ms = 3 detik. Bantuan launch juga dibatasi sisa jendela pembukaan.', tradeoff: 'Durasi panjang dapat menambah konsumsi daya. Bukan boost permanen; engine memulihkan nilai setelah bantuan selesai.' },
  boost_uclamp_launch: { label: 'Batas bantuan CPU saat membuka app', purpose: 'Permintaan minimum utilisasi penjadwal untuk top-app, bukan mengunci penggunaan CPU. Awal: 20%.', tradeoff: 'Nilai besar dapat menambah daya/panas. Hanya berpengaruh jika target uclamp didukung, ditemukan dan diizinkan.' },
  boost_uclamp_jank: { label: 'Batas bantuan CPU saat tersendat', purpose: 'Permintaan uclamp sementara ketika gangguan frame disertai tekanan CPU. Awal: 10%.', tradeoff: 'Ini bukan target FPS. Jangan menaikkannya hanya karena GPU sibuk; penyebab gangguan harus sesuai.' },
  boost_readahead_kb: { label: 'Read-ahead sementara', purpose: 'Ukuran baca pendahuluan untuk antrean penyimpanan yang lolos pemeriksaan. Awal: 512 KB; 0 berarti tidak menambah read-ahead.', tradeoff: 'Lebih besar belum tentu lebih cepat dan dapat menambah I/O/memori. Hanya diminta bila ada tekanan I/O dengan tekanan memori rendah.' },
  boost_gpu_steps: { label: 'Langkah bantuan GPU', purpose: 'Jumlah langkah peningkatan batas minimum GPU pada tabel frekuensi yang ditemukan. Awal: 1; 0 meniadakan tambahan langkah.', tradeoff: 'Bukan MHz dan bukan optimasi driver Vulkan. Nilai besar dibatasi kapabilitas perangkat dan berpotensi menambah panas.' },
};
export function settingsPreset(id: 'balanced' | 'quiet' | 'responsive', defaults: PolicyFormValues): PolicyFormValues {
  if (id === 'quiet') return { ...defaults, monitor_interval: '8', min_confidence: '0.70', interactivity_mode: 'observe' };
  if (id === 'responsive') return { ...defaults, monitor_interval: '2', min_confidence: '0.65' };
  return { ...defaults };
}
