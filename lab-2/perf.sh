#!/usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]}")"
mkdir -p perf_data

for v in ijk ikj jik jki kij kji; do
    perf record -g -o "perf_data/${v}.data" -- build/main "$v" 2500 3
done
