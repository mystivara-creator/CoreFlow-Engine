use crate::bridge::ffi::ThermalTrendResult;

const WINDOW_CAPACITY: usize = 16;

#[derive(Clone, Copy, Default)]
struct ThermalPoint {
    ts_ms: u64,
    temp_c: f64,
}

/// Fixed-size zero-allocation ring buffer performing OLS linear regression over
/// recent SoC temperature samples to produce jitter-free thermal velocity (°C/min).
pub struct ThermalTrendAnalyzer {
    buf: [ThermalPoint; WINDOW_CAPACITY],
    head: usize,
    len: usize,
}

pub fn new_thermal_trend_analyzer() -> Box<ThermalTrendAnalyzer> {
    Box::new(ThermalTrendAnalyzer {
        buf: [ThermalPoint::default(); WINDOW_CAPACITY],
        head: 0,
        len: 0,
    })
}

impl ThermalTrendAnalyzer {
    pub fn reset(&mut self) {
        self.head = 0;
        self.len = 0;
    }

    pub fn push_sample(&mut self, ts_ms: u64, temp_c: f64, trip_c: f64) -> ThermalTrendResult {
        if !temp_c.is_finite() || temp_c <= 0.0 || temp_c > 130.0 {
            return ThermalTrendResult {
                velocity_cpm: 0.0,
                stability_score: 100,
                seconds_to_trip: -1.0,
            };
        }

        if self.len > 0 {
            let last_idx = (self.head + WINDOW_CAPACITY - 1) % WINDOW_CAPACITY;
            if ts_ms <= self.buf[last_idx].ts_ms {
                self.reset();
            }
        }

        self.buf[self.head] = ThermalPoint { ts_ms, temp_c };
        self.head = (self.head + 1) % WINDOW_CAPACITY;
        if self.len < WINDOW_CAPACITY {
            self.len += 1;
        }

        if self.len < 4 {
            return ThermalTrendResult {
                velocity_cpm: 0.0,
                stability_score: 100,
                seconds_to_trip: -1.0,
            };
        }

        let start = if self.len == WINDOW_CAPACITY {
            self.head
        } else {
            0
        };
        let t0 = self.buf[start].ts_ms;
        let n = self.len as f64;

        // Ordinary Least Squares slope (x = elapsed minutes, y = Celsius)
        let mut sum_x = 0.0;
        let mut sum_y = 0.0;
        let mut sum_xx = 0.0;
        let mut sum_xy = 0.0;
        for i in 0..self.len {
            let pt = self.buf[(start + i) % WINDOW_CAPACITY];
            let x = (pt.ts_ms.saturating_sub(t0) as f64) / 60_000.0;
            let y = pt.temp_c;
            sum_x += x;
            sum_y += y;
            sum_xx += x * x;
            sum_xy += x * y;
        }

        let denom = n * sum_xx - sum_x * sum_x;
        let velocity_cpm = if denom > 1e-9 {
            ((n * sum_xy - sum_x * sum_y) / denom).clamp(-25.0, 25.0)
        } else {
            0.0
        };

        // Stability score (100 = flat/cooling, drops smoothly as heating velocity increases)
        let penalty = if velocity_cpm > 0.4 {
            ((velocity_cpm - 0.4) * 14.0).clamp(0.0, 75.0)
        } else {
            0.0
        };
        let stability_score = (100.0 - penalty).round().clamp(15.0, 100.0) as u32;

        // Estimated seconds until reaching trip_c if heating steadily
        let seconds_to_trip =
            if trip_c.is_finite() && trip_c > temp_c && velocity_cpm >= 0.5 {
                (((trip_c - temp_c) / velocity_cpm) * 60.0).clamp(1.0, 3600.0)
            } else {
                -1.0
            };

        ThermalTrendResult {
            velocity_cpm,
            stability_score,
            seconds_to_trip,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn ols_detects_steady_heating_and_cooling_accurately() {
        let mut analyzer = new_thermal_trend_analyzer();
        // Heating at +6.0 C/min (+0.1 C per second)
        let mut last = ThermalTrendResult {
            velocity_cpm: 0.0,
            stability_score: 100,
            seconds_to_trip: -1.0,
        };
        for i in 0..10u64 {
            last = analyzer.push_sample(1_000 + i * 1_000, 40.0 + (i as f64) * 0.1, 55.0);
        }
        assert!((last.velocity_cpm - 6.0).abs() < 0.05);
        assert!(last.stability_score < 50);
        assert!(last.seconds_to_trip > 100.0 && last.seconds_to_trip < 200.0);

        analyzer.reset();
        // Cooling at -3.0 C/min (-0.05 C per second)
        for i in 0..8u64 {
            last = analyzer.push_sample(50_000 + i * 1_000, 45.0 - (i as f64) * 0.05, 55.0);
        }
        assert!((last.velocity_cpm + 3.0).abs() < 0.05);
        assert_eq!(last.stability_score, 100);
        assert!(last.seconds_to_trip < 0.0);
    }

    #[test]
    fn rejects_nan_and_non_monotonic_timestamps_gracefully() {
        let mut analyzer = new_thermal_trend_analyzer();
        let bad = analyzer.push_sample(1000, f64::NAN, 55.0);
        assert_eq!(bad.velocity_cpm, 0.0);
        assert_eq!(bad.stability_score, 100);
    }
}
