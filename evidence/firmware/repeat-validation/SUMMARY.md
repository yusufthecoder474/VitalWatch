# VitalWatch executable validation

Passed: **15/15** scenarios.

Python: 3.12.14; platform: Linux-6.18.55-x86_64-with-glibc2.39
Executable SHA-256: `b184ce42057a6dff47ebcc218bb8d8292f8f1931b2f7994e004275a7cdd8e860`

| Scenario | Repeat | Exit | Verified samples | Duration (s) | Result |
| --- | --- | --- | --- | --- | --- |
| demo | 1 | 0 | 8 | 0.008712 | PASS |
| demo | 2 | 0 | 8 | 0.006797 | PASS |
| demo | 3 | 0 | 8 | 0.003530 | PASS |
| demo | 4 | 0 | 8 | 0.003763 | PASS |
| demo | 5 | 0 | 8 | 0.003565 | PASS |
| stress | 1 | 0 | 10000 | 0.064333 | PASS |
| stress | 2 | 0 | 10000 | 0.173138 | PASS |
| stress | 3 | 0 | 10000 | 0.079993 | PASS |
| stress | 4 | 0 | 10000 | 0.065892 | PASS |
| stress | 5 | 0 | 10000 | 0.077231 | PASS |
| replay | 1 | 0 | 8 | 0.004083 | PASS |
| malformed | 1 | 1 | 0 | 0.002592 | PASS |
| invalid_count | 1 | 1 | 0 | 0.002604 | PASS |
| invalid_config | 1 | 1 | 0 | 0.002339 | PASS |
| replay_mismatch_control | 1 | 0 | 8 | 0.003367 | PASS |

10,000-sample run median: **0.077231 s** over 5 runs.
Approximate whole-process rate from median duration: **129481.5 samples/s**.

Timing includes process startup/exit, application acquisition/validation, CSV flushing and captured console I/O; Python scheduling and pipe handling add overhead. Input fixture construction and CSV verification/hashing are outside the timed section. No warmup runs are excluded. No per-sample latency is inferred from synthetic timestamps. This is not a real-time claim or a speed improvement claim.

Missing samples means missing records, not intentionally blank sensor fields. Replay mismatch is a validator negative control; valid changed replay is not an application error. Raw commands, stdout, stderr and CSV files are retained.

## Separate instrumented pipeline probe

Capacity 64; 10,000 in-memory readings; no CSV logger or dashboard. Latency is steady-clock time from immediately before enqueue to consumer callback entry, including backpressure waiting. Two clock reads, timestamp storage, sequence checks and latency collection add overhead. It is not end-to-end CLI latency.

| Trial | Samples | Duration (s) | Median latency (us) | p95 (us) | Result |
| --- | --- | --- | --- | --- | --- |
| 1 | 10000 | 0.016077099 | 20.6395 | 112.197 | PASS |
| 2 | 10000 | 0.008457853 | 18.2025 | 90.452 | PASS |
| 3 | 10000 | 0.028863572 | 20.866 | 94.895 | PASS |
| 4 | 10000 | 0.008416195 | 18.428 | 78.197 | PASS |
| 5 | 10000 | 0.0266571 | 20.4715 | 91.049 | PASS |

Probe passed: **5/5**; command: `/workspace/scratch/bb8633e62f1c/firmware-work/VitalWatch/build-firmware-release/vitalwatch_firmware_tests measure`.
