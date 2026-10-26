# Measured performance — existing CLI preserved

Environment: Linux-6.18.55-x86_64-with-glibc2.39, Python 3.12.14. GCC 13.3.0 / Release -O3.

| Batch | Runs | Samples/run | Median duration (s) | Min–max (s) | Rate from median (samples/s) |
| --- | --- | --- | --- | --- | --- |
| baseline-validation | 5 | 10000 | 0.075278 | 0.072534–0.095795 | 132841.3 |
| enhanced-validation | 5 | 10000 | 0.094660 | 0.080171–0.138137 | 105640.8 |
| repeat-validation | 5 | 10000 | 0.077231 | 0.064333–0.173138 | 129481.5 |

Baseline was measured before source changes, using the same validator core and workload. These are separate sequential batches on a shared host; load and cache conditions were not controlled. No speedup, regression percentage or real-time claim is inferred. Timing covers launch, executable processing, CSV flushes, captured console/pipe I/O, exit and Python process-wait overhead. CSV validation/hashing is outside the timer. No warmup was discarded.

## Instrumented pipeline latency

See repeat-validation/latency-results.csv for all five measured runs. Each processed 10,000 samples in FIFO order, capacity 64. Timing ends before sorting latency values. The probe has no logger/dashboard and is not comparable to whole-CLI throughput. Latency is measured from the producer callback immediately before enqueue until consumer callback entry, including backpressure; it excludes processing completion and file output. Two steady-clock calls per sample, timestamp storage, vector appends and order checks add overhead. Synthetic patient timestamps are never used as wall-time measurements.

| Trial | Samples | Duration (s) | Rate (samples/s) | Median latency (us) | p95 latency (us) | Max latency (us) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 10000 | 0.016077099 | 622002.7631 | 20.6395 | 112.197 | 876.953 |
| 2 | 10000 | 0.008457853 | 1182333.152 | 18.2025 | 90.452 | 347.783 |
| 3 | 10000 | 0.028863572 | 346457.4655 | 20.866 | 94.895 | 2764.073 |
| 4 | 10000 | 0.008416195 | 1188185.397 | 18.428 | 78.197 | 279.965 |
| 5 | 10000 | 0.0266571 | 375134.5795 | 20.4715 | 91.049 | 2441.912 |

These are observed instrumented runs, not clinical response latency or production capacity. Percentiles are nearest-rank p95 per 10,000-value trial; medians average sorted elements 5,000 and 5,001. No peak memory or coverage measurement was performed.
