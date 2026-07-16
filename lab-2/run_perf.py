# /// script
# requires-python = ">=3.9"
# dependencies = ["matplotlib"]
# ///
"""End-to-end lab driver: benchmark, then plot, in one pass.

    uv run run_perf.py        (or: python run_perf.py, if matplotlib is present)

Writes benchmark_results.json (Table A: loop-order comparison at N=2048;
Table B: problem-size sweep for the fastest ordering) plus three PNGs
(gflops_vs_order, gflops_vs_logN, stride_cacheline).

Runs sequentially and pins each process to one core at minimum nice (-20),
so concurrent L3/DRAM contention and scheduling jitter don't corrupt the
cache-bandwidth signal. Needs privilege for the affinity/priority prefix.
"""

import subprocess
import itertools
import math
import re
import sys
import json
import shutil
from pathlib import Path

import matplotlib

matplotlib.use("Agg")  # headless: write PNGs, never open a window
import matplotlib.pyplot as plt

HERE = Path(__file__).parent
BIN = HERE / "build/main"
RESULTS = HERE / "benchmark_results.json"

N_PART_A = 2048
SIZES_PART_B = [32, 64, 128, 256, 512, 1024, 2048]
REPEATS = 10

# Stride experiment settings (see run_stride in main.cpp).
STRIDE_TOTAL_DOUBLES = 1 << 24  # 128 MB buffer, comfortably larger than L3
STRIDE_ROUNDS = 5
STRIDE_LIST = [1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 128, 256, 512, 1024, 2048]
STRIDE_REPEATS = 5  # best-of-N to damp noise
CACHE_LINE_BYTES = 64  # expected x86-64 line size, annotated on the plot

# Innermost-loop var -> per-array stride, for row-major C[i][j] += A[i][k]*B[k][j].
# stride1 = contiguous, strideN = jumps a row, invariant = loop-invariant.
STRIDE_BY_INNERMOST = {
    "i": {"A": "strideN", "B": "invariant", "C": "strideN"},
    "j": {"A": "invariant", "B": "stride1", "C": "stride1"},
    "k": {"A": "stride1", "B": "strideN", "C": "invariant"},
}


def access_pattern(order):
    """'A:.., B:.., C:..' access pattern for this ordering (order[-1] = innermost)."""
    strides = STRIDE_BY_INNERMOST[order[-1]]
    return f"A:{strides['A']}, B:{strides['B']}, C:{strides['C']}"


NICE = -20  # minimum nice value = highest scheduling priority
AFFINITY_CPU = 1  # pin every benchmark to this one core (avoid busy core 0)
HAS_NICE = shutil.which("nice") is not None
HAS_TASKSET = shutil.which("taskset") is not None


def launch_prefix():
    """Prefix pinning to one core (taskset) at minimum nice: no migration,
    warm per-core caches across repeats. nice falls back gracefully unprivileged."""
    prefix = ["sudo", "-E"] if HAS_NICE else []
    if HAS_TASKSET:
        prefix += ["taskset", "-c", str(AFFINITY_CPU)]
    if HAS_NICE:
        prefix += ["nice", "-n", str(NICE)]
    return prefix


def run_perf(*args):
    """Run `perf stat -- build/main <args>`; return (raw_output, elapsed_seconds).

    elapsed is parsed from the binary's own `RESULT <name> <N> <time_s> <gflops>`
    line, which times only the kernel (alloc/init excluded) - unlike perf's
    "seconds time elapsed", which would include setup and inflate small N.
    perf's counters (cache-misses etc.) are kept in raw_output for the report."""
    cmd = launch_prefix() + ["perf", "stat", "--", str(BIN)] + list(args)
    result = subprocess.run(cmd, capture_output=True, text=True)
    # perf reports on stderr; the binary's RESULT line is on stdout.
    output = result.stdout + result.stderr
    m = re.search(r"^RESULT\s+\S+\s+\d+\s+([\d.eE+-]+)", result.stdout, re.MULTILINE)
    elapsed = float(m.group(1)) if m else None
    return output, elapsed


def run_perf_best(*args, repeats=REPEATS):
    """Run `run_perf` `repeats` times; return (logs, best_elapsed) - best None
    if every run failed."""
    logs = []
    elapsed_list = []
    for i in range(repeats):
        output, elapsed = run_perf(*args)
        logs.append(f"[run {i + 1}/{repeats}]\n{output}")
        if elapsed is not None:
            elapsed_list.append(elapsed)
    best = min(elapsed_list) if elapsed_list else None
    return logs, best


def run_benchmarks():
    """Run Tables A and B, print progress + summaries, write RESULTS, and
    return the parsed results dict."""
    if not BIN.exists():
        sys.exit(f"Binary not found: {BIN} -- build it first (cmake --build build)")

    # --- Table A: every loop ordering at a fixed N ---
    print(f"=== Table A: Loop-order comparison (N={N_PART_A}, best of {REPEATS} runs) ===\n")
    orders = ["".join(p) for p in itertools.permutations("ijk")]
    table_a = {}

    for order in orders:
        print(f"--- {order} ---")
        logs, best = run_perf_best(order, str(N_PART_A))
        print("\n".join(logs))
        if best is None:
            print(f"[!] no successful runs for {order}")
            continue
        gflops = (2 * N_PART_A**3) / best / 1e9
        table_a[order] = {"access_pattern": access_pattern(order), "time_s": best, "gflops": gflops}
        print(f"best elapsed for {order}: {best:.6f}s ({gflops:.2f} GFLOP/s)\n")

    if not table_a:
        sys.exit("Table A failed: no timing data collected")

    ranked = sorted(table_a, key=lambda o: table_a[o]["time_s"])
    for rank, order in enumerate(ranked, start=1):
        table_a[order]["rank"] = rank
    fastest = ranked[0]
    print(f"Fastest ordering: {fastest} ({table_a[fastest]['time_s']:.6f}s best)\n")

    print("--- Table A summary ---")
    print(f"{'Order':<8}{'Access pattern':<28}{'Time(s)':<12}{'GFLOP/s':<12}{'Rank':<6}")
    for order in orders:
        if order not in table_a:
            continue
        r = table_a[order]
        print(f"{order:<8}{r['access_pattern']:<28}{r['time_s']:<12.6f}{r['gflops']:<12.2f}{r['rank']:<6}")
    print()

    # --- Table B: cache sweep with the fastest ordering ---
    print(f"=== Table B: Problem-size sweep ({fastest}, best of {REPEATS} runs) ===\n")
    table_b = {}
    for N in SIZES_PART_B:
        print(f"--- N={N} ---")
        logs, best = run_perf_best(fastest, str(N))
        print("\n".join(logs))
        if best is None:
            print(f"[!] no successful runs for N={N}")
            continue
        table_b[N] = best
        print(f"best elapsed for N={N}: {best:.6f}s\n")

    # Build the ordered summary + ratios.
    summary = []
    prev_time = None
    for N in SIZES_PART_B:
        if N not in table_b:
            continue
        best = table_b[N]
        gflops = (2 * N**3) / best / 1e9
        footprint_bytes = 3 * N**2 * 8
        ratio = best / prev_time if prev_time else None
        summary.append({"N": N, "footprint_bytes": footprint_bytes, "time_s": best, "gflops": gflops, "ratio": ratio})
        prev_time = best

    print("--- Table B summary ---")
    print(f"{'N':<8}{'Footprint':<14}{'Time(s)':<14}{'GFLOP/s':<12}{'T(N)/T(N/2)':<12}")
    for r in summary:
        footprint_mb = r["footprint_bytes"] / (1024**2)
        ratio_str = f"{r['ratio']:.2f}" if r["ratio"] else "--"
        print(f"{r['N']:<8}{footprint_mb:<10.2f} MB {r['time_s']:<14.6f}{r['gflops']:<12.2f}{ratio_str:<12}")

    data = {"table_a": table_a, "table_a_N": N_PART_A, "table_b": summary, "fastest": fastest}
    with open(RESULTS, "w") as f:
        json.dump(data, f, indent=2)
    print(f"\nResults saved to {RESULTS.name}")
    return data


def plot_gflops_vs_order(data):
    """Step 7-i: GFLOP/s per loop ordering (bar, fastest first)."""
    table_a = data.get("table_a", {})
    if not table_a:
        print("[!] no table_a in results, skipping gflops_vs_order")
        return
    orders = sorted(table_a, key=lambda o: table_a[o]["gflops"], reverse=True)
    gflops = [table_a[o]["gflops"] for o in orders]

    fig, ax = plt.subplots(figsize=(7, 4.5))
    bars = ax.bar(orders, gflops, color="#3b6ea5", edgecolor="black", linewidth=0.5)
    ax.set_xlabel("loop ordering (outer→inner)")
    ax.set_ylabel("GFLOP/s")
    ax.set_title(f"Matrix multiply throughput by loop ordering (N={data.get('table_a_N', 2048)})")
    ax.bar_label(bars, fmt="%.2f", padding=3, fontsize=9)
    ax.margins(y=0.15)
    ax.grid(axis="y", linestyle=":", alpha=0.5)
    fig.tight_layout()
    out = HERE / "gflops_vs_order.png"
    fig.savefig(out, dpi=140)
    plt.close(fig)
    print(f"wrote {out}")


def plot_gflops_vs_logN(data):
    """Step 7-ii: GFLOP/s vs. log2 N for the fastest ordering (Table B)."""
    table_b = data.get("table_b", [])
    if not table_b:
        print("[!] no table_b in results, skipping gflops_vs_logN")
        return
    rows = sorted(table_b, key=lambda r: r["N"])
    log2n = [math.log2(r["N"]) for r in rows]
    gflops = [r["gflops"] for r in rows]

    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.plot(log2n, gflops, marker="o", color="#a5423b", linewidth=1.8)
    ax.set_xlabel("log₂ N")
    ax.set_ylabel("GFLOP/s")
    ax.set_title(f"Throughput vs. problem size ({data.get('fastest', '?')} ordering)")
    ax.set_xticks(log2n)
    ax.set_xticklabels([f"{int(x)}\n(N={r['N']})" for x, r in zip(log2n, rows)], fontsize=8)
    for x, y in zip(log2n, gflops):
        ax.annotate(f"{y:.2f}", (x, y), textcoords="offset points", xytext=(0, 7), fontsize=8, ha="center")
    ax.grid(True, linestyle=":", alpha=0.5)
    ax.margins(y=0.15)
    fig.tight_layout()
    out = HERE / "gflops_vs_logN.png"
    fig.savefig(out, dpi=140)
    plt.close(fig)
    print(f"wrote {out}")


def measure_stride():
    """Run `build/main stride ...` STRIDE_REPEATS times, keeping the min
    ns/touch per stride. Returns (stride, bytes, ns) rows sorted by stride."""
    if not BIN.exists():
        sys.exit(f"binary not found: {BIN} -- build it first (cmake --build build)")
    args = launch_prefix() + [
        str(BIN), "stride", str(STRIDE_TOTAL_DOUBLES), str(STRIDE_ROUNDS)
    ] + [str(s) for s in STRIDE_LIST]
    best = {}  # stride -> (bytes, min ns)
    for i in range(STRIDE_REPEATS):
        out = subprocess.run(args, capture_output=True, text=True).stdout
        for m in re.finditer(r"^STRIDE (\d+) (\d+) ([\d.eE+-]+)$", out, re.MULTILINE):
            stride, nbytes, ns = int(m.group(1)), int(m.group(2)), float(m.group(3))
            if stride not in best or ns < best[stride][1]:
                best[stride] = (nbytes, ns)
        print(f"  stride run {i + 1}/{STRIDE_REPEATS} done")
    rows = [{"stride": s, "bytes": best[s][0], "ns_per_touch": best[s][1]} for s in sorted(best)]
    return rows


def plot_stride_cacheline(rows):
    """Step 8: ns/touch vs. stride bytes (log2 x), marking the 64-byte line."""
    xbytes = [r["bytes"] for r in rows]
    ns = [r["ns_per_touch"] for r in rows]

    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.plot(xbytes, ns, marker="o", color="#2e7d32", linewidth=1.8)
    ax.set_xscale("log", base=2)
    ax.set_xlabel("stride (bytes)")
    ax.set_ylabel("time per touch (ns)")
    ax.set_title("Cache-line probe: per-touch cost vs. access stride")
    ax.set_xticks(xbytes)
    ax.set_xticklabels(xbytes, fontsize=8, rotation=45)
    ax.axvline(CACHE_LINE_BYTES, color="gray", linestyle="--", linewidth=1)
    ax.annotate(
        f"{CACHE_LINE_BYTES} B line",
        (CACHE_LINE_BYTES, max(ns)),
        textcoords="offset points",
        xytext=(6, -6),
        fontsize=9,
        color="gray",
    )
    ax.grid(True, which="both", linestyle=":", alpha=0.5)
    fig.tight_layout()
    out = HERE / "stride_cacheline.png"
    fig.savefig(out, dpi=140)
    plt.close(fig)
    print(f"wrote {out}")


def main():
    # 1. Benchmarks -> RESULTS
    data = run_benchmarks()

    # 2. Timing plots from the freshly-collected tables.
    print("\n=== Plots ===")
    plot_gflops_vs_order(data)
    plot_gflops_vs_logN(data)

    # 3. Stride / cache-line experiment + plot.
    print(f"measuring stride experiment (best of {STRIDE_REPEATS})...")
    stride_rows = measure_stride()
    plot_stride_cacheline(stride_rows)

    # Persist the raw stride numbers alongside the timing tables.
    data["stride"] = {
        "total_doubles": STRIDE_TOTAL_DOUBLES,
        "rounds": STRIDE_ROUNDS,
        "repeats": STRIDE_REPEATS,
        "rows": stride_rows,
    }
    with open(RESULTS, "w") as f:
        json.dump(data, f, indent=2)
    print(f"updated {RESULTS.name} with stride data")


if __name__ == "__main__":
    main()
