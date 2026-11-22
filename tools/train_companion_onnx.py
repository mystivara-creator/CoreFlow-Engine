#!/usr/bin/env python3
"""
CFAE System Framework — Phase B Companion App On-Demand ONNX Trainer & Exporter.

Trains two compact FP32 ONNX opset-13 models with zero third-party pip dependencies
(pure Python standard library + spec-compliant ONNX IR v7 / opset 13 protobuf writer):
  1. `android/src/main/assets/cfae_nlu_intent.onnx`       (128 -> 32 -> 11)
     Bilingual Indonesian + English free-form NLU intent classifier for the
     CFAE TELEMETRY LINK Sovereign Dialogue Console.
  2. `android/src/main/assets/cfae_session_anomaly.onnx`  (8 -> 16 -> 5)
     On-demand telemetry session anomaly root-cause classifier.

Usage:
  python3 tools/train_companion_onnx.py
"""
from __future__ import annotations

import math
import random
import re
import struct
import sys
from pathlib import Path
from typing import Dict, List, Sequence, Tuple

# ---------------------------------------------------------------------------
# Minimal Spec-Compliant ONNX IR v7 / Opset 13 Protobuf Serializer
# ---------------------------------------------------------------------------
def _pb_varint(v: int) -> bytes:
    if v < 0:
        v &= (1 << 64) - 1
    out = bytearray()
    while True:
        b = v & 0x7F
        v >>= 7
        if v:
            out.append(b | 0x80)
        else:
            out.append(b)
            break
    return bytes(out)


def _pb_tag(field_num: int, wire_type: int) -> bytes:
    return _pb_varint((field_num << 3) | wire_type)


def _pb_int64(field_num: int, value: int) -> bytes:
    return _pb_tag(field_num, 0) + _pb_varint(value)


def _pb_bytes(field_num: int, data: bytes) -> bytes:
    return _pb_tag(field_num, 2) + _pb_varint(len(data)) + data


def _pb_str(field_num: int, text: str) -> bytes:
    return _pb_bytes(field_num, text.encode("utf-8"))


def _make_tensor_proto(name: str, dims: Sequence[int], values: Sequence[float]) -> bytes:
    parts = bytearray()
    for d in dims:
        parts.extend(_pb_int64(1, d))  # dims
    parts.extend(_pb_int64(2, 1))      # data_type = FLOAT (1)
    parts.extend(_pb_str(8, name))     # name
    raw = struct.pack(f"<{len(values)}f", *values)
    parts.extend(_pb_bytes(9, raw))    # raw_data
    return bytes(parts)


def _make_value_info(name: str, dims: Sequence[int]) -> bytes:
    shape = bytearray()
    for d in dims:
        dim_msg = _pb_int64(1, d)      # Dimension.dim_value
        shape.extend(_pb_bytes(1, dim_msg))  # TensorShapeProto.dim
    tensor_type = bytearray()
    tensor_type.extend(_pb_int64(1, 1))      # elem_type = FLOAT (1)
    tensor_type.extend(_pb_bytes(2, bytes(shape)))  # shape
    type_proto = _pb_bytes(1, bytes(tensor_type))   # TypeProto.tensor_type
    vi = bytearray()
    vi.extend(_pb_str(1, name))
    vi.extend(_pb_bytes(2, type_proto))
    return bytes(vi)


def _make_int_attr(name: str, value: int) -> bytes:
    attr = bytearray()
    attr.extend(_pb_str(1, name))      # name
    attr.extend(_pb_int64(3, value))   # i
    attr.extend(_pb_int64(20, 2))      # type = INT (2)
    return bytes(attr)


def _make_node(
    op_type: str,
    name: str,
    inputs: Sequence[str],
    outputs: Sequence[str],
    attrs: Sequence[bytes] = (),
) -> bytes:
    node = bytearray()
    for inp in inputs:
        node.extend(_pb_str(1, inp))
    for out in outputs:
        node.extend(_pb_str(2, out))
    node.extend(_pb_str(3, name))
    node.extend(_pb_str(4, op_type))
    for a in attrs:
        node.extend(_pb_bytes(5, a))
    return bytes(node)


def export_mlp_onnx(
    out_path: Path,
    graph_name: str,
    in_dim: int,
    h_dim: int,
    out_dim: int,
    w1: Sequence[float],
    b1: Sequence[float],
    w2: Sequence[float],
    b2: Sequence[float],
) -> bytes:
    assert len(w1) == h_dim * in_dim
    assert len(b1) == h_dim
    assert len(w2) == out_dim * h_dim
    assert len(b2) == out_dim

    n1 = _make_node("Gemm", "gemm1", ["features", "w1", "b1"], ["z1"], [_make_int_attr("transB", 1)])
    n2 = _make_node("Tanh", "tanh1", ["z1"], ["h1"])
    n3 = _make_node("Gemm", "gemm2", ["h1", "w2", "b2"], ["logits"], [_make_int_attr("transB", 1)])

    inits = [
        _make_tensor_proto("w1", [h_dim, in_dim], w1),
        _make_tensor_proto("b1", [h_dim], b1),
        _make_tensor_proto("w2", [out_dim, h_dim], w2),
        _make_tensor_proto("b2", [out_dim], b2),
    ]

    graph = bytearray()
    for node in (n1, n2, n3):
        graph.extend(_pb_bytes(1, node))
    graph.extend(_pb_str(2, graph_name))
    for init in inits:
        graph.extend(_pb_bytes(5, init))
    graph.extend(_pb_bytes(11, _make_value_info("features", [1, in_dim])))
    graph.extend(_pb_bytes(12, _make_value_info("logits", [1, out_dim])))

    opset = _pb_str(1, "") + _pb_int64(2, 13)  # ai.onnx opset 13

    model = bytearray()
    model.extend(_pb_int64(1, 7))  # ir_version = 7
    model.extend(_pb_str(2, "cfae-companion-trainer"))
    model.extend(_pb_str(3, "6.0.0-R"))
    model.extend(_pb_bytes(7, bytes(graph)))
    model.extend(_pb_bytes(8, opset))

    blob = bytes(model)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(blob)
    return blob


# ---------------------------------------------------------------------------
# Shared Deterministic FNV-1a Feature Hashing (100% parity with Kotlin)
# ---------------------------------------------------------------------------
NLU_DIM = 256
NLU_HIDDEN = 32

INTENTS = [
    "CONTEXT_SHIFT_GAMING",      # 0
    "CONTEXT_SHIFT_BATTERY",     # 1
    "CONTEXT_SHIFT_SMOOTHNESS",  # 2
    "CONTEXT_SHIFT_BALANCED",    # 3
    "TAMPER_RESYNC",             # 4
    "DIAGNOSE_STUTTER",          # 5
    "DIAGNOSE_THERMAL_LOAD",     # 6
    "DIAGNOSE_CONFLICT_TAMPER",  # 7
    "DIAGNOSE_LEARNING_DNA",     # 8
    "REJECT_MANUAL_TWEAK",       # 9
    "GENERAL_SUMMARY",           # 10
]


def fnv1a_32(text: str) -> int:
    h = 0x811C9DC5
    for b in text.encode("utf-8"):
        h ^= b
        h = (h * 0x01000193) & 0xFFFFFFFF
    return h


def featurize_query(raw: str) -> List[float]:
    cleaned = re.sub(r"[^a-z0-9]+", " ", raw.strip().lower()).strip()
    vec = [0.0] * NLU_DIM
    if not cleaned:
        return vec
    tokens = [t for t in cleaned.split(" ") if t]

    def add_feat(key: str, weight: float) -> None:
        h = fnv1a_32(key)
        bucket = h & (NLU_DIM - 1)
        sign = 1.0 if ((h >> 8) & 1) == 0 else -1.0
        vec[bucket] += sign * weight

    for tok in tokens:
        if len(tok) >= 2:
            add_feat(f"w:{tok}", 1.5)
        padded = f"^{tok}$"
        if len(padded) >= 3:
            for i in range(len(padded) - 2):
                add_feat(f"g:{padded[i:i+3]}", 0.6)
    for i in range(len(tokens) - 1):
        add_feat(f"b:{tokens[i]}_{tokens[i+1]}", 1.8)

    norm = math.sqrt(sum(v * v for v in vec))
    if norm > 1e-6:
        inv = 1.0 / norm
        vec = [v * inv for v in vec]
    return vec


# ---------------------------------------------------------------------------
# Compact 2-Layer Softmax Cross-Entropy Trainer
# ---------------------------------------------------------------------------
def softmax(logits: Sequence[float]) -> List[float]:
    m = max(logits)
    exps = [math.exp(max(-30.0, min(30.0, x - m))) for x in logits]
    s = sum(exps)
    return [e / s for e in exps]


def train_classifier(
    samples: Sequence[Tuple[List[float], int]],
    in_dim: int,
    h_dim: int,
    out_dim: int,
    epochs: int = 240,
    lr: float = 0.08,
    l2: float = 2e-4,
    seed: int = 2026,
) -> Tuple[List[float], List[float], List[float], List[float], float]:
    rng = random.Random(seed)
    scale1 = math.sqrt(2.0 / (in_dim + h_dim))
    scale2 = math.sqrt(2.0 / (h_dim + out_dim))
    w1 = [rng.uniform(-scale1, scale1) for _ in range(h_dim * in_dim)]
    b1 = [0.0] * h_dim
    w2 = [rng.uniform(-scale2, scale2) for _ in range(out_dim * h_dim)]
    b2 = [0.0] * out_dim

    order = list(range(len(samples)))
    for epoch in range(epochs):
        rng.shuffle(order)
        step = lr / (1.0 + 0.008 * epoch)
        for idx in order:
            x, y = samples[idx]
            h = [0.0] * h_dim
            for i in range(h_dim):
                row = i * in_dim
                acc = b1[i]
                for k in range(in_dim):
                    acc += w1[row + k] * x[k]
                h[i] = math.tanh(acc)

            logits = [0.0] * out_dim
            for o in range(out_dim):
                row = o * h_dim
                acc = b2[o]
                for k in range(h_dim):
                    acc += w2[row + k] * h[k]
                logits[o] = acc

            probs = softmax(logits)
            d_logits = [probs[o] - (1.0 if o == y else 0.0) for o in range(out_dim)]

            d_h = [0.0] * h_dim
            for o in range(out_dim):
                row = o * h_dim
                g = d_logits[o]
                for k in range(h_dim):
                    d_h[k] += w2[row + k] * g
                    w2[row + k] -= step * (g * h[k] + l2 * w2[row + k])
                b2[o] -= step * g

            for i in range(h_dim):
                dz = d_h[i] * (1.0 - h[i] * h[i])
                row = i * in_dim
                for k in range(in_dim):
                    w1[row + k] -= step * (dz * x[k] + l2 * w1[row + k])
                b1[i] -= step * dz

    correct = 0
    for x, y in samples:
        h = [math.tanh(b1[i] + sum(w1[i * in_dim + k] * x[k] for k in range(in_dim))) for i in range(h_dim)]
        logits = [b2[o] + sum(w2[o * h_dim + k] * h[k] for k in range(h_dim)) for o in range(out_dim)]
        pred = max(range(out_dim), key=lambda o: logits[o])
        if pred == y:
            correct += 1
    acc = correct / max(1, len(samples))
    return w1, b1, w2, b2, acc


def build_nlu_dataset() -> List[Tuple[List[float], int]]:
    utterances: Dict[str, List[str]] = {
        "CONTEXT_SHIFT_GAMING": [
            "aku mau push rank sekarang",
            "mabar genshin berat nih",
            "mabar mlbb biar ngebut",
            "gas main pubg sama ff",
            "siapin performa buat main game",
            "aktifkan mode gaming dong",
            "tolong kasih tenaga penuh buat game",
            "frame rate game biar stabil pas mabar",
            "butuh performa tinggi buat turnamen game",
            "switch to gaming profile please",
            "prepare device for heavy gaming session",
            "boost frame pacing for genshin impact",
            "i am about to play competitive matches",
            "turn on game performance profile",
            "set context for intense 3d gaming",
        ],
        "CONTEXT_SHIFT_BATTERY": [
            "hp mulai anget nih bikin adem dikit",
            "mode hemat baterai aja ya",
            "batre lagi boros banget tolong irit",
            "hp kerasa hangat kurangi panasnya",
            "lagi di jalan bikin baterai awet seharian",
            "dinginkan hp sekarang lagi gerah",
            "prioritaskan daya tahan baterai dulu",
            "tolong irit daya baterai hari ini",
            "phone feels warm please cool it down",
            "save battery life for the rest of the day",
            "switch to low power efficiency mode",
            "reduce heat and conserve battery",
            "keep device cool and extend endurance",
            "battery is draining fast make it last longer",
        ],
        "CONTEXT_SHIFT_SMOOTHNESS": [
            "scroll medsos biar mulus banget",
            "buat animasi layar dan ui jadi licin",
            "pengen scrolling halus tanpa jeda",
            "fokus ke kehalusan sentuhan layar",
            "mode mulus untuk baca dan browsing",
            "prioritaskan kelancaran ui dan transisi",
            "biar geser layar super halus dan responsif",
            "make scrolling butter smooth please",
            "focus on ui fluidity and touch smoothness",
            "switch to smoothness mode for daily apps",
            "keep animations silky and responsive",
            "prioritize fluid scrolling experience",
        ],
        "CONTEXT_SHIFT_BALANCED": [
            "balik ke mode otomatis seimbang",
            "normalin lagi profilnya",
            "pakai pengaturan standar otomatis aja",
            "kembali ke balanced seperti biasa",
            "sudah selesai main balik normal",
            "sudah beres mabar normalin lagi",
            "return to balanced automatic mode",
            "back to normal default profile",
            "restore balanced daily operation",
            "use standard adaptive mode again",
            "done gaming go back to balanced",
        ],
        "TAMPER_RESYNC": [
            "rebut balik semua node yang diganggu",
            "kunci ulang izin 0444 sekarang",
            "sinkron ulang paksa semua parameter",
            "paksa resync sovereign guard",
            "pulihkan penguncian node sysfs",
            "force resync and reclaim all leased nodes",
            "re-lock 0444 permissions immediately",
            "enforce sovereign tamper recovery now",
            "clear quarantine and reclaim sysfs",
            "run full resync on tampered knobs",
        ],
        "DIAGNOSE_STUTTER": [
            "kenapa barusan agak patah patah",
            "kok tadi nge lag pas buka galeri",
            "barusan drop fps kenapa ya",
            "ada jeda micro stutter saat scroll",
            "layar sempat macet sebentar tadi",
            "kenapa fps turun mendadak barusan",
            "tadi berasa nyendat pas pindah aplikasi",
            "why did frames drop just now",
            "explain recent stutter and jank spikes",
            "why is scrolling lagging a bit",
            "diagnose frame pacing hitch just now",
            "what caused the micro stutter",
            "is jank risk high right now",
        ],
        "DIAGNOSE_THERMAL_LOAD": [
            "status suhu dan beban sekarang gimana",
            "cek temperatur cpu gpu sama ram",
            "berapa derajat soc dan baterai saat ini",
            "prediksi suhu 10 detik ke depan gimana",
            "beban kerja perangkat lagi berat gak",
            "seberapa panas hp sekarang",
            "what is current temperature and cpu load",
            "check soc thermal pressure and gpu busy",
            "show thermal forecast and bus scaling",
            "how hot is the phone right now",
            "report current workload class and load",
            "inspect cpu gpu memory pressure now",
        ],
        "DIAGNOSE_CONFLICT_TAMPER": [
            "ada konflik modul lain atau intervensi gak",
            "siapa yang coba ubah sysfs barusan",
            "cek keamanan tamper guard dan modul bentrok",
            "apakah ada aplikasi root yang mengganggu",
            "lihat log gangguan eksternal hari ini",
            "any conflicting modules or external tampering",
            "check tamper guard events and blocked writes",
            "did another module fight coreflowd",
            "show forensic security audit of sysfs",
            "are there root conflicts detected",
        ],
        "DIAGNOSE_LEARNING_DNA": [
            "apa yang sudah dipelajari di hp ini",
            "berapa aplikasi yang sudah dikenali app dna",
            "status pembelajaran ai dan cfdna2 gimana",
            "model neural apa yang aktif sekarang",
            "lihat memori kebiasaan aplikasi saya",
            "what has cfae learned on this device",
            "how many apps are profiled in app dna",
            "show neural engine backend and learning stats",
            "check cfdna2 zero wear persistence status",
            "which ai model is currently running",
        ],
        "REJECT_MANUAL_TWEAK": [
            "kasih slider frekuensi cpu manual dong",
            "tolong overclock cpu ke maksimal paksa",
            "buka kunci 0444 biar bisa tweak sendiri",
            "matikan thermal guard dan proteksi suhu",
            "aktifkan mode yield biar modul lain bebas",
            "underclock gpu manual pakai angka sendiri",
            "give me manual frequency sliders",
            "overclock my cpu and disable thermal guard",
            "bypass guard and unlock 0444 permissions",
            "enable yield mode to let other tweaks write",
            "let me set custom clock speeds manually",
        ],
        "GENERAL_SUMMARY": [
            "halo ringkasan kondisi sistem sekarang",
            "semuanya aman dan sehat kan",
            "kasih laporan singkat cfae hari ini",
            "apa kabar kondisi daemon coreflow",
            "overview status keseluruhan perangkat",
            "hello give me a quick system summary",
            "is everything healthy and running fine",
            "overall coreflow status report please",
            "brief overview of daemon health",
            "how is the system doing overall",
        ],
    }

    rng = random.Random(999)
    samples: List[Tuple[List[float], int]] = []
    for idx, name in enumerate(INTENTS):
        for text in utterances[name]:
            samples.append((featurize_query(text), idx))
            # Token-dropout augmentation improves generalization to unseen colloquial phrases
            words = text.split()
            if len(words) >= 4:
                drop_idx = rng.randrange(len(words))
                aug = " ".join(w for j, w in enumerate(words) if j != drop_idx)
                samples.append((featurize_query(aug), idx))
    return samples


# ---------------------------------------------------------------------------
# Session Anomaly Classifier (8 -> 16 -> 5)
# Features: [peak_temp_norm, avg_temp_norm, temp_slope_norm, jank_ratio,
#            cpu_util_norm, thermal_guard_active, tamper_norm, charge_heat_norm]
# Classes:
#   0: HEALTHY
#   1: THERMAL_SATURATION
#   2: FAST_CHARGE_HEAT
#   3: STUTTER_IO_CONTENTION
#   4: EXTERNAL_TAMPER
# ---------------------------------------------------------------------------
ANOMALY_CLASSES = [
    "HEALTHY",
    "THERMAL_SATURATION",
    "FAST_CHARGE_HEAT",
    "STUTTER_IO_CONTENTION",
    "EXTERNAL_TAMPER",
]


def build_anomaly_dataset() -> List[Tuple[List[float], int]]:
    rng = random.Random(77)
    samples: List[Tuple[List[float], int]] = []

    def jitter(base: List[float], cls: int, count: int = 24) -> None:
        for _ in range(count):
            row = [max(0.0, min(1.5, v + rng.uniform(-0.07, 0.07))) for v in base]
            samples.append((row, cls))

    # 0: HEALTHY (cool/moderate temps, low jank, no guard/tamper)
    jitter([0.25, 0.20, 0.05, 0.02, 0.22, 0.0, 0.0, 0.0], 0, 30)
    jitter([0.40, 0.35, 0.10, 0.05, 0.45, 0.0, 0.0, 0.10], 0, 30)
    # 1: THERMAL_SATURATION (high peak/avg temp + thermal guard or rapid rise, not charging)
    jitter([0.92, 0.82, 0.65, 0.22, 0.78, 1.0, 0.0, 0.05], 1, 30)
    jitter([0.85, 0.75, 0.55, 0.15, 0.85, 0.8, 0.0, 0.0], 1, 30)
    # 2: FAST_CHARGE_HEAT (elevated temp specifically correlated with high charging heat)
    jitter([0.78, 0.70, 0.55, 0.10, 0.35, 0.5, 0.0, 0.95], 2, 30)
    jitter([0.65, 0.58, 0.45, 0.08, 0.25, 0.2, 0.0, 0.85], 2, 30)
    # 3: STUTTER_IO_CONTENTION (elevated jank despite cool/moderate thermal headroom)
    jitter([0.38, 0.32, 0.10, 0.55, 0.65, 0.0, 0.0, 0.05], 3, 30)
    jitter([0.45, 0.40, 0.15, 0.75, 0.55, 0.0, 0.0, 0.10], 3, 30)
    # 4: EXTERNAL_TAMPER (tamper events or conflicting modules present)
    jitter([0.45, 0.40, 0.15, 0.18, 0.45, 0.0, 0.85, 0.05], 4, 30)
    jitter([0.60, 0.55, 0.25, 0.30, 0.60, 0.4, 1.00, 0.10], 4, 30)
    return samples


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    assets = repo / "android/src/main/assets"

    nlu_data = build_nlu_dataset()
    w1, b1, w2, b2, nlu_acc = train_classifier(
        nlu_data, in_dim=NLU_DIM, h_dim=NLU_HIDDEN, out_dim=len(INTENTS), epochs=260, lr=0.09, seed=2026
    )
    nlu_blob = export_mlp_onnx(
        assets / "cfae_nlu_intent.onnx",
        "cfae_nlu_intent_mlp",
        NLU_DIM,
        NLU_HIDDEN,
        len(INTENTS),
        w1,
        b1,
        w2,
        b2,
    )

    anom_data = build_anomaly_dataset()
    aw1, ab1, aw2, ab2, anom_acc = train_classifier(
        anom_data, in_dim=8, h_dim=16, out_dim=len(ANOMALY_CLASSES), epochs=220, lr=0.06, seed=4096
    )
    anom_blob = export_mlp_onnx(
        assets / "cfae_session_anomaly.onnx",
        "cfae_session_anomaly_mlp",
        8,
        16,
        len(ANOMALY_CLASSES),
        aw1,
        ab1,
        aw2,
        ab2,
    )

    print(f"Exported cfae_nlu_intent.onnx      ({len(nlu_blob)} B, train_acc={nlu_acc*100:.1f}%, samples={len(nlu_data)})")
    print(f"Exported cfae_session_anomaly.onnx ({len(anom_blob)} B, train_acc={anom_acc*100:.1f}%, samples={len(anom_data)})")
    assert nlu_acc >= 0.99, f"Expected NLU train accuracy >= 99%, got {nlu_acc}"
    assert anom_acc >= 0.97, f"Expected Anomaly train accuracy >= 97%, got {anom_acc}"
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
