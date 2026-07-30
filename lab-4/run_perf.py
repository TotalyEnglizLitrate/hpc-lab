# /// script
# requires-python = ">=3.9"
# dependencies = ["matplotlib"]
# ///
"""End-to-end lab driver for Experiment 3: build each flag variant, run the
benchmarks under perf, and plot.

    uv run run_perf.py        (or: python run_perf.py, if matplotlib is present)

Writes benchmark_results.json (Tables A-D) plus vecreport_<target>.txt
(compiler vectorisation reports, captured at build time) and
bandwidth_vs_blocksize.png.

Runs sequentially and pins each process to one core at minimum nice, so
concurrent contention and scheduling jitter don't corrupt the bandwidth
signal. Needs privilege for the affinity/priority prefix.
"""

import subprocess
import math
import re
import sys
import json
import shutil
import os
from pathlib import Path

import matplotlib

matplotlib.use("Agg")  # headless: write PNGs, never open a window
import matplotlib.pyplot as plt


os.environ["NIX_ENFORCE_NO_NATIVE"] = "0"

HERE = Path(__file__).parent
BUILD = HERE / "build"
RESULTS = HERE / "benchmark_results.json"

N = 1 << 24
B_TABLE_A = 4096      # fixed block size while sweeping optimisation flags
B_TABLE_C = [64, 256, 1024, 4096, 16384, 65536, N]  # last = single block
REPEATS = 3

# name -> (cmake target, human label for Table A)
VARIANTS: dict[str, str] = {
    "-O0": "main_O0",
    "-O1": "main_O1",
    "-O2": "main_O2",
    "-O3": "main_O3",
    "-O3 -march=native": "main_O3_native",
    "-O3 -march=native -ffast-math": "main_O3_native_fast",
}
SCALAR_TARGET = "main_scalar"          # -O2 -fno-tree-vectorize
VECTOR_TARGET = "main_O3_native"       # -O3 -march=native
BESTFLAGS_TARGET = "main_bestflags"    # -O3 -march=native

NICE = -20
AFFINITY_CPU = 2
HAS_NICE = shutil.which("nice") is not None
HAS_TASKSET = shutil.which("taskset") is not None


def launch_prefix():
    prefix = ["sudo", "-n", "-E"]
    if HAS_TASKSET:
        prefix += ["taskset", "-c", str(AFFINITY_CPU)]
    if HAS_NICE:
        prefix += ["nice", "-n", str(NICE)]
    return prefix
 
 
def build_target(target):
    """Rebuild one target from scratch, saving its compiler vectorisation
    report (stderr, since -fopt-info-vec has no other output route here)."""
    subprocess.run(["cmake", "-S", ".", "-B", "build"], cwd=HERE, capture_output=True)
    proc = subprocess.run(
        ["cmake", "--build", "build", "--target", target, "--clean-first"],
        cwd=HERE, capture_output=True, text=True,
    )
    report = HERE / f"vecreport_{target}.txt"
    report.write_text(proc.stdout + proc.stderr)
    vectorised = "loop vectorized" in (proc.stdout + proc.stderr)
    return vectorised, report
 
 
def run_bin(target, *args, events=None):
    """Run `perf stat [-e events] -- build/<target> <args>`. Returns
    (RESULT-line fields dict, perf counter dict)."""
    binpath = BUILD / target
    cmd = launch_prefix() + ["perf", "stat"]
    if events:
        cmd += ["-e", ",".join(events)]
    cmd += ["--", str(binpath)] + list(args)
    proc = subprocess.run(cmd, capture_output=True, text=True)
 
    fields = {}
    m = re.search(r"^RESULT\s+(.*)$", proc.stdout, re.MULTILINE)
    if m:
        for kv in m.group(1).split():
            if "=" in kv:
                k, v = kv.split("=", 1)
                fields[k] = v
 
    counters = {}
    for line in proc.stderr.splitlines():
        m2 = re.search(r"^\s*([\d,]+)\s+(instructions|cycles|cache-misses)\b", line)
        if m2:
            counters[m2.group(2)] = int(m2.group(1).replace(",", ""))
    return fields, counters
 
 
def best_of(target, *args, events=None, repeats=REPEATS):
    """Run `repeats` times, keep the fastest by time_s. Runs that produced
    no RESULT line (fields empty/missing time_s) are skipped entirely."""
    best_fields, best_counters, best_time = None, None, None
    failures = 0
    for _ in range(repeats):
        fields, counters = run_bin(target, *args, events=events)
        if "time_s" not in fields:
            failures += 1
            continue
        t = float(fields["time_s"])
        if best_time is None or t < best_time:
            best_time, best_fields, best_counters = t, fields, counters
    if best_fields is None:
        print(f"  [!] all {repeats} runs of {target} {' '.join(args)} failed to "
              f"produce a RESULT line - check that perf can launch the binary "
              f"(run the printed command manually to see the real error)")
        return {}, {}
    if failures:
        print(f"  [!] {failures}/{repeats} runs of {target} failed silently")
    return best_fields, best_counters
 
 
def table_a():
    print(f"=== Table A: optimisation levels (N={N}, B={B_TABLE_A}) ===")
    rows = {}
    baseline = None
    for flags, target in VARIANTS.items():
        print(f"--- {flags} ({target}) ---")
        vectorised, _ = build_target(target)
        fields, _ = best_of(target, "blocked", str(N), str(B_TABLE_A))
        if not fields:
            print(f"[!] no result for {target}")
            continue
        t = float(fields["time_s"])
        row = {
            "vectorised": vectorised,
            "time_s": t,
            "bandwidth_GBps": float(fields["bandwidth_GBps"]),
            "verified": fields.get("verified") == "Y",
        }
        if baseline is None:
            baseline = t
        row["speedup_vs_O0"] = baseline / t
        rows[flags] = row
        print(f"  vectorised={vectorised} time={t:.4f}s "
              f"bw={row['bandwidth_GBps']:.2f} GB/s speedup={row['speedup_vs_O0']:.2f}x")
    return rows
 
 
def table_b():
    print("=== Table B: perf counters, scalar vs. vector build ===")
    events = ["instructions", "cycles"]
    build_target(SCALAR_TARGET) 
    scalar_fields, scalar_counters = best_of(SCALAR_TARGET, "blocked", str(N), str(B_TABLE_A), events=events)

    build_target(VECTOR_TARGET)
    vector_fields, vector_counters = best_of(VECTOR_TARGET, "blocked", str(N), str(B_TABLE_A), events=events)
 
    def row(fields, counters):
        if "time_s" not in fields:
            return {"instructions": None, "cycles": None, "ipc": None, "time_s": None}
        insn, cyc = counters.get("instructions"), counters.get("cycles")
        ipc = insn / cyc if insn and cyc else None
        return {"instructions": insn, "cycles": cyc, "ipc": ipc, "time_s": float(fields["time_s"])}
 
    scalar_row = row(scalar_fields, scalar_counters)
    vector_row = row(vector_fields, vector_counters)
    ratio = (scalar_row["instructions"] / vector_row["instructions"]
              if scalar_row["instructions"] and vector_row["instructions"] else None)
    print(f"  scalar: {scalar_row}")
    print(f"  vector: {vector_row}")
    print(f"  instruction ratio (scalar/vector) = {ratio}")
    return {"scalar": scalar_row, "vector": vector_row, "instruction_ratio": ratio}
 
 
def table_c():
    print("=== Table C: block-size sweep (fixed best flags) ===")
    build_target(BESTFLAGS_TARGET)
    rows = []
    for b in B_TABLE_C:
        print(f"--- B={b} ---")
        fields, _ = best_of(BESTFLAGS_TARGET, "blocked", str(N), str(b))
        if not fields:
            print(f"[!] no result for B={b}")
            continue
        row = {
            "B": b,
            "time_s": float(fields["time_s"]),
            "bandwidth_GBps": float(fields["bandwidth_GBps"]),
            "n_blocks": N // b,
        }
        rows.append(row)
        print(f"  time={row['time_s']:.4f}s bw={row['bandwidth_GBps']:.2f} GB/s n_blocks={row['n_blocks']}")
    return rows
 
 
def table_d():
    print("=== Table D: AoS vs. SoA x-component sum ===")
    build_target(BESTFLAGS_TARGET)
    events = ["cache-misses"]
    aos_fields, aos_counters = best_of(BESTFLAGS_TARGET, "aos", str(N), events=events)
    soa_fields, soa_counters = best_of(BESTFLAGS_TARGET, "soa", str(N), events=events)
 
    if "time_s" not in aos_fields or "time_s" not in soa_fields:
        print("  [!] aos/soa run(s) failed - skipping Table D")
        return {"aos": {}, "soa": {}, "speedup_soa_over_aos": None}
 
    aos = {"time_s": float(aos_fields["time_s"]), "cache_misses": aos_counters.get("cache-misses")}
    soa = {"time_s": float(soa_fields["time_s"]), "cache_misses": soa_counters.get("cache-misses")}
    speedup = aos["time_s"] / soa["time_s"]
    print(f"  aos: {aos}")
    print(f"  soa: {soa}")
    print(f"  speedup (SoA/AoS) = {speedup:.2f}x")
    return {"aos": aos, "soa": soa, "speedup_soa_over_aos": speedup}
 
 
def plot_bandwidth_vs_blocksize(rows):
    if not rows:
        print("[!] no table_c rows, skipping plot")
        return
    log2b = [math.log2(r["B"]) for r in rows]
    bw = [r["bandwidth_GBps"] for r in rows]
 
    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.plot(log2b, bw, marker="o", color="#3b6ea5", linewidth=1.8)
    ax.set_xlabel("log\u2082 B")
    ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_title("Blocked-sum bandwidth vs. block size")
    ax.set_xticks(log2b)
    ax.set_xticklabels([f"{int(x)}\n(B={r['B']})" for x, r in zip(log2b, rows)], fontsize=8)
    ax.grid(True, linestyle=":", alpha=0.5)
    fig.tight_layout()
    out = HERE / "bandwidth_vs_blocksize.png"
    fig.savefig(out, dpi=140)
    plt.close(fig)
    print(f"wrote {out}")
 
 
def main():
    if not shutil.which("perf"):
        sys.exit("perf not found on PATH - install linux-tools for your kernel")
 
    data = {
        "N": N,
        "table_a": table_a(),
        "table_b": table_b(),
        "table_c": table_c(),
        "table_d": table_d(),
    }
    with open(RESULTS, "w") as f:
        json.dump(data, f, indent=2)
    print(f"\nResults saved to {RESULTS.name}")
 
    plot_bandwidth_vs_blocksize(data["table_c"])
 
 
if __name__ == "__main__":
    main()
