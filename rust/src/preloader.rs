use crate::bridge::ffi::PreloadPrediction;

const MAX_SLOTS: usize = 16;
const MIN_TRANSITIONS_FOR_PREDICTION: u32 = 3;
const MIN_CONFIDENCE: f64 = 0.45;

/// Fixed-capacity, zero-allocation Markov + recency app transition predictor (< 1 KB RAM).
/// Learns the user's real daily app-switching habits using 64-bit package hashes and predicts
/// the next likely foreground app for bounded kernel page-cache warm-up (`posix_fadvise`).
pub struct AppPreloader {
    hashes: [u64; MAX_SLOTS],
    recency: [u64; MAX_SLOTS],
    transitions: [[u16; MAX_SLOTS]; MAX_SLOTS],
    last_slot: Option<usize>,
    tick: u64,
}

pub fn new_app_preloader() -> Box<AppPreloader> {
    Box::new(AppPreloader {
        hashes: [0; MAX_SLOTS],
        recency: [0; MAX_SLOTS],
        transitions: [[0; MAX_SLOTS]; MAX_SLOTS],
        last_slot: None,
        tick: 0,
    })
}

impl AppPreloader {
    pub fn reset(&mut self) {
        self.hashes = [0; MAX_SLOTS];
        self.recency = [0; MAX_SLOTS];
        self.transitions = [[0; MAX_SLOTS]; MAX_SLOTS];
        self.last_slot = None;
        self.tick = 0;
    }

    fn find_slot(&self, pkg_hash: u64) -> Option<usize> {
        if pkg_hash == 0 {
            return None;
        }
        self.hashes.iter().position(|&h| h == pkg_hash)
    }

    fn intern_slot(&mut self, pkg_hash: u64) -> Option<usize> {
        if pkg_hash == 0 {
            return None;
        }
        if let Some(idx) = self.find_slot(pkg_hash) {
            self.tick = self.tick.saturating_add(1);
            self.recency[idx] = self.tick;
            return Some(idx);
        }
        // Pick an empty slot or evict the least-recently used slot (never evicting last_slot).
        let slot = if let Some(empty) = self.hashes.iter().position(|&h| h == 0) {
            empty
        } else {
            let mut lru_idx = 0usize;
            let mut lru_val = u64::MAX;
            for i in 0..MAX_SLOTS {
                if self.last_slot == Some(i) {
                    continue;
                }
                if self.recency[i] < lru_val {
                    lru_val = self.recency[i];
                    lru_idx = i;
                }
            }
            lru_idx
        };

        // Clear stale transition counts for the evicted slot.
        self.hashes[slot] = pkg_hash;
        self.tick = self.tick.saturating_add(1);
        self.recency[slot] = self.tick;
        for i in 0..MAX_SLOTS {
            self.transitions[slot][i] = 0;
            self.transitions[i][slot] = 0;
        }
        Some(slot)
    }

    pub fn record_launch(&mut self, pkg_hash: u64) {
        let Some(curr) = self.intern_slot(pkg_hash) else {
            return;
        };
        if let Some(prev) = self.last_slot {
            if prev != curr {
                let cell = &mut self.transitions[prev][curr];
                *cell = cell.saturating_add(1);
                // Gentle halving decay keeps weights responsive to changing daily habits.
                if *cell >= 32 {
                    for count in &mut self.transitions[prev] {
                        *count /= 2;
                    }
                }
            }
        }
        self.last_slot = Some(curr);
    }

    pub fn predict_next(&self, current_pkg_hash: u64) -> PreloadPrediction {
        let Some(curr) = self.find_slot(current_pkg_hash) else {
            return PreloadPrediction {
                has_prediction: false,
                target_hash: 0,
                confidence: 0.0,
            };
        };

        let mut total: u32 = 0;
        let mut best_idx: usize = 0;
        let mut best_count: u16 = 0;
        for (idx, &count) in self.transitions[curr].iter().enumerate() {
            if idx == curr || self.hashes[idx] == 0 {
                continue;
            }
            total += u32::from(count);
            if count > best_count
                || (count == best_count
                    && count > 0
                    && self.recency[idx] > self.recency[best_idx])
            {
                best_count = count;
                best_idx = idx;
            }
        }

        if total < MIN_TRANSITIONS_FOR_PREDICTION || best_count == 0 {
            return PreloadPrediction {
                has_prediction: false,
                target_hash: 0,
                confidence: 0.0,
            };
        }

        let confidence = f64::from(best_count) / f64::from(total);
        if confidence < MIN_CONFIDENCE {
            return PreloadPrediction {
                has_prediction: false,
                target_hash: 0,
                confidence,
            };
        }

        PreloadPrediction {
            has_prediction: true,
            target_hash: self.hashes[best_idx],
            confidence,
        }
    }

    /// Exports the compact 16-slot Markov transition state for atomic cross-reboot persistence.
    pub fn export_state(&self) -> String {
        use std::fmt::Write;
        let mut out = String::with_capacity(512);
        let _ = writeln!(out, "CFM1 {}", self.tick);
        for i in 0..MAX_SLOTS {
            if self.hashes[i] != 0 {
                let _ = writeln!(out, "S {} {} {}", i, self.hashes[i], self.recency[i]);
            }
        }
        for from in 0..MAX_SLOTS {
            if self.hashes[from] == 0 {
                continue;
            }
            for to in 0..MAX_SLOTS {
                let cnt = self.transitions[from][to];
                if cnt > 0 && self.hashes[to] != 0 {
                    let _ = writeln!(out, "E {} {} {}", from, to, cnt);
                }
            }
        }
        out
    }

    /// Restores a previously exported Markov state. Rejects malformed payloads safely.
    pub fn import_state(&mut self, payload: &str) -> bool {
        let mut lines = payload.lines();
        let Some(header) = lines.next() else {
            return false;
        };
        let mut h_parts = header.split_whitespace();
        if h_parts.next() != Some("CFM1") {
            return false;
        }
        let Ok(parsed_tick) = h_parts.next().unwrap_or("0").parse::<u64>() else {
            return false;
        };

        let mut next_hashes = [0u64; MAX_SLOTS];
        let mut next_recency = [0u64; MAX_SLOTS];
        let mut next_trans = [[0u16; MAX_SLOTS]; MAX_SLOTS];

        for line in lines {
            let trimmed = line.trim();
            if trimmed.is_empty() {
                continue;
            }
            let parts: Vec<&str> = trimmed.split_whitespace().collect();
            match parts.as_slice() {
                ["S", idx_s, hash_s, rec_s] => {
                    let (Ok(idx), Ok(hash), Ok(rec)) = (
                        idx_s.parse::<usize>(),
                        hash_s.parse::<u64>(),
                        rec_s.parse::<u64>(),
                    ) else {
                        continue;
                    };
                    if idx < MAX_SLOTS && hash != 0 {
                        next_hashes[idx] = hash;
                        next_recency[idx] = rec;
                    }
                }
                ["E", from_s, to_s, cnt_s] => {
                    let (Ok(from), Ok(to), Ok(cnt)) = (
                        from_s.parse::<usize>(),
                        to_s.parse::<usize>(),
                        cnt_s.parse::<u16>(),
                    ) else {
                        continue;
                    };
                    if from < MAX_SLOTS && to < MAX_SLOTS && from != to && cnt > 0 {
                        next_trans[from][to] = cnt.min(64);
                    }
                }
                _ => continue,
            }
        }

        self.hashes = next_hashes;
        self.recency = next_recency;
        self.transitions = next_trans;
        self.last_slot = None;
        self.tick = parsed_tick;
        true
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn learns_markov_transition_and_respects_confidence_threshold() {
        let mut p = new_app_preloader();
        let launcher = 0x1111u64;
        let whatsapp = 0x2222u64;
        let camera = 0x3333u64;

        // Under min transitions -> no prediction yet
        p.record_launch(launcher);
        p.record_launch(whatsapp);
        p.record_launch(launcher);
        p.record_launch(whatsapp);
        assert!(!p.predict_next(launcher).has_prediction);

        // Third transition crosses min threshold with high confidence
        p.record_launch(launcher);
        p.record_launch(whatsapp);
        let pred = p.predict_next(launcher);
        assert!(pred.has_prediction);
        assert_eq!(pred.target_hash, whatsapp);
        assert!((pred.confidence - 1.0).abs() < 1e-9);

        // One rare jump to camera keeps whatsapp dominant (3/4 = 0.75)
        p.record_launch(launcher);
        p.record_launch(camera);
        let pred2 = p.predict_next(launcher);
        assert!(pred2.has_prediction);
        assert_eq!(pred2.target_hash, whatsapp);
        assert!((pred2.confidence - 0.75).abs() < 1e-9);
    }

    #[test]
    fn export_and_import_preserves_markov_predictions_across_reboot() {
        let mut p1 = new_app_preloader();
        for _ in 0..4 {
            p1.record_launch(0xAAAA);
            p1.record_launch(0xBBBB);
        }
        let blob = p1.export_state();
        let mut p2 = new_app_preloader();
        assert!(p2.import_state(&blob));
        let pred = p2.predict_next(0xAAAA);
        assert!(pred.has_prediction);
        assert_eq!(pred.target_hash, 0xBBBB);
        assert!((pred.confidence - 1.0).abs() < 1e-9);
        assert!(!p2.import_state("CORRUPT_HEADER"));
    }

    #[test]
    fn lru_eviction_and_decay_stay_bounded_and_panic_free() {
        let mut p = new_app_preloader();
        for i in 1..=40u64 {
            p.record_launch(i);
            p.record_launch(i + 1000);
        }
        // Heavy repetition triggers halving decay safely without overflow
        for _ in 0..80 {
            p.record_launch(9999);
            p.record_launch(8888);
        }
        let pred = p.predict_next(9999);
        assert!(pred.has_prediction);
        assert_eq!(pred.target_hash, 8888);
        assert!((0.0..=1.0).contains(&pred.confidence));

        // Zero hash is safely ignored
        p.record_launch(0);
        assert!(!p.predict_next(0).has_prediction);
    }
}
