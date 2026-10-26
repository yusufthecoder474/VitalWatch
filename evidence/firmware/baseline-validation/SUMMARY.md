# VitalWatch executable validation

Passed: **15/15** scenarios.

Python: 3.12.14; platform: Linux-6.18.55-x86_64-with-glibc2.39
Executable SHA-256: `4a60ebfa77e11df9625ec8d25c51c24345b816e7a13a99033789db6acdf814d1`

| Scenario | Repeat | Exit | Verified samples | Duration (s) | Result |
| --- | --- | --- | --- | --- | --- |
| demo | 1 | 0 | 8 | 0.003893 | PASS |
| demo | 2 | 0 | 8 | 0.004101 | PASS |
| demo | 3 | 0 | 8 | 0.003478 | PASS |
| demo | 4 | 0 | 8 | 0.004892 | PASS |
| demo | 5 | 0 | 8 | 0.005355 | PASS |
| stress | 1 | 0 | 10000 | 0.077424 | PASS |
| stress | 2 | 0 | 10000 | 0.095795 | PASS |
| stress | 3 | 0 | 10000 | 0.073621 | PASS |
| stress | 4 | 0 | 10000 | 0.072534 | PASS |
| stress | 5 | 0 | 10000 | 0.075278 | PASS |
| replay | 1 | 0 | 8 | 0.005024 | PASS |
| malformed | 1 | 1 | 0 | 0.008665 | PASS |
| invalid_count | 1 | 1 | 0 | 0.002518 | PASS |
| invalid_config | 1 | 1 | 0 | 0.002767 | PASS |
| replay_mismatch_control | 1 | 0 | 8 | 0.003852 | PASS |

10,000-sample run median: **0.075278 s** over 5 runs.
Approximate whole-process rate from median duration: **132841.3 samples/s**.

Timing includes process startup/exit, application acquisition/validation, CSV flushing and captured console I/O; Python scheduling and pipe handling add overhead. Input fixture construction and CSV verification/hashing are outside the timed section. No warmup runs are excluded. No per-sample latency is inferred from synthetic timestamps. This is not a real-time claim or a speed improvement claim.

Missing samples means missing records, not intentionally blank sensor fields. Replay mismatch is a validator negative control; valid changed replay is not an application error. Raw commands, stdout, stderr and CSV files are retained.
