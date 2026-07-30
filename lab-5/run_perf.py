# /// script
# requires-python = ">=3.9"
# ///
"""End-to-end lab driver for Experiment 4: build once, then run Parts A-D
and dump the results.

    python run_perf.py     (or: uv run run_perf.py)

Writes benchmark_results.json (Tables B-D). Table A (team-size control) is
printed to stdout only, since it's per-thread text output, not a RESULT
line - copy the printed team sizes into Table A by hand.
"""

import json
import os
import re
import shlex
import subprocess
from pathlib import Path

os.environ["NIX_ENFORCE_NO_NATIVE"] = "0"

HERE = Path(__file__).parent
BUILD = HERE / "build"
BIN = BUILD / "main"
RESULTS = HERE / "benchmark_results.json"

N = 10_000_000
T_FIXED = 24                     # thread count used for Table B/C
REPEATS = 3
IMPLS = ["unprotected", "critical-naive", "atomic-naive", "critical-smart", "atomic-smart"]
SCALING_THREADS = sorted(set([1, 2, 4, 8, os.cpu_count() or 1]))

HAS_TASKSET = False
AFFINITY_CPU = 2
HAS_NICE = True
NICE = -20

def launch_prefix():
    prefix = ["sudo", "-n", "-E"]
    if HAS_TASKSET:
        prefix += ["taskset", "-c", str(AFFINITY_CPU)]
    if HAS_NICE:
        prefix += ["nice", "-n", str(NICE)]
    return prefix

def build():
    subprocess.run(["cmake", "-S", ".", "-B", "build"], cwd=HERE, check=True, capture_output=True)
    subprocess.run(["cmake", "--build", "build"], cwd=HERE, check=True, capture_output=True)


def run(*args):
    proc = subprocess.run(shlex.split(" ".join((launch_prefix() + [str(BIN), *args]))), capture_output=True, text=True)
    fields = {}
    m = re.search(r"^RESULT\s+(.*)$", proc.stdout, re.MULTILINE)
    if m:
        for kv in m.group(1).split():
            if "=" in kv:
                k, v = kv.split("=", 1)
                fields[k] = v
    return fields, proc.stdout


def best_of(*args, repeats=REPEATS):
    best = None
    for _ in range(repeats):
        fields, _ = run(*args)
        if "time_s" not in fields:
            continue
        t = float(fields["time_s"])
        if best is None or t < float(best["time_s"]):
            best = fields
    return best


def table_a():
    print("=== Table A: team-size control (see stdout, fill Table A by hand) ===")
    print("--- (i) default ---")
    subprocess.run([str(BIN), "teamsize"])
    print("--- (ii) OMP_NUM_THREADS=6 ---")
    subprocess.run([str(BIN), "teamsize"], env={**os.environ, "OMP_NUM_THREADS": "6"})
    print("--- (iii) num_threads(5) clause ---")
    subprocess.run([str(BIN), "teamsize", "--clause", "5"])
    print("--- (iv) omp_set_num_threads(3) ---")
    subprocess.run([str(BIN), "teamsize", "--runtime", "3"])
    print("--- (v) OMP_NUM_THREADS=2 vs num_threads(6) clause (conflict) ---")
    subprocess.run([str(BIN), "teamsize", "--clause", "6"], env={**os.environ, "OMP_NUM_THREADS": "2"})


def table_b():
    print(f"=== Table B: unprotected race, 5 runs (N={N}, T={T_FIXED}) ===")
    rows = []
    for i in range(1, 6):
        fields, _ = run("sum", "unprotected", str(N), str(T_FIXED))
        row = {
            "run": i,
            "reference": float(fields["reference"]),
            "measured": float(fields["sum"]),
            "abs_diff": abs(float(fields["sum"]) - float(fields["reference"])),
            "correct": fields["verified"] == "Y",
        }
        rows.append(row)
        print(f"  run {i}: measured={row['measured']:.6f} diff={row['abs_diff']:.3e} correct={row['correct']}")
    return rows


def table_c():
    print(f"=== Table C: six implementations (N={N}, T={T_FIXED}) ===")
    serial = best_of("serial", str(N))
    serial_time = float(serial["time_s"])
    rows = {"serial": {"time_s": serial_time, "verified": True, "slowdown": 1.0}}
    print(f"  serial: time={serial_time:.6f}s")

    unprotected = best_of("sum", "unprotected", str(N), str(T_FIXED))
    rows["unprotected"] = {
        "time_s": float(unprotected["time_s"]),
        "verified": unprotected["verified"] == "Y",
        "slowdown": float(unprotected["time_s"]) / serial_time,
    }
    print(f"  unprotected: time={rows['unprotected']['time_s']:.6f}s verified={rows['unprotected']['verified']}")

    for impl in IMPLS[1:]:  # skip unprotected, already done above
        fields = best_of("sum", impl, str(N), str(T_FIXED))
        rows[impl] = {
            "time_s": float(fields["time_s"]),
            "verified": fields["verified"] == "Y",
            "slowdown": float(fields["time_s"]) / serial_time,
        }
        print(f"  {impl}: time={rows[impl]['time_s']:.6f}s verified={rows[impl]['verified']} "
              f"slowdown={rows[impl]['slowdown']:.2f}x")
    return rows


def table_d(fastest_impl="atomic-smart"):
    print(f"=== Table D: scaling of {fastest_impl} (N={N}) ===")
    serial = best_of("serial", str(N))
    serial_time = float(serial["time_s"])
    rows = []
    for t in SCALING_THREADS:
        fields = best_of("sum", fastest_impl, str(N), str(t))
        time_s = float(fields["time_s"])
        speedup = serial_time / time_s
        efficiency = speedup / t
        rows.append({"threads": t, "time_s": time_s, "speedup": speedup, "efficiency": efficiency})
        print(f"  T={t}: time={time_s:.6f}s speedup={speedup:.2f}x efficiency={efficiency:.2f}")
    return rows


def main():
    print("Building...")
    build()

    table_a()
    data = {
        "N": N,
        "table_b": table_b(),
        "table_c": table_c(),
        "table_d": table_d(),
    }
    with open(RESULTS, "w") as f:
        json.dump(data, f, indent=2)
    print(f"\nResults saved to {RESULTS.name}")


if __name__ == "__main__":
    main()
