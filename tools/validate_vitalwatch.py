#!/usr/bin/env python3
"""Standard-library-only black-box validation. Does not change the application."""
import argparse
import collections
import csv
import datetime as dt
import hashlib
import io
import json
import math
import platform
import re
import statistics
import subprocess
import sys
import time
from pathlib import Path

BASE = dt.datetime(2026, 1, 1, tzinfo=dt.timezone.utc)
READ_HEADER = ["timestamp", "heart_rate_bpm", "spo2_percent", "temperature_c", "connected"]
EVENT_HEADER = ["timestamp", "type", "sensor", "message"]


def expected(count):
    readings, events = [], []
    for i in range(count):
        stamp = (BASE + dt.timedelta(seconds=i)).strftime("%Y-%m-%dT%H:%M:%SZ")
        hr, oxygen, temperature, connected = "75.0", "97.0", "37.0", "1"
        kind, sensor = "", ""
        phase = i % 8
        if phase == 1:
            hr, kind, sensor = "120.0", "DEMO_ALERT", "heart_rate"
        elif phase == 2:
            oxygen, kind, sensor = "85.0", "DEMO_ALERT", "oxygen"
        elif phase == 3:
            temperature, kind, sensor = "39.0", "DEMO_ALERT", "temperature"
        elif phase == 4:
            hr, kind, sensor = "-5.0", "INVALID", "heart_rate"
        elif phase == 5:
            oxygen, kind, sensor = "", "MISSING", "oxygen"
        elif phase == 6:
            hr, oxygen, temperature, connected = "", "", "", "0"
            kind, sensor = "DISCONNECTED", "unit"
        readings.append([stamp, hr, oxygen, temperature, connected])
        if kind:
            events.append([stamp, kind, sensor])
    return readings, events


def load_csv(path, header):
    with path.open(newline="", encoding="utf-8") as stream:
        data = list(csv.reader(stream))
    if not data or data[0] != header:
        raise ValueError(f"Unexpected CSV header: {path}")
    return data[1:]


def inspect(logs, count, stdout, altered=False):
    rows = load_csv(logs / "readings.csv", READ_HEADER)
    events = load_csv(logs / "events.csv", EVENT_HEADER)
    wanted, wanted_events = expected(count)
    reference_mismatch = rows != wanted
    if altered:
        if not reference_mismatch:
            raise ValueError("Replay mismatch negative control was not detected")
        wanted[0][1] = "76.0"  # Valid changed input; application should replay it faithfully.
    stamps = [row[0] for row in rows]
    expected_stamps = {row[0] for row in wanted}
    missing = len(expected_stamps - set(stamps))
    duplicates = sum(n - 1 for n in collections.Counter(stamps).values())
    out_of_order = sum(b < a for a, b in zip(stamps, stamps[1:]))
    summary = re.search(r"^Processed (\d+) readings\.", stdout, re.MULTILINE)
    dashboard = re.findall(r"^\d{4}-\d{2}-\d{2}T", stdout, re.MULTILINE)
    if (rows != wanted or [row[:3] for row in events] != wanted_events
            or any(len(row) != 4 or not row[3] for row in events)
            or not summary or int(summary[1]) != count or len(dashboard) != count
            or missing or duplicates or out_of_order):
        raise ValueError("Readings, events, summary, count or FIFO validation failed")
    return len(rows), missing, duplicates, out_of_order, reference_mismatch


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--data-dir", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--latency-probe", type=Path, help="Optional vitalwatch_firmware_tests executable")
    args = parser.parse_args()
    if args.repeats < 1 or args.timeout <= 0:
        parser.error("repeats and timeout must be positive")
    exe, data, output = args.exe.resolve(), args.data_dir.resolve(), args.output.resolve()
    if not exe.is_file() or not (data / "sample_replay.csv").is_file():
        parser.error("Executable or sample replay is missing")
    # Refuse reuse rather than letting stale files masquerade as new successful output.
    output.mkdir(parents=True, exist_ok=False)
    fixtures = output / "fixtures"
    fixtures.mkdir()
    malformed = fixtures / "malformed.csv"
    malformed.write_text("timestamp,heart_rate,oxygen,temperature,connected\n1767225600,75junk,97,37,1\n")
    config = fixtures / "invalid.cfg"
    config.write_text("hr_min=200\nhr_max=100\n")
    changed = fixtures / "changed.csv"
    changed.write_text((data / "sample_replay.csv").read_text().replace("1767225600,75,", "1767225600,76,", 1))
    cases = []
    for name, count in [("demo", 8), ("stress", 10000)]:
        for repeat in range(1, args.repeats + 1):
            cases.append((name, repeat, ["--deterministic", "--count", str(count)], 0, count))
    cases += [
        ("replay", 1, ["--replay", str(data / "sample_replay.csv")], 0, 8),
        ("malformed", 1, ["--replay", str(malformed)], 1, 0),
        ("invalid_count", 1, ["--count", "0"], 1, 0),
        ("invalid_config", 1, ["--config", str(config)], 1, 0),
        ("replay_mismatch_control", 1, ["--replay", str(changed)], 0, 8),
    ]
    results, hashes = [], {}
    for name, repeat, options, expected_exit, count in cases:
        directory = output / f"{name}-{repeat}"
        directory.mkdir()
        logs = directory / "logs"
        command = [str(exe), *options, "--interval-ms", "0", "--log-dir", str(logs)]
        row = dict(scenario=name, repeat=repeat, exit_code="", expected_exit=expected_exit,
                   passed=False, requested_samples=count, successful_samples=0,
                   missing="", duplicates="", out_of_order="", elapsed_s=0,
                   samples_per_second="", readings_sha256="", events_sha256="", notes="",
                   command_json=json.dumps(command))
        start = time.perf_counter_ns()
        try:
            process = subprocess.run(command, capture_output=True, text=True, timeout=args.timeout, check=False)
            elapsed = (time.perf_counter_ns() - start) / 1e9
            row.update(exit_code=process.returncode, elapsed_s=elapsed)
            (directory / "stdout.txt").write_text(process.stdout)
            (directory / "stderr.txt").write_text(process.stderr)
            if process.returncode != expected_exit:
                raise ValueError(f"Expected exit {expected_exit}, got {process.returncode}")
            if count:
                if process.stderr:
                    raise ValueError("Unexpected stderr on a successful scenario")
                values = inspect(logs, count, process.stdout, name == "replay_mismatch_control")
                row.update(successful_samples=values[0], missing=values[1], duplicates=values[2],
                           out_of_order=values[3], samples_per_second=count / elapsed)
                for file in ["readings", "events"]:
                    row[f"{file}_sha256"] = hashlib.sha256((logs / f"{file}.csv").read_bytes()).hexdigest()
                if name in ["demo", "stress"]:
                    pair = (row["readings_sha256"], row["events_sha256"])
                    if name in hashes and hashes[name] != pair:
                        raise ValueError("Repeated output hashes differ")
                    hashes[name] = pair
                if name == "replay_mismatch_control":
                    row["notes"] = "Expected-reference mismatch detected; changed valid replay reproduced correctly"
            elif not process.stderr.startswith("Error:"):
                raise ValueError("Missing expected error diagnostic")
            elif name == "malformed" and "Replay line 2:" not in process.stderr:
                raise ValueError("Malformed replay diagnostic lacks line 2")
            row["passed"] = True
        except subprocess.TimeoutExpired as exc:
            row.update(elapsed_s=(time.perf_counter_ns() - start) / 1e9,
                       exit_code="timeout", notes=str(exc))
        except (ValueError, OSError) as exc:
            row["notes"] = str(exc)
        results.append(row)
    with (output / "results.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(results[0]))
        writer.writeheader()
        writer.writerows(results)
    passed = sum(row["passed"] for row in results)
    stress = [row for row in results if row["scenario"] == "stress" and row["passed"]]
    report = ["# VitalWatch executable validation", "", f"Passed: **{passed}/{len(results)}** scenarios.", "",
              f"Python: {platform.python_version()}; platform: {platform.platform()}",
              f"Executable SHA-256: `{hashlib.sha256(exe.read_bytes()).hexdigest()}`", "",
              "| Scenario | Repeat | Exit | Verified samples | Duration (s) | Result |",
              "| --- | --- | --- | --- | --- | --- |"]
    for row in results:
        report.append(f"| {row['scenario']} | {row['repeat']} | {row['exit_code']} | {row['successful_samples']} | {row['elapsed_s']:.6f} | {'PASS' if row['passed'] else 'FAIL'} |")
    if stress:
        median = statistics.median(row["elapsed_s"] for row in stress)
        report += ["", f"10,000-sample run median: **{median:.6f} s** over {len(stress)} runs.",
                   f"Approximate whole-process rate from median duration: **{10000 / median:.1f} samples/s**."]
    report += ["", "Timing includes process startup/exit, application acquisition/validation, CSV flushing and captured console I/O; Python scheduling and pipe handling add overhead. Input fixture construction and CSV verification/hashing are outside the timed section. No warmup runs are excluded. No per-sample latency is inferred from synthetic timestamps. This is not a real-time claim or a speed improvement claim.",
               "", "Missing samples means missing records, not intentionally blank sensor fields. Replay mismatch is a validator negative control; valid changed replay is not an application error. Raw commands, stdout, stderr and CSV files are retained."]
    probe_success, probe_rows = 0, []
    if args.latency_probe:
        probe = args.latency_probe.resolve()
        report += ["", "## Separate instrumented pipeline probe", "",
                   "Capacity 64; 10,000 in-memory readings; no CSV logger or dashboard. Latency is steady-clock time from immediately before enqueue to consumer callback entry, including backpressure waiting. Two clock reads, timestamp storage, sequence checks and latency collection add overhead. It is not end-to-end CLI latency.", "",
                   "| Trial | Samples | Duration (s) | Median latency (us) | p95 (us) | Result |",
                   "| --- | --- | --- | --- | --- | --- |"]
        for trial in range(1, args.repeats + 1):
            entry = dict(trial=trial, passed=False, exit_code="", samples=0, duration_s="",
                         samples_per_second="", fifo_ok="", latency_median_us="",
                         latency_p95_us="", latency_max_us="", capacity="", notes="")
            try:
                result = subprocess.run([str(probe), "measure"], capture_output=True, text=True,
                                        timeout=args.timeout, check=False)
                entry["exit_code"] = result.returncode
                if result.returncode != 0 or result.stderr:
                    raise ValueError(f"Probe failed: {result.stderr}")
                metrics = list(csv.DictReader(io.StringIO(result.stdout)))
                if len(metrics) != 1 or metrics[0]["samples"] != "10000" or metrics[0]["fifo_ok"] != "1":
                    raise ValueError("Probe count/order validation failed")
                if any(not math.isfinite(float(value)) or float(value) < 0 for value in metrics[0].values()):
                    raise ValueError("Invalid probe measurement")
                entry.update(metrics[0], passed=True)
                probe_success += 1
            except (OSError, ValueError, KeyError, subprocess.TimeoutExpired) as exc:
                entry["notes"] = str(exc)
            probe_rows.append(entry)
            report.append(f"| {trial} | {entry['samples']} | {entry['duration_s']} | {entry['latency_median_us']} | {entry['latency_p95_us']} | {'PASS' if entry['passed'] else 'FAIL'} |")
        with (output / "latency-results.csv").open("w", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=list(probe_rows[0]))
            writer.writeheader()
            writer.writerows(probe_rows)
        report += ["", f"Probe passed: **{probe_success}/{args.repeats}**; command: `{probe} measure`."]
    (output / "SUMMARY.md").write_text("\n".join(report) + "\n")
    print(f"{passed}/{len(results)} validation scenarios passed; output: {output}")
    return 0 if passed == len(results) and (not args.latency_probe or probe_success == args.repeats) else 1


if __name__ == "__main__":
    sys.exit(main())
