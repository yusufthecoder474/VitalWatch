<div align="center">

# VitalWatch

### A C++17 Multithreaded Monitoring & Fault Detection Simulator

**Producer–consumer concurrency · Synthetic sensor data · Fault classification · Deterministic replay · Automated testing**

![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue?style=flat-square)
![Build](https://img.shields.io/badge/Build-CMake-informational?style=flat-square)
![Testing](https://img.shields.io/badge/Testing-CTest-informational?style=flat-square)
![Verification](https://img.shields.io/badge/Recorded%20Linux%20run-39%2F39%20tests%20passed-brightgreen?style=flat-square)
![Status](https://img.shields.io/badge/Status-Educational%20simulator-orange?style=flat-square)

[Quick Start](#quick-start) · [How It Works](#how-it-works) · [Evidence](#verified-results) · [Architecture](docs/ARCHITECTURE.md) · [Interview Guide](docs/INTERVIEW.md)

</div>

## Current firmware reliability enhancement — 2026-10-26

The recovered source passed **33/33 baseline tests before modification**; the enhanced Linux/Bash suite passes **39/39**, including all existing tests. Changes are limited to sanitizer options, six deterministic fault/backpressure cases, a standard-library Python validator and measured performance/latency tooling. The application still has one producer, one consumer and the same CLI behavior. Optional bounded capacity is a library/test feature; the CLI keeps its original unbounded default.

Read [the current enhancement report](docs/FIRMWARE_RELIABILITY.md), [measured performance](evidence/firmware/PERFORMANCE.md) and [the combined MiPhi verification report](MiPhi-Project-Verification-Report.md). The current results supersede the historical Thales section below. The second MiPhi project's source/name was not supplied or recoverable in this task; its report section is explicitly pending.

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DVITALWATCH_WARNINGS_AS_ERRORS=ON
cmake --build build-release --parallel 2
(cd build-release && ctest --output-on-failure)
python3 tools/validate_vitalwatch.py --exe build-release/vitalwatch --data-dir data --output evidence/my-validation --repeats 5 --latency-probe build-release/vitalwatch_firmware_tests
```

GCC/Clang can enable VITALWATCH_ENABLE_ASAN and VITALWATCH_ENABLE_UBSAN together; MSVC supports the ASan option only. Native Windows/Clang and hosted CI execution remain pending. Python is needed only for the added validator. The combined ASan/UBSan run with leak detection disabled passed 39/39; the default leak-enabled runtime failed in this environment and is not counted as a pass. No performance improvement is claimed.

> [!IMPORTANT]
> **Educational software only.** VitalWatch processes **synthetic** readings, not real patient information. Its thresholds are non-clinical demonstration rules. It is **not a medical device**, was not clinically validated, and is not affiliated with Philips or any healthcare manufacturer.

## Overview

VitalWatch is a compact, modular C++17 console application that simulates three sensor channels—**heart rate**, **oxygen saturation (SpO₂)**, and **temperature**—and processes their readings using a producer–consumer pipeline. Its purpose is to make core software engineering concepts visible and testable: thread synchronization, error handling, input validation, file I/O, deterministic debugging, and component-level testing.

Unlike a UI-only demonstration, VitalWatch includes executable test cases and checked-in build and runtime evidence. The project does **not** require real sensors, third-party runtime libraries, a database, or cloud credentials.

### What you can demonstrate

- **Multithreading:** a producer generates/replays samples while a consumer validates, classifies, and logs them.
- **Safe coordination:** a `std::mutex`-protected FIFO queue uses `std::condition_variable` to wait without busy polling and supports graceful close/drain.
- **Fault distinction:** readings are labelled **missing**, **invalid**, **disconnected**, or a **demo threshold alert**, rather than treating every issue as a single alarm.
- **Reproducibility:** fixed deterministic inputs and strict CSV replay make failures straightforward to reproduce.
- **Observable output:** a console table plus `readings.csv` and `events.csv` records what happened.
- **Failure handling:** injected producer/consumer exceptions and malformed inputs are covered by tests.

## Historical Thales verification — 2026-10-22

These results come from the **2026-10-22 Thales preparation audit of this attached source**, using Linux/GCC. The original 23-case suite was rerun before modifications; the current Linux suite has 33 cases. See [the current report](docs/THALES_VERIFICATION.md) for source identity, exact commands, failed diagnostic attempts and limitations. These are not native Windows or production-performance claims.

| Verification | Observed result | Evidence |
| --- | --- | --- |
| Automated checks | **33 / 33 passed** in both Debug and Release, 0 failed, 0 skipped | [`evidence/thales/release-final.txt`](evidence/thales/release-final.txt), [`debug-final.txt`](evidence/thales/debug-final.txt) |
| Producer–consumer FIFO | **1,000 samples** processed once and in order in the `shutdown` test | [`tests/tests.cpp`](tests/tests.cpp), [`docs/VERIFICATION.md`](docs/VERIFICATION.md) |
| Deterministic console demo | **8 readings**, **6 emitted events** | [`evidence/demo-console.txt`](evidence/demo-console.txt), [`evidence/demo-events.csv`](evidence/demo-events.csv) |
| Larger pipeline workload | **10,000 generated and processed IDs**, zero missing, duplicate or out-of-order entries in the tested run | `large_fifo`, `sequence_audit`; [`tests/concurrency_tests.cpp`](tests/concurrency_tests.cpp) |
| Sample replay | Current test compares generated/replayed reading and event CSVs | `deterministic_replay`; [`evidence/thales/release-final.txt`](evidence/thales/release-final.txt) |
| Malformed replay input | Rejected with non-zero exit and line-number diagnostic | [`evidence/malformed-input.txt`](evidence/malformed-input.txt) |
| SIGINT interruption | Automated Linux check waits for a flushed sample, sends SIGINT, checks exit and summary/row agreement | `signal_shutdown`; [`tools/check-signal.sh`](tools/check-signal.sh) |
| UndefinedBehaviorSanitizer / ThreadSanitizer | **33/33 passed in each separate configuration**, no diagnostics reported | [`evidence/thales/ubsan-final.txt`](evidence/thales/ubsan-final.txt), [`tsan-final.txt`](evidence/thales/tsan-final.txt) |
| AddressSanitizer + UBSan | **33/33 with leak detection explicitly disabled**; default leak-enabled run failed in this environment | [`evidence/thales/asan-ubsan-no-leaks.txt`](evidence/thales/asan-ubsan-no-leaks.txt); [limitations](docs/THALES_VERIFICATION.md) |

**Environment:** Linux x86-64 · GCC 13.3.0 · CMake/CTest 3.28.3 · C++17. Native Windows/MSVC and GitHub-hosted CI execution are pending. No current coverage or throughput measurement is claimed. Archived evidence directly under evidence/ and docs/VERIFICATION.md preserves the older session; evidence/thales/ records the current audit.

## Tech stack

| Area | Technologies / design |
| --- | --- |
| Language | **C++17**, Standard Template Library (STL) |
| Concurrency | `std::thread`, `std::mutex`, `std::condition_variable`, `std::atomic` |
| Data structures | `std::queue`, `std::vector`, `std::optional` |
| Build & test | **CMake**, **CTest**, GCC/Linux verification; MSVC configuration supported but unverified |
| Inputs | Synthetic generator, configurable thresholds, strict CSV replay |
| Outputs | Console report, timestamped readings/events in CSV |

## Architecture

```mermaid
flowchart TD
    A[CLI options / config] --> B[Producer thread]
    B --> C[Sensor simulator or CSV replay]
    C --> D[Thread-safe FIFO queue]
    D --> E[Consumer thread]
    E --> F[Reading validator]
    F --> G[Demo threshold engine]
    G --> H[CSV event logger]
    G --> I[Console dashboard]
```

The producer and consumer have separate responsibilities. The queue guards shared state with a mutex and uses a condition variable to wake the consumer when new data arrives or the queue closes. The processing path validates input **before** applying demo alert rules, so disconnected or invalid values are not interpreted as threshold events.

For component ownership, error propagation, shutdown semantics, requirements traceability, and design trade-offs, read **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)**.

For the queue's deque backing, operation complexity, memory bounds and the distinction between input/dequeue/completion/output order, read **[docs/CONCURRENCY.md](docs/CONCURRENCY.md)**. The application still has exactly one producer and one consumer.

## Quick start

### Option A — Linux (verified toolchain family)

**Prerequisites:** a C++17 compiler (GCC 9+ recommended), CMake 3.16+, and Make or Ninja.

From the directory containing `CMakeLists.txt`:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/vitalwatch --deterministic --count 8 --interval-ms 0 --log-dir logs-demo
```

The reproducible warning-strict helper keeps configurations separate:

```bash
bash tools/check.sh release
bash tools/check.sh debug
shellcheck tools/*.sh
```

It checks prerequisites, returns 2 for invalid usage/profile, 127 for missing prerequisites, and propagates configure/build/CTest failures. A C++17 compiler, CMake, CTest and a Make/Ninja build tool are required. ShellCheck is an optional developer tool. Run `bash tools/check.sh ubsan`, `asan-ubsan`, or `tsan` for separate Debug diagnostic builds. Never combine TSan and ASan in one executable. See [diagnostic instructions](docs/THALES_VERIFICATION.md) before interpreting results.

The final command should print a table of eight synthetic readings and write two CSV files under `logs-demo/`. To inspect them:

```bash
cat logs-demo/readings.csv
cat logs-demo/events.csv
```

### Option B — Windows 11 with Visual Studio Build Tools (instructions; not yet verified)

**Prerequisites:** Visual Studio 2022 Build Tools with **Desktop development with C++** and a Windows SDK, plus CMake 3.16+.

Run from the project folder in **PowerShell**:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
.\build\Release\vitalwatch.exe --deterministic --count 8 --interval-ms 0 --log-dir logs-demo
Get-Content .\logs-demo\readings.csv
Get-Content .\logs-demo\events.csv
```

If Visual Studio 2022 Build Tools aren't installed, get them via the [official Microsoft instructions](https://learn.microsoft.com/en-us/cpp/overview/acquire-msvc?view=msvc-170), then install [CMake](https://cmake.org/download/). On Windows, the default generator may differ; the explicit command above assumes VS 2022. Compilation/testing on your own system is needed before claiming Windows compatibility has been verified.

> **No application login is needed.** This is a C++ console simulator, not a hosted web application or a patient dashboard.

## Sample console output

*Excerpt from the captured deterministic run (not a rendered UI screenshot):*

```text
VitalWatch | EDUCATIONAL SIMULATOR | NON-CLINICAL DEMO THRESHOLDS
HR: 60..100  SpO2: 90..100  Temp C: 36..38
UTC timestamp         HR BPM   SpO2 %   Temp C   Sensor/event status
2026-01-01T00:00:00Z     75.0     97.0     37.0  CONNECTED
2026-01-01T00:00:01Z    120.0     97.0     37.0  CONNECTED | DEMO_ALERT:heart_rate
2026-01-01T00:00:02Z     75.0     85.0     37.0  CONNECTED | DEMO_ALERT:oxygen
2026-01-01T00:00:04Z     -5.0     97.0     37.0  CONNECTED | INVALID:heart_rate
2026-01-01T00:00:05Z     75.0       --     37.0  CONNECTED | MISSING:oxygen
2026-01-01T00:00:06Z       --       --       --  DISCONNECTED | DISCONNECTED:unit
Processed 8 readings. Logs: logs-demo
```

Read the entire real output: [`evidence/demo-console.txt`](evidence/demo-console.txt).

## How it works

1. **Read options and inputs.** `main.cpp` validates CLI options, optional configuration, and replay files before worker startup.
2. **Generate or replay data.** The producer emits synthetic readings or a prepared sequence of CSV rows.
3. **Transfer safely.** A thread-safe FIFO queue transfers each accepted reading to the consumer.
4. **Validate and classify.** `ReadingValidator` distinguishes disconnection, missing, and invalid values; `AlertEngine` checks valid values against configurable **demo** intervals.
5. **Record evidence.** `EventLogger` writes readings and emitted events to CSV and the program prints a console status row.
6. **Shut down safely.** On normal completion or a producer stop, the queue closes, pending accepted items drain, and threads join. Consumer failure closes the queue and surfaces an error rather than promising data retention.

### Default demonstration intervals (not clinical guidance)

| Synthetic channel | Input domain | Demonstration interval |
| --- | --- | --- |
| Heart rate | 0–300 BPM | 60–100 BPM |
| SpO₂ | 0–100% | 90–100% |
| Temperature | 20–50 °C | 36–38 °C |

The bounds are inclusive. Adjust alert rules using [`data/demo_thresholds.cfg`](data/demo_thresholds.cfg). Invalid configuration keys/values are rejected; the parser does not accept arbitrary inline comments.

## Useful CLI scenarios

| Goal | Command (Linux path shown) |
| --- | --- |
| Reproducible eight-sample demo | `./build/vitalwatch --deterministic --count 8 --interval-ms 0 --log-dir logs-demo` |
| Seeded synthetic samples | `./build/vitalwatch --seed 7 --count 16 --log-dir logs-seeded` |
| Replay supplied CSV and configuration | `./build/vitalwatch --replay data/sample_replay.csv --config data/demo_thresholds.cfg --interval-ms 0 --log-dir logs-replay` |
| Verify malformed input handling | `./build/vitalwatch --replay data/malformed_replay.csv` (expected non-zero exit) |
| List all available options | `./build/vitalwatch --help` |

On Windows, replace `./build/vitalwatch` with `.\build\Release\vitalwatch.exe`. Every run **overwrites** both output CSVs within the chosen log directory. Use distinct `--log-dir` values when you need to preserve previous runs. The output CSV schema is not the same as the replay input schema.

## Testing and reproducibility

The current CMake configuration retains all 33 checks from the Thales baseline and adds six firmware-reliability cases: **39 cases on this Linux/Bash environment**. Other supported build targets register 37 portable cases; that is a platform registration difference, not 2 skipped tests. Windows execution remains pending. Each test has a 10-second timeout.

```bash
ctest --test-dir build --output-on-failure
```

For a focused test:

```bash
ctest --test-dir build -R "^(shutdown|producer_error|consumer_error)$" --output-on-failure
```

The suite uses throwing checks that still run in Release. The Linux helper enables `-Wall -Wextra -Wpedantic -Werror`. CLI checks verify both exact exit codes and diagnostics; output matching cannot hide a failing process exit. The badge above refers to local recorded execution. `.github/workflows/build.yml` configures Linux Debug/Release and ShellCheck automation; it has **not been executed on GitHub** in this session.

More detail: [Testing & debugging](docs/TESTING.md) · [Verification report](docs/VERIFICATION.md) · [Raw CTest evidence](evidence/ctest-release.txt).

## Project structure

```text
VitalWatch/
├── CMakeLists.txt             # C++17 build and CTest registration
├── README.md                  # You are here
├── include/vitalwatch/        # Core component interfaces and types
├── src/                      # C++ implementation and CLI entry point
├── tests/tests.cpp           # Unit/integration/CLI support checks
├── data/                     # Synthetic replay and demo rule fixtures
├── evidence/                 # Captured build, test, console and CSV evidence
└── docs/
    ├── ARCHITECTURE.md       # Architecture, requirements and SDLC
    ├── TESTING.md            # Test strategy and debugging guidance
    ├── VERIFICATION.md       # Exact observed metrics and limitations
    ├── INTERVIEW.md          # Interview explanations and Q&A
    ├── DEMO_GUIDE.md         # Short recruiter-friendly demo
    └── GITHUB_PUBLISHING.md  # Safe upload instructions for Windows
```

## Design decisions & trade-offs

- **C++17 and the STL** demonstrate standard threads, data ownership and low-level synchronization without external runtime packages. This is a learning and software-design choice, **not a performance superiority claim**.
- **One consumer writes logs** to avoid concurrent file-stream writes; CSV prioritizes traceability over persistence/performance.
- **Strict replay parsing** favors reproducibility but intentionally supports a narrow CSV format (unquoted, five fields).
- **Unbounded CLI default and preloaded replay** preserve the original workflows. Library callers can opt into bounded capacity/backpressure; streaming large replay datasets remains future work.
- **Deterministic tests** verify exercised scenarios, not every possible thread interleaving or guaranteed hard real-time behavior.

## Limitations and responsible use

- No real sensors or patient data; no clinical, medical-device, safety, certification, or standards claims.
- No GUI/web portal, hardware firmware, cloud deployment, verified hosted CI run, or throughput benchmark.
- Windows/MSVC build and Windows Ctrl+C behavior remain **unverified**.
- No alarm persistence/acknowledgement, timed missing-data watchdog or per-channel link states. Bounded-queue backpressure is available only through the library/test API, not a new CLI option.
- Logs are for local demonstrations; they are not a secure clinical audit trail. Forced process termination can interrupt output.

These limitations are intentional and discussed further in [architecture documentation](docs/ARCHITECTURE.md).

## Documentation

| Document | What you'll find |
| --- | --- |
| [Architecture](docs/ARCHITECTURE.md) | System diagram, module responsibilities, thread ownership, SDLC and trade-offs |
| [Concurrency and DSA](docs/CONCURRENCY.md) | Queue choice and complexity, ownership, four orders, shutdown and test limitations |
| [Current audit](docs/THALES_VERIFICATION.md) | Current build, diagnostic results, source identity and skill evidence table |
| [Thales interview notes](docs/THALES_INTERVIEW.md) | Simple project-specific answers and verified resume bullets |
| [Testing](docs/TESTING.md) | Tests, debugging cases and Windows validation checklist |
| [Verification](docs/VERIFICATION.md) | Observed results, supporting evidence and what remains untested |
| [Interview preparation](docs/INTERVIEW.md) | Short project pitch, key decisions, 15 likely technical questions and suggested resume bullets |
| [5-minute demo](docs/DEMO_GUIDE.md) | Concise demo steps for reviewers and placement interviews |
| [GitHub publishing](docs/GITHUB_PUBLISHING.md) | Safe Windows upload steps and updating an already staged README |

## Roadmap (not yet implemented)

- [ ] Verify build/tests on native Windows/MSVC; capture new evidence.
- [ ] Execute the included Linux GitHub Actions workflow after publishing; native Windows CI is still future work.
- [x] Add optional library-level bounded capacity with tested backpressure; preserve the unbounded CLI default.
- [ ] Support timeout-based missing-data detection and log rotation.
- [ ] Improve independent per-channel connection states.

## License and project status

**License:** No license is included in the supplied project. The repository should not claim MIT, Apache-2.0, or any other license until the owner explicitly selects one. Public GitHub visibility does **not** by itself grant reuse or redistribution rights.

**Author:** [Mohammad Yusuf K A](https://github.com/yusufthecoder474) · Educational portfolio project · Current Linux audit: 2026-10-22; original historical verification: 2026-10-15.
