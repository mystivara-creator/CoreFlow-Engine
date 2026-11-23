#!/usr/bin/env python3
"""
CFAE System Framework — ONNX to CFW2 Micro-Model Compiler & Inspector.

Compiles a PyTorch/scikit-learn exported `.onnx` MLP (10 -> 16 -> 12 -> 3)
into CoreFlow's compact, CRC32-verified binary `CFW2` format (~1.7 KB) for
zero-dependency native execution inside `coreflowd`.

Usage:
  python3 tools/onnx_to_cfw2.py --export-default /tmp/equilibrium.cfw2
  python3 tools/onnx_to_cfw2.py --from-onnx model.onnx -o equilibrium.cfw2
  python3 tools/onnx_to_cfw2.py --inspect equilibrium.cfw2
"""
from __future__ import annotations

import argparse
import math
import struct
import sys
import zlib
from pathlib import Path
from typing import List, Tuple

CFW2_MAGIC = b"CFW2"
CFW2_VERSION = 2
CFW2_IN_DIM = 10
CFW2_H1_DIM = 16
CFW2_H2_DIM = 12
CFW2_OUT_DIM = 3
CFW2_HEADER_FMT = "<4sHHHHHHII"
CFW2_HEADER_SIZE = struct.calcsize(CFW2_HEADER_FMT)
CFW2_MAX_BYTES = 16384
CFW2_MAX_ABS_WEIGHT = 50.0


def expected_float_count(in_dim: int, h1: int, h2: int, out_dim: int) -> int:
    return (h1 * in_dim) + h1 + (h2 * h1) + h2 + (out_dim * h2) + out_dim


def validate_weights(weights: List[float], in_dim: int, h1: int, h2: int, out_dim: int) -> None:
    expected = expected_float_count(in_dim, h1, h2, out_dim)
    if len(weights) != expected:
        raise ValueError(f"Weight count mismatch: got {len(weights)}, expected {expected}")
    for idx, val in enumerate(weights):
        if not math.isfinite(val) or abs(val) > CFW2_MAX_ABS_WEIGHT:
            raise ValueError(f"Invalid weight at index {idx}: {val}")


def pack_cfw2(
    weights: List[float],
    in_dim: int = CFW2_IN_DIM,
    h1: int = CFW2_H1_DIM,
    h2: int = CFW2_H2_DIM,
    out_dim: int = CFW2_OUT_DIM,
    flags: int = 0,
) -> bytes:
    if (flags & ~0x0001) != 0:
        raise ValueError(f"Unsupported CFW2 flags bitmask: 0x{flags:04x}")
    validate_weights(weights, in_dim, h1, h2, out_dim)
    payload = struct.pack(f"<{len(weights)}f", *weights)
    crc = zlib.crc32(payload) & 0xFFFFFFFF
    header = struct.pack(
        CFW2_HEADER_FMT,
        CFW2_MAGIC,
        CFW2_VERSION,
        flags,
        in_dim,
        h1,
        h2,
        out_dim,
        crc,
        0,
    )
    blob = header + payload
    if len(blob) > CFW2_MAX_BYTES:
        raise ValueError(f"CFW2 binary exceeds {CFW2_MAX_BYTES} byte safety cap ({len(blob)} B)")
    return blob


def unpack_cfw2(blob: bytes) -> Tuple[dict, List[float]]:
    if len(blob) < CFW2_HEADER_SIZE or len(blob) > CFW2_MAX_BYTES:
        raise ValueError(f"Invalid CFW2 file size: {len(blob)} bytes")
    magic, ver, flags, in_dim, h1, h2, out_dim, crc_expected, reserved = struct.unpack(
        CFW2_HEADER_FMT, blob[:CFW2_HEADER_SIZE]
    )
    if magic != CFW2_MAGIC:
        raise ValueError(f"Invalid magic: {magic!r}")
    if ver != CFW2_VERSION:
        raise ValueError(f"Unsupported CFW2 version: {ver}")
    if (flags & ~0x0001) != 0:
        raise ValueError(f"Unsupported CFW2 flags: 0x{flags:04x}")
    if reserved != 0:
        raise ValueError(f"Non-zero reserved field: {reserved}")

    payload = blob[CFW2_HEADER_SIZE:]
    crc_actual = zlib.crc32(payload) & 0xFFFFFFFF
    if crc_actual != crc_expected:
        raise ValueError(f"CRC32 mismatch: expected 0x{crc_expected:08x}, got 0x{crc_actual:08x}")

    count = expected_float_count(in_dim, h1, h2, out_dim)
    if len(payload) != count * 4:
        raise ValueError(f"Payload size mismatch: {len(payload)} != {count * 4}")
    weights = list(struct.unpack(f"<{count}f", payload))
    validate_weights(weights, in_dim, h1, h2, out_dim)
    meta = {
        "version": ver,
        "flags": flags,
        "activation": "silu" if (flags & 1) else "tanh",
        "dims": (in_dim, h1, h2, out_dim),
        "crc32": f"0x{crc_actual:08x}",
        "bytes": len(blob),
        "params": count,
    }
    return meta, weights


def build_physics_baseline_weights() -> List[float]:
    """
    Build deterministic physics-calibrated 10 -> 16 -> 12 -> 3 baseline weights.
    Inputs: [norm_temp_p, norm_vel, norm_cpu_psi, norm_mem_psi, norm_io_psi,
             gpu_busy, render_p, jank_ratio, norm_scale, norm_charge_mw]
    Outputs: [delta_pressure_10s, jank_risk_logit, recommended_scale_logit]
    """
    w1 = [0.0] * (CFW2_H1_DIM * CFW2_IN_DIM)
    b1 = [0.0] * CFW2_H1_DIM

    def set_h1(unit: int, coeffs: List[float], bias: float) -> None:
        assert len(coeffs) == CFW2_IN_DIM
        base = unit * CFW2_IN_DIM
        for j, c in enumerate(coeffs):
            w1[base + j] = c
        b1[unit] = bias

    # Specialized physics feature detectors
    set_h1(0, [1.30, 1.10, 0.35, 0.05, 0.10, 0.45, 0.20, 0.00, 0.30, 0.55], -0.35)  # thermal surge
    set_h1(1, [0.45, 0.30, 1.15, -0.15, -0.10, 0.50, 0.40, 0.15, 0.35, 0.10], -0.25)  # compute load
    set_h1(2, [0.15, 0.10, 0.10, 1.25, 1.15, 0.05, 0.25, 0.35, 0.10, 0.00], -0.40)  # mem/IO stall
    set_h1(3, [-1.10, -0.80, -0.60, -0.40, -0.40, -0.50, -0.30, -0.30, -0.15, -0.25], 0.30)  # cool idle
    set_h1(4, [0.85, 1.45, 0.50, 0.15, 0.20, 0.45, 0.15, 0.00, 0.45, 0.65], -0.30)  # accel damper
    set_h1(5, [0.05, 0.05, 0.35, 0.25, 0.25, 0.45, 0.95, 1.45, -0.20, 0.00], -0.45)  # jank/frame-drop
    set_h1(6, [1.55, 0.75, 0.25, 0.55, 0.50, 0.40, 0.15, 0.10, 0.40, 0.45], -0.55)  # saturation guard
    set_h1(7, [-0.70, -0.90, -0.25, -0.15, -0.15, -0.20, -0.10, -0.15, 0.10, -0.35], 0.20)  # cooling recovery
    set_h1(8, [0.10, 0.15, 0.10, 0.00, 0.00, 1.25, 0.85, 0.55, 0.20, 0.15], -0.35)  # GPU gaming load
    set_h1(9, [0.40, 0.60, 0.05, 0.00, 0.00, 0.10, 0.05, 0.00, 0.15, 1.35], -0.45)  # fast-charge heat
    set_h1(10, [-0.25, 0.10, 0.85, -0.35, -0.25, 0.15, 0.80, 0.45, -0.10, 0.00], -0.20)  # UI scroll burst
    set_h1(11, [0.00, 0.00, 0.20, 0.80, 0.90, 0.00, 0.55, 0.95, -0.15, 0.00], -0.40)  # IO-induced jank
    set_h1(12, [0.70, 0.25, 0.40, 0.20, 0.20, 0.80, 0.35, 0.20, 0.50, 0.25], -0.40)  # sustained game heat
    set_h1(13, [-0.55, -0.40, 0.30, -0.20, -0.20, 0.35, 0.65, 0.60, -0.25, -0.15], -0.15)  # safe rescue window
    set_h1(14, [0.95, 0.45, 0.15, 0.30, 0.20, 0.20, 0.05, 0.05, 0.30, 0.70], -0.45)  # thermal+charge combined
    set_h1(15, [-0.45, -0.55, -0.45, -0.25, -0.25, -0.35, -0.55, -0.60, 0.15, -0.10], 0.25)  # smooth equilibrium

    # Layer 2 (12 x 16): combine detectors into thermal, jank, and headroom latents
    w2 = [0.0] * (CFW2_H2_DIM * CFW2_H1_DIM)
    b2 = [0.0] * CFW2_H2_DIM
    for h in range(CFW2_H2_DIM):
        w2[h * CFW2_H1_DIM + (h % CFW2_H1_DIM)] = 0.75
        w2[h * CFW2_H1_DIM + ((h + 4) % CFW2_H1_DIM)] = 0.25

    # Layer 3 (3 x 12)
    w3 = [
        # Out 0: delta_pressure_10s residual
        0.75, 0.35, 0.20, -0.55, 0.85, 0.10, 0.80, -0.45, 0.40, 0.55, 0.05, 0.10,
        # Out 1: jank_risk logit
        0.10, 0.30, 0.55, -0.50, 0.05, 1.15, 0.20, -0.30, 0.45, 0.05, 0.55, 0.90,
        # Out 2: recommended_scale logit (negative = throttle, positive = headroom)
        -0.95, -0.20, -0.65, 0.75, -0.85, 0.25, -1.10, 0.65, -0.35, -0.60, 0.30, -0.25,
    ]
    b3 = [0.05, -0.65, 0.70]
    return w1 + b1 + w2 + b2 + w3 + b3


def extract_from_onnx(onnx_path: Path) -> Tuple[List[float], Tuple[int, int, int, int], int]:
    """Extract weights and activation flag from a standard ONNX 3-layer MLP (`Gemm` or `MatMul`+`Add`)."""
    try:
        import onnx  # type: ignore
        from onnx import numpy_helper  # type: ignore
    except ImportError as exc:
        raise RuntimeError(
            "Compiling `.onnx` requires `pip install onnx numpy`. "
            "Use `--export-default` for zero-dependency baseline generation."
        ) from exc

    model = onnx.load(str(onnx_path))
    onnx.checker.check_model(model)
    init_map = {t.name: numpy_helper.to_array(t) for t in model.graph.initializer}

    mats = []
    biases = []
    saw_tanh = False
    saw_sigmoid = False
    saw_mul = False
    for node in model.graph.node:
        if node.op_type == "Tanh":
            saw_tanh = True
        elif node.op_type == "Sigmoid":
            saw_sigmoid = True
        elif node.op_type == "Mul":
            saw_mul = True
        elif node.op_type == "Gemm" and len(node.input) >= 2:
            w = init_map.get(node.input[1])
            b = init_map.get(node.input[2]) if len(node.input) >= 3 else None
            if w is None:
                continue
            trans_a = 0
            trans_b = 0
            alpha = 1.0
            beta = 1.0
            for attr in node.attribute:
                if attr.name == "transA":
                    trans_a = int(attr.i)
                elif attr.name == "transB":
                    trans_b = int(attr.i)
                elif attr.name == "alpha":
                    alpha = float(attr.f)
                elif attr.name == "beta":
                    beta = float(attr.f)
            if trans_a != 0:
                raise ValueError("Unsupported Gemm transA != 0 in CFW2 compiler")
            w_arr = (w if trans_b == 1 else w.T).astype("float32") * alpha
            mats.append(w_arr)
            biases.append(
                (b.astype("float32").reshape(-1) * beta)
                if b is not None
                else [0.0] * int(w_arr.shape[0])
            )
        elif node.op_type == "MatMul" and len(node.input) >= 2:
            w = init_map.get(node.input[1])
            if w is not None:
                mats.append(w.T.astype("float32"))
                biases.append([0.0] * int(w.shape[1]))
        elif node.op_type == "Add" and biases:
            for inp in node.input:
                if inp in init_map:
                    arr = init_map[inp].astype("float32").reshape(-1)
                    if len(arr) == len(biases[-1]):
                        biases[-1] = arr

    if len(mats) != 3:
        raise ValueError(f"Expected 3 linear layers (10->16->12->3), found {len(mats)}")
    h1, in_dim = int(mats[0].shape[0]), int(mats[0].shape[1])
    h2, h1_check = int(mats[1].shape[0]), int(mats[1].shape[1])
    out_dim, h2_check = int(mats[2].shape[0]), int(mats[2].shape[1])
    if h1 != h1_check or h2 != h2_check:
        raise ValueError("Layer dimension chain mismatch in ONNX graph")

    flags = 1 if (saw_sigmoid and saw_mul and not saw_tanh) else 0

    flat: List[float] = []
    for mat, bias in zip(mats, biases):
        flat.extend(float(x) for x in mat.reshape(-1))
        flat.extend(float(x) for x in bias)
    return flat, (in_dim, h1, h2, out_dim), flags


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description="CFAE ONNX-to-CFW2 Micro-Model Compiler")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--export-default", metavar="OUT_CFW2", help="Export physics-calibrated baseline CFW2")
    group.add_argument("--from-onnx", metavar="MODEL_ONNX", help="Compile a 3-layer ONNX MLP into CFW2")
    group.add_argument("--inspect", metavar="FILE_CFW2", help="Inspect and verify a CFW2 binary")
    parser.add_argument("-o", "--output", metavar="OUT_CFW2", help="Output path for --from-onnx")
    args = parser.parse_args(argv)

    if args.export_default:
        out_path = Path(args.export_default)
        blob = pack_cfw2(build_physics_baseline_weights())
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_bytes(blob)
        meta, _ = unpack_cfw2(blob)
        print(f"Exported CFW2 baseline -> {out_path} ({meta['bytes']} B, dims={meta['dims']}, crc32={meta['crc32']})")
        return 0

    if args.from_onnx:
        if not args.output:
            parser.error("--output is required with --from-onnx")
        weights, (in_dim, h1, h2, out_dim), flags = extract_from_onnx(Path(args.from_onnx))
        blob = pack_cfw2(weights, in_dim=in_dim, h1=h1, h2=h2, out_dim=out_dim, flags=flags)
        out_path = Path(args.output)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_bytes(blob)
        meta, _ = unpack_cfw2(blob)
        print(
            f"Compiled ONNX -> {out_path} ({meta['bytes']} B, act={meta['activation']}, "
            f"dims={meta['dims']}, crc32={meta['crc32']})"
        )
        return 0

    if args.inspect:
        blob = Path(args.inspect).read_bytes()
        meta, weights = unpack_cfw2(blob)
        print(
            f"CFW2 VALID: version={meta['version']} act={meta['activation']} dims={meta['dims']} "
            f"params={meta['params']} size={meta['bytes']}B crc32={meta['crc32']} "
            f"range=[{min(weights):.3f}, {max(weights):.3f}]"
        )
        return 0

    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
