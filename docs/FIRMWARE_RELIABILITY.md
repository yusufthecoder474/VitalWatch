# Firmware reliability additions — current verification

Date: 2026-10-26. This update is limited to the four requested areas: sanitizer CMake options, deterministic fault/backpressure tests, a Python executable validator and performance measurement. It retains C++17, existing CLI options/data schemas, the original test files, synthetic scenarios, one producer and one consumer. No GUI, database, hardware, networking service or additional production worker was added.

## Current baseline and source

Input: recovered `VitalWatch_Thales_Verified.zip`, SHA-256 `3688ea8a697c908b3ae9a437aaf38de783fc0972a659fc62d42d84915ffcfcd7`. No upstream Git revision was supplied. Before modifying project source/CMake/queue/tests, this exact input was configured, built and tested: **33/33 passed**, 0 failed, 0 skipped. The historical 23/23 claim was not used as the current count. Raw commands/output are in evidence/firmware/baseline-configure.txt, baseline-build.txt and baseline-tests.txt.

The Python validator was initially written outside the project and executed against that unmodified baseline binary: **15/15 scenarios passed**. Its five 10,000-sample runs establish the pre-change timing baseline. See baseline-validation/SUMMARY.md and results.csv. Baseline source checksums were captured before project edits. The current manifest and its content revision are in evidence/firmware/current-source.sha256 and source-revision.txt. Reports and captured data are outside the code manifest.

The input includes older historical documentation/evidence, which is retained. The current README section and this document supersede old claims that all queue modes are unbounded or that MSVC ASan configuration is unavailable. No license file was supplied and none was added; existing author attribution is unchanged.

## A. Sanitizer options

| Toolchain | ASan | UBSan | Status |
| --- | --- | --- | --- |
| GCC/Linux | VITALWATCH_ENABLE_ASAN=ON | VITALWATCH_ENABLE_UBSAN=ON | Combined and UBSan-only executed |
| Clang with compatible runtime | Same option | Same option | Implemented; no Clang execution here |
| MSVC / VS 2022+ | /fsanitize=address via ASan option | Rejected at configuration | Implemented; native MSVC build/runtime pending |

Existing VITALWATCH_SANITIZER profiles none/asan-ubsan/ubsan/tsan are preserved; an asan profile is also accepted. New options default OFF. ASan+UBSan is supported together on GCC/Clang. A TSan profile combined with either new option is rejected; TSan stays a separate configuration. The rejected-combination command and actual configure failure are recorded in rejected-combination.txt.

MSVC ASan removes /RTC defaults from configuration-specific C++ flags, maps Edit-and-Continue /ZI to /Zi, and links with /INCREMENTAL:NO and /DEBUG:FULL. Do not reintroduce /RTC, /ZI, /DEBUG:FASTLINK or incremental linking through custom base flags or toolchain settings. UBSan and TSan are unsupported by this MSVC recipe. The MSVC branch has not been verified on Windows; no compiler or runtime pass is claimed.

References: [Microsoft ASan support](https://learn.microsoft.com/en-us/cpp/sanitizers/asan), [Microsoft known issues](https://learn.microsoft.com/en-us/cpp/sanitizers/asan-known-issues?view=msvc-170), [GCC instrumentation](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html).

## B. Six deterministic additions, without a framework

All original CTest names, original tests/tests.cpp and tests/concurrency_tests.cpp are retained. Six cases in tests/firmware_tests.cpp make **39 Linux/Bash cases** in this environment. On non-Linux builds the registration is 37 portable cases; that is not a verified Windows pass or a skipped-test count.

| New case | What it checks |
| --- | --- |
| fw_malformed | Malformed HR, SpO2 and temperature numbers plus overflow are rejected with a replay line-2 diagnostic |
| fw_active_shutdown | During the first active consumer callback, the source has accepted three readings; a coordinated stop request ends acquisition and all three drain in order |
| fw_capacity | Capacity-one queue deterministically rejects nonblocking tryPush when full; a push can complete when pop frees capacity; FIFO and close/drain still hold |
| fw_close_saturated | Closing a full capacity-one queue releases/rejects a pending producer and retains the accepted item for draining |
| fw_bounded_exception | Producer error drains three accepted items; consumer error closes capacity-one queue so a capacity-waiting producer can exit; sentinel errors propagate |
| fw_replay_mismatch | A valid changed HR replay is distinguished from the frozen deterministic reference; it does not wrongly reject valid replay syntax |

Known-reference mismatch is a test/validator negative control, not an application medical/fault rule. A valid changed replay should be processed faithfully. The Python validator checks both that the mismatch is detected and that the application reproduced the deliberately changed value.

The queue now accepts an optional capacity. Zero, the default, means the original unbounded behavior. A capacity-aware producer wait uses a second condition variable, space_; pop notifies it after removing an item, and close wakes both producer and consumer waiters. Container state, capacity checks and the closed flag use the same mutex. tryPush returns false on full/closed without blocking. runPipeline has an optional fourth queueCapacity argument defaulting to zero; the unchanged application CLI never selects a bounded capacity. Tests opt in at the library level. There is no drop policy or enqueue timeout.

The saturation precondition is deterministic via a known full queue and rejected tryPush. Handshakes do not claim the OS physically parked a producer at a precise instant; close-versus-entry and pop-versus-entry schedules must both work. Tests have ten-second CTest timeouts and use promises/futures for coordination, not arbitrary sleeps. Existing Linux SIGINT checks remain intact. Normal/cooperative stop and producer failure drain accepted work; consumer failure may discard queued work and reports failure.

## C. Python validation

tools/validate_vitalwatch.py uses only the Python standard library. Python is a developer verification dependency, not an application runtime dependency. Use Python 3.9+ and choose a fresh output directory for every invocation; reuse is rejected to prevent stale output from masquerading as success.

For five repetitions it runs 15 black-box scenarios: five 8-reading demos, five 10,000-reading runs, supplied replay, malformed sensor input, invalid count, invalid config and a replay-mismatch control. It checks exact reading rows/values, expected event timestamp/type/channel, nonempty messages, summary count, dashboard row count, timestamp uniqueness/order, missing records and exit codes. Repeated reading/event CSV hashes must match. Event message text is not compared byte-for-byte with a frozen prose string.

It writes results.csv, SUMMARY.md and raw stdout/stderr/input/output fixtures. Every CSV row retains the executable command and measured duration. Expected rejection is a passing test only when it has the expected exit code and an error diagnostic; a valid case requires exit 0 and empty stderr. Timeouts are failures. The optional --latency-probe runs the existing firmware test executable's measurement mode and writes latency-results.csv.

## D. Performance measurement

Measured baseline and enhanced batches use the same 10,000-reading CLI command and validator core, five repetitions each. Results and timing boundaries are in [../evidence/firmware/PERFORMANCE.md](../evidence/firmware/PERFORMANCE.md). No improvement claim is made: shared-host CPU load/cache conditions were not controlled and the later repeated batch shows variability.

Whole-process duration uses Python perf_counter_ns around subprocess.run. It includes process startup/exit, acquisition, validation/alerts, CSV writes/flushes, captured console output and Python pipe/wait overhead; it excludes fixture generation and output CSV validation/hashing. Rate is verified sample count divided by measured duration. Data integrity checks run after the timer. No warmup is discarded.

The separate Release probe processes 10,000 in-memory readings with capacity 64 and two existing pipeline workers. std::chrono::steady_clock timestamps immediately before enqueue and at consumer callback entry measure acquisition-to-callback latency, including backpressure wait. They do not measure callback completion, file I/O or clinical response time. Clock calls, timestamp storage, FIFO checks and latency collection add overhead. Buffers are allocated before the timed interval and sorting occurs after it. Per-trial median/p95/max and successful count are recorded; synthetic timestamps are never used as a real-time clock.

## Reproduce

From the extracted project root on Linux:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DVITALWATCH_WARNINGS_AS_ERRORS=ON
cmake --build build-release --parallel 2
(cd build-release && ctest --output-on-failure)
python3 tools/validate_vitalwatch.py --exe build-release/vitalwatch --data-dir data --output evidence/my-validation --repeats 5 --latency-probe build-release/vitalwatch_firmware_tests
```

Sanitizers, each in a clean separate build:

```bash
cmake -S . -B build-ubsan -DCMAKE_BUILD_TYPE=Debug -DVITALWATCH_ENABLE_UBSAN=ON -DVITALWATCH_WARNINGS_AS_ERRORS=ON
cmake --build build-ubsan --parallel 2
(cd build-ubsan && UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ctest --output-on-failure)
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DVITALWATCH_ENABLE_ASAN=ON -DVITALWATCH_ENABLE_UBSAN=ON -DVITALWATCH_WARNINGS_AS_ERRORS=ON
cmake --build build-asan --parallel 2
(cd build-asan && ASAN_OPTIONS=halt_on_error=1:detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ctest --output-on-failure)
```

The recorded leak-enabled run failed in this host's LeakSanitizer runtime. A separately labeled limited run used ASAN_OPTIONS=halt_on_error=1:detect_leaks=0 and passed; this does not establish leak-freedom. The tiny runtime probe and raw diagnostics are retained. Do not silently disable leak detection or label the default run a pass.

For TSan, use VITALWATCH_SANITIZER=tsan in a separate Debug build (no new ASan/UBSan booleans); the recorded run passed. CTest --repeat until-fail:3 was executed on CTest 3.28.3; that repeat syntax requires CTest 3.17+.

Windows CMD instructions, not executed here:

```bat
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 -DVITALWATCH_WARNINGS_AS_ERRORS=ON
cmake --build build-msvc --config Release --parallel 2
cd build-msvc
ctest -C Release --output-on-failure
cd ..
python tools\validate_vitalwatch.py --exe build-msvc\Release\vitalwatch.exe --data-dir data --output evidence\windows-validation --repeats 5 --latency-probe build-msvc\Release\vitalwatch_firmware_tests.exe
cmake -S . -B build-msvc-asan -G "Visual Studio 17 2022" -A x64 -DVITALWATCH_ENABLE_ASAN=ON
cmake --build build-msvc-asan --config RelWithDebInfo --parallel 2
cd build-msvc-asan
ctest -C RelWithDebInfo --output-on-failure
```

## Description and two XYZ resume bullets

VitalWatch is a C++17 educational simulator for three synthetic monitoring channels. One producer and one consumer exchange readings through a synchronized FIFO, validate values, apply demonstration rules and write local CSV output. This enhancement adds optional library-level backpressure, reproducible fault tests, cross-platform sanitizer configuration and measured validation tooling without changing its CLI workflows.

- Validated a C++17 producer-consumer simulator, measured by **39/39 CTest passes under Release, UBSan and TSan**, by testing sensor faults, backpressure, worker exceptions and shutdown.
- Established repeatable verification, measured by **15/15 executable checks and 50,000 FIFO-verified stress samples per batch**, by automating CSV, exit-code, replay-hash and latency checks with Python.

These metrics describe executed synthetic workloads. No NAND/SSD firmware, hardware timing, clinical accuracy, standards compliance, speedup or production guarantee is claimed. See the root MiPhi-Project-Verification-Report.md for final results and the pending second-project section.
