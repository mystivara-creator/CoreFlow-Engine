#!/usr/bin/env python3
"""
CFAE System Framework — Offline CFW2 Micro-MLP Trainer from Real `--diag` Telemetry.

Warm-starts from the physics-calibrated 10 -> 16 -> 12 -> 3 baseline MLP, extracts
real 10-second thermal/load trajectories from one or more `coreflowd --diag` JSON
captures, blends them with safety boundary anchors so extreme thermal/jank regimes
never catastrophically regress, fine-tunes with deterministic backpropagation, and
exports a CRC32-verified `CFW2` micro-model (~1.7 KB) for `coreflowd`.

Zero third-party dependencies are required for training and `CFW2` export.
Optional `--export-onnx` exports an ONNX opset-13 graph when `onnx` + `numpy` are installed.

Usage:
  python3 tools/train_cfw2_from_diag.py \
      --diag coreflow-diag-v6.0.0-R.json \
      -o equilibrium.cfw2
"""
from __future__ import annotations

import argparse
import json
import math
import random
import sys
from pathlib import Path
from typing import List, Sequence, Tuple

# Allow running directly as `python3 tools/train_cfw2_from_diag.py`
SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from onnx_to_cfw2 import (  # noqa: E402
    CFW2_H1_DIM,
    CFW2_H2_DIM,
    CFW2_IN_DIM,
    CFW2_MAX_ABS_WEIGHT,
    CFW2_OUT_DIM,
    build_physics_baseline_weights,
    pack_cfw2,
    unpack_cfw2,
)

Sample = Tuple[List[float], List[float]]  # (features[10], target_logits[3])


def clamp(x: float, lo: float, hi: float) -> float:
    return lo if x < lo else (hi if x > hi else x)


def safe_float(val: object, default: float) -> float:
    if val is None or isinstance(val, bool):
        return default
    try:
        out = float(val)  # type: ignore[arg-type]
    except (TypeError, ValueError):
        return default
    return out if math.isfinite(out) else default


def logit(p: float) -> float:
    p_safe = clamp(p, 0.02, 0.98)
    return math.log(p_safe / (1.0 - p_safe))


def activate(x: float, use_silu: bool) -> float:
    if not use_silu:
        return math.tanh(x)
    c = clamp(x, -12.0, 12.0)
    return c / (1.0 + math.exp(-c))


def activate_deriv(x: float, y: float, use_silu: bool) -> float:
    if not use_silu:
        return 1.0 - y * y
    c = clamp(x, -12.0, 12.0)
    sig = 1.0 / (1.0 + math.exp(-c))
    return sig * (1.0 + c * (1.0 - sig))


def physics_boundary_samples() -> List[Sample]:
    """
    Deterministic anchor points covering the full operating envelope so fine-tuning
    on calm device captures preserves proactive thermal/jank safety guarantees.
    Feature order matches NeuralPredictor::infer_and_learn:
      [norm_p, norm_vel, norm_cpu, norm_mem, norm_io,
       norm_gpu, norm_render, norm_jank, norm_scale, norm_charge]
    Target order:
      [out0 (tanh(out0)*10 == delta_p_10s), out1 (sigmoid -> jank_risk), out2 (0.4+0.6*sigmoid -> scale)]
    """
    anchors: List[Sample] = [
        # Cool idle: slight cooling, very low jank, high headroom
        ([-0.60, -0.20, 0.04, 0.00, 0.00, 0.05, 0.02, 0.00, 1.00, 0.00], [-0.25, -2.60, 2.20]),
        ([-0.40, 0.00, 0.08, 0.02, 0.02, 0.08, 0.05, 0.02, 1.00, 0.00], [-0.10, -2.20, 1.80]),
        # Balanced interactive UI scroll
        ([-0.15, 0.10, 0.30, 0.05, 0.05, 0.25, 0.35, 0.08, 0.95, 0.00], [0.10, -1.20, 1.20]),
        ([0.10, 0.20, 0.45, 0.10, 0.08, 0.40, 0.55, 0.20, 0.90, 0.10], [0.28, -0.35, 0.65]),
        # High UI jank while thermally cool (rescue window: high jank risk, preserve headroom)
        ([-0.30, 0.05, 0.55, 0.20, 0.15, 0.35, 0.85, 0.75, 0.80, 0.00], [0.15, 1.85, 0.90]),
        # IO-bound stall
        ([0.15, 0.15, 0.30, 0.85, 0.95, 0.15, 0.60, 0.65, 0.85, 0.00], [0.35, 1.60, -0.20]),
        # Sustained heavy GPU gaming (rising heat)
        ([0.50, 0.60, 0.75, 0.25, 0.20, 0.88, 0.70, 0.30, 0.95, 0.15], [0.75, 0.55, -0.65]),
        # Fast-charging while moderately loaded
        ([0.35, 0.55, 0.40, 0.10, 0.10, 0.35, 0.25, 0.10, 0.90, 1.15], [0.70, -0.40, -0.55]),
        # Rapid thermal surge (must predict surge and lower recommended scale)
        ([0.75, 1.25, 0.90, 0.40, 0.35, 0.80, 0.65, 0.45, 0.85, 0.60], [1.25, 1.10, -1.65]),
        ([1.05, 1.40, 1.10, 0.55, 0.45, 0.90, 0.75, 0.60, 0.75, 0.80], [1.55, 1.65, -2.35]),
        # Hot but actively cooling down (recovery)
        ([0.55, -0.85, 0.20, 0.05, 0.05, 0.15, 0.10, 0.05, 0.60, 0.00], [-0.75, -1.50, 0.35]),
        ([0.20, -0.60, 0.10, 0.00, 0.00, 0.10, 0.05, 0.02, 0.75, 0.00], [-0.55, -1.90, 1.10]),
    ]
    return anchors


def extract_samples_from_diag(path: Path) -> List[Sample]:
    raw = json.loads(path.read_text(encoding="utf-8"))
    status = raw.get("status", raw) if isinstance(raw, dict) else {}
    if not isinstance(status, dict):
        return []

    history = status.get("history", [])
    if not isinstance(history, list) or len(history) < 12:
        return []

    base_mem_psi = max(0.0, safe_float(status.get("memPsi"), 0.0))
    base_io_psi = max(0.0, safe_float(status.get("ioPsi"), 0.0))
    base_gpu = clamp(safe_float(status.get("gpuBusyPct"), 0.0), 0.0, 100.0)
    base_render = clamp(safe_float(status.get("renderPressure"), 0.0), 0.0, 1.0)
    hyper = status.get("hyperFluid", {})
    base_jank = clamp(safe_float(hyper.get("jankRatio"), 0.0), 0.0, 1.0) if isinstance(hyper, dict) else 0.0
    battery = status.get("batteryPower", {})
    charge_mw = 0.0
    if isinstance(battery, dict):
        # Support either explicit power mW or voltage/current estimate
        if "powerMw" in battery and battery["powerMw"] is not None:
            charge_mw = max(0.0, safe_float(battery["powerMw"], 0.0))
        elif "chargingPowerMw" in battery and battery["chargingPowerMw"] is not None:
            charge_mw = max(0.0, safe_float(battery["chargingPowerMw"], 0.0))

    samples: List[Sample] = []
    n = len(history)
    for i in range(1, n):
        cur = history[i]
        prev = history[i - 1]
        if not isinstance(cur, dict) or not isinstance(prev, dict):
            continue
        ts_cur = safe_float(cur.get("ts"), float(i * 1000))
        ts_prev = safe_float(prev.get("ts"), float((i - 1) * 1000))
        raw_dt_sec = (ts_cur - ts_prev) / 1000.0
        dt = clamp(raw_dt_sec, 0.2, 30.0) if raw_dt_sec > 0.0 else 1.0

        # Find future point ~10s ahead (between 7s and 15s)
        future = None
        for j in range(i + 1, min(n, i + 20)):
            cand = history[j]
            if not isinstance(cand, dict):
                continue
            ts_j = safe_float(cand.get("ts"), float(j * 1000))
            horizon = (ts_j - ts_cur) / 1000.0
            if horizon >= 7.0:
                if horizon <= 15.0:
                    future = cand
                break
        if future is None and i + 10 < n and isinstance(history[i + 10], dict):
            future = history[i + 10]
        if future is None:
            continue

        p_cur = clamp(safe_float(cur.get("pressure"), 45.0), 0.0, 130.0)
        p_prev = clamp(safe_float(prev.get("pressure"), p_cur), 0.0, 130.0)
        p_fut = clamp(safe_float(future.get("pressure"), p_cur), 0.0, 130.0)
        cpu_pct = clamp(safe_float(cur.get("cpu"), 10.0), 0.0, 100.0)
        scale = clamp(safe_float(cur.get("scale"), 1.0), 0.4, 1.0)

        velocity = clamp((p_cur - p_prev) / dt, -10.0, 10.0)
        # Approximate CPU PSI from CPU load burst when per-sample PSI isn't stored in ring buffer
        est_cpu_psi = max(0.0, (cpu_pct - 35.0) * 0.5)

        feats = [
            clamp((p_cur - 60.0) / 40.0, -1.5, 2.0),
            clamp(velocity / 2.0, -2.0, 2.0),
            clamp(est_cpu_psi / 50.0, 0.0, 2.0),
            clamp(base_mem_psi / 25.0, 0.0, 2.0),
            clamp(base_io_psi / 25.0, 0.0, 2.0),
            clamp(base_gpu / 100.0, 0.0, 1.0),
            clamp(base_render, 0.0, 1.0),
            clamp(base_jank, 0.0, 1.0),
            clamp((scale - 0.4) / 0.6, 0.0, 1.0),
            clamp(charge_mw / 15000.0, 0.0, 1.5),
        ]

        delta_p_10s = clamp(p_fut - p_cur, -9.2, 9.2)
        target_out0 = math.atanh(clamp(delta_p_10s / 10.0, -0.92, 0.92))

        est_jank = clamp(base_jank + 0.15 * max(0.0, (cpu_pct - 75.0) / 25.0), 0.02, 0.95)
        target_out1 = logit(est_jank)

        # Target capacity ceiling: 1.0 below 55 pressure, tapering down to 0.45 at 105 pressure
        target_scale = clamp(1.0 - max(0.0, (p_fut - 55.0) / 90.0), 0.42, 0.98)
        norm_target_scale = clamp((target_scale - 0.40) / 0.60, 0.03, 0.97)
        target_out2 = logit(norm_target_scale)

        samples.append((feats, [target_out0, target_out1, target_out2]))

    return samples


class MicroMlp:
    def __init__(self, flat_weights: Sequence[float], use_silu: bool = False) -> None:
        self.use_silu = use_silu
        self.load_flat(flat_weights)

    def load_flat(self, flat_weights: Sequence[float]) -> None:
        idx = 0
        n_w1 = CFW2_H1_DIM * CFW2_IN_DIM
        self.w1 = list(flat_weights[idx : idx + n_w1])
        idx += n_w1
        self.b1 = list(flat_weights[idx : idx + CFW2_H1_DIM])
        idx += CFW2_H1_DIM
        n_w2 = CFW2_H2_DIM * CFW2_H1_DIM
        self.w2 = list(flat_weights[idx : idx + n_w2])
        idx += n_w2
        self.b2 = list(flat_weights[idx : idx + CFW2_H2_DIM])
        idx += CFW2_H2_DIM
        n_w3 = CFW2_OUT_DIM * CFW2_H2_DIM
        self.w3 = list(flat_weights[idx : idx + n_w3])
        idx += n_w3
        self.b3 = list(flat_weights[idx : idx + CFW2_OUT_DIM])

    def flatten(self) -> List[float]:
        return self.w1 + self.b1 + self.w2 + self.b2 + self.w3 + self.b3

    def forward(self, x: Sequence[float]) -> Tuple[List[float], List[float], List[float], List[float], List[float]]:
        z1 = [0.0] * CFW2_H1_DIM
        h1 = [0.0] * CFW2_H1_DIM
        for i in range(CFW2_H1_DIM):
            acc = self.b1[i]
            row = i * CFW2_IN_DIM
            for k in range(CFW2_IN_DIM):
                acc += self.w1[row + k] * x[k]
            z1[i] = acc
            h1[i] = activate(acc, self.use_silu)

        z2 = [0.0] * CFW2_H2_DIM
        h2 = [0.0] * CFW2_H2_DIM
        for i in range(CFW2_H2_DIM):
            acc = self.b2[i]
            row = i * CFW2_H1_DIM
            for k in range(CFW2_H1_DIM):
                acc += self.w2[row + k] * h1[k]
            z2[i] = acc
            h2[i] = activate(acc, self.use_silu)

        out = [0.0] * CFW2_OUT_DIM
        for o in range(CFW2_OUT_DIM):
            acc = self.b3[o]
            row = o * CFW2_H2_DIM
            for k in range(CFW2_H2_DIM):
                acc += self.w3[row + k] * h2[k]
            out[o] = acc
        return z1, h1, z2, h2, out

    def evaluate_mse(self, dataset: Sequence[Sample]) -> float:
        if not dataset:
            return 0.0
        total = 0.0
        for x, y in dataset:
            _, _, _, _, out = self.forward(x)
            for o in range(CFW2_OUT_DIM):
                diff = out[o] - y[o]
                total += diff * diff
        return total / (len(dataset) * CFW2_OUT_DIM)

    def train(
        self,
        dataset: Sequence[Sample],
        epochs: int,
        lr: float,
        weight_decay: float = 1e-4,
        seed: int = 42,
    ) -> Tuple[float, float]:
        rng = random.Random(seed)
        before_mse = self.evaluate_mse(dataset)
        best_mse = before_mse
        best_weights = self.flatten()
        order = list(range(len(dataset)))

        for epoch in range(epochs):
            rng.shuffle(order)
            step_lr = lr / (1.0 + 0.015 * epoch)
            for idx in order:
                x, target = dataset[idx]
                z1, h1, z2, h2, out = self.forward(x)

                d_out = [clamp((out[o] - target[o]) * (2.0 / CFW2_OUT_DIM), -1.0, 1.0) for o in range(CFW2_OUT_DIM)]

                d_h2 = [0.0] * CFW2_H2_DIM
                for o in range(CFW2_OUT_DIM):
                    row = o * CFW2_H2_DIM
                    g = d_out[o]
                    for k in range(CFW2_H2_DIM):
                        d_h2[k] += self.w3[row + k] * g
                        self.w3[row + k] = clamp(
                            self.w3[row + k] - step_lr * (g * h2[k] + weight_decay * self.w3[row + k]),
                            -CFW2_MAX_ABS_WEIGHT,
                            CFW2_MAX_ABS_WEIGHT,
                        )
                    self.b3[o] = clamp(self.b3[o] - step_lr * g, -CFW2_MAX_ABS_WEIGHT, CFW2_MAX_ABS_WEIGHT)

                d_z2 = [clamp(d_h2[i] * activate_deriv(z2[i], h2[i], self.use_silu), -1.0, 1.0) for i in range(CFW2_H2_DIM)]

                d_h1 = [0.0] * CFW2_H1_DIM
                for i in range(CFW2_H2_DIM):
                    row = i * CFW2_H1_DIM
                    g = d_z2[i]
                    for k in range(CFW2_H1_DIM):
                        d_h1[k] += self.w2[row + k] * g
                        self.w2[row + k] = clamp(
                            self.w2[row + k] - step_lr * (g * h1[k] + weight_decay * self.w2[row + k]),
                            -CFW2_MAX_ABS_WEIGHT,
                            CFW2_MAX_ABS_WEIGHT,
                        )
                    self.b2[i] = clamp(self.b2[i] - step_lr * g, -CFW2_MAX_ABS_WEIGHT, CFW2_MAX_ABS_WEIGHT)

                d_z1 = [clamp(d_h1[i] * activate_deriv(z1[i], h1[i], self.use_silu), -1.0, 1.0) for i in range(CFW2_H1_DIM)]

                for i in range(CFW2_H1_DIM):
                    row = i * CFW2_IN_DIM
                    g = d_z1[i]
                    for k in range(CFW2_IN_DIM):
                        self.w1[row + k] = clamp(
                            self.w1[row + k] - step_lr * (g * x[k] + weight_decay * self.w1[row + k]),
                            -CFW2_MAX_ABS_WEIGHT,
                            CFW2_MAX_ABS_WEIGHT,
                        )
                    self.b1[i] = clamp(self.b1[i] - step_lr * g, -CFW2_MAX_ABS_WEIGHT, CFW2_MAX_ABS_WEIGHT)

            epoch_mse = self.evaluate_mse(dataset)
            if math.isfinite(epoch_mse) and epoch_mse < best_mse:
                best_mse = epoch_mse
                best_weights = self.flatten()

        # Always retain the lowest-MSE finite checkpoint (prevents divergence on aggressive --lr)
        self.load_flat(best_weights)
        return before_mse, best_mse


def export_onnx_graph(mlp: MicroMlp, out_path: Path) -> None:
    try:
        import numpy as np  # type: ignore
        import onnx  # type: ignore
        from onnx import TensorProto, helper, numpy_helper  # type: ignore
    except ImportError as exc:
        raise RuntimeError(
            "Exporting `.onnx` requires `pip install onnx numpy`. "
            "Omit `--export-onnx` to generate zero-dependency `.cfw2` directly."
        ) from exc

    w1 = np.array(mlp.w1, dtype=np.float32).reshape(CFW2_H1_DIM, CFW2_IN_DIM)
    b1 = np.array(mlp.b1, dtype=np.float32)
    w2 = np.array(mlp.w2, dtype=np.float32).reshape(CFW2_H2_DIM, CFW2_H1_DIM)
    b2 = np.array(mlp.b2, dtype=np.float32)
    w3 = np.array(mlp.w3, dtype=np.float32).reshape(CFW2_OUT_DIM, CFW2_H2_DIM)
    b3 = np.array(mlp.b3, dtype=np.float32)

    initializers = [
        numpy_helper.from_array(w1, name="w1"),
        numpy_helper.from_array(b1, name="b1"),
        numpy_helper.from_array(w2, name="w2"),
        numpy_helper.from_array(b2, name="b2"),
        numpy_helper.from_array(w3, name="w3"),
        numpy_helper.from_array(b3, name="b3"),
    ]

    nodes = [helper.make_node("Gemm", ["features", "w1", "b1"], ["z1"], transB=1)]
    if mlp.use_silu:
        nodes.append(helper.make_node("Sigmoid", ["z1"], ["s1"]))
        nodes.append(helper.make_node("Mul", ["z1", "s1"], ["h1"]))
    else:
        nodes.append(helper.make_node("Tanh", ["z1"], ["h1"]))

    nodes.append(helper.make_node("Gemm", ["h1", "w2", "b2"], ["z2"], transB=1))
    if mlp.use_silu:
        nodes.append(helper.make_node("Sigmoid", ["z2"], ["s2"]))
        nodes.append(helper.make_node("Mul", ["z2", "s2"], ["h2"]))
    else:
        nodes.append(helper.make_node("Tanh", ["z2"], ["h2"]))

    nodes.append(helper.make_node("Gemm", ["h2", "w3", "b3"], ["logits"], transB=1))

    graph = helper.make_graph(
        nodes,
        "coreflow_equilibrium_mlp",
        [helper.make_tensor_value_info("features", TensorProto.FLOAT, [1, CFW2_IN_DIM])],
        [helper.make_tensor_value_info("logits", TensorProto.FLOAT, [1, CFW2_OUT_DIM])],
        initializer=initializers,
    )
    model = helper.make_model(graph, opset_imports=[helper.make_opsetid("", 13)])
    onnx.checker.check_model(model)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    onnx.save(model, str(out_path))


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description="Train a CFW2 Micro-MLP from coreflowd --diag JSON telemetry")
    parser.add_argument("--diag", nargs="*", default=[], help="One or more coreflowd --diag / --status JSON files")
    parser.add_argument("-o", "--output", required=True, help="Output .cfw2 binary path")
    parser.add_argument("--epochs", type=int, default=80, help="Training epochs (default: 80)")
    parser.add_argument("--lr", type=float, default=0.008, help="Initial learning rate (default: 0.008)")
    parser.add_argument("--activation", choices=("tanh", "silu"), default="tanh", help="Hidden activation")
    parser.add_argument("--seed", type=int, default=42, help="Deterministic RNG seed")
    parser.add_argument("--export-onnx", metavar="OUT_ONNX", help="Optionally export matching ONNX opset-13 graph")
    args = parser.parse_args(argv)

    if args.epochs < 1 or args.epochs > 5000:
        parser.error("--epochs must be within [1, 5000]")
    if not math.isfinite(args.lr) or args.lr <= 0.0 or args.lr > 1.0:
        parser.error("--lr must be finite and within (0, 1.0]")

    diag_samples: List[Sample] = []
    for diag_file in args.diag:
        p = Path(diag_file)
        if not p.is_file():
            parser.error(f"Diagnostic file not found: {diag_file}")
        diag_samples.extend(extract_samples_from_diag(p))

    anchors = physics_boundary_samples()
    # Keep safety boundary anchors balanced against real-device telemetry captures
    anchor_repeats = max(1, len(diag_samples) // max(1, len(anchors)))
    dataset = list(diag_samples) + anchors * anchor_repeats

    use_silu = args.activation == "silu"
    mlp = MicroMlp(build_physics_baseline_weights(), use_silu=use_silu)
    before_mse, after_mse = mlp.train(dataset, epochs=args.epochs, lr=args.lr, seed=args.seed)

    flags = 1 if use_silu else 0
    blob = pack_cfw2(mlp.flatten(), flags=flags)
    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(blob)
    meta, _ = unpack_cfw2(blob)

    if args.export_onnx:
        export_onnx_graph(mlp, Path(args.export_onnx))

    print(
        f"Trained CFW2 -> {out_path} ({meta['bytes']} B, act={args.activation}, "
        f"diag_samples={len(diag_samples)}, total_samples={len(dataset)}, "
        f"mse={before_mse:.4f}->{after_mse:.4f}, crc32={meta['crc32']})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
