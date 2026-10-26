# VitalWatch: current-source verification for Thales preparation

Historical 2026-10-22 audit. The latest enhancement and current source identity are documented in [FIRMWARE_RELIABILITY.md](FIRMWARE_RELIABILITY.md) and the root MiPhi-Project-Verification-Report.md. The original evidence and manifest below are intentionally preserved.

Session date: **2026-10-22**. Source: attached `VitalWatch_GitHub_Ready_Project(1).zip`. This is a software-engineering audit of a synthetic simulator, not clinical validation.

## Source identity and scope

- Input ZIP SHA-256: `cedcc3a490514ef5326e642960b3054cce4ea3311e6fb64898abd7d8c0ae64f9`.
- The ZIP contains no `.git` directory or commit identifier. No upstream Git revision is invented. The final source identity is the SHA-256 of `evidence/thales/source-files.sha256`, recorded in `source-revision.txt` beside it.
- That manifest covers CMakeLists.txt, .gitignore and all files under src/, include/, tests/, tools/ and .github/. It excludes reports, captured logs and generated build artifacts so reporting does not create a circular identity.
- Final content revision: `sha256:2fc425bc2f422df88d5d9830f11b1d6cef149761a162743c9610db35308e51aa` (hash of that source manifest, not a Git commit).
- No AGENTS.md/repository instruction file was present in the archive or applicable workspace parents. All source/header files, tests, CMake, data fixtures and existing documentation were inspected.
- No LICENSE/COPYING file or CI workflow was supplied. No license has been added and the existing author attribution is preserved. A Linux CI workflow was added, but no GitHub-hosted run or publication was performed.

The original ZIP has the **23-case implementation**, regardless of different audit results discussed in earlier conversations. Those earlier results were not substituted for execution on this source.

## Environment

Linux 6.18.55 x86-64, GCC 13.3.0, CMake/CTest 3.28.3, ShellCheck 0.9.0. See `evidence/thales/environment.txt`. GCC diagnostic runtimes are recorded in `tsan-libraries.txt`, `asan-libraries.txt` and `sanitizer-flags.txt`.

Release uses `-O3 -DNDEBUG`; Debug uses `-g`. Both use C++17 with `-Wall -Wextra -Wpedantic -Werror`. Throwing test checks remain active in Release. No third-party application dependency or C++ standard change was introduced.

## Baseline audit and justified changes

1. **Reproduced original checks:** fresh GCC/Linux Release build, **23 passed, 0 failed, 0 skipped**. Logs: baseline-configure.txt, baseline-build.txt and baseline-ctest.txt under evidence/thales/.
2. **Found a real reporting bug:** redirected console output to `/dev/full`. All CSV writes could complete while stdout failed silently and the application returned **0**. Raw reproduction: baseline-stdout-error.txt. Enabled cout error exceptions and explicit flushes before successful exits. The new regression initially caught a secondary failure: cerr's default tie flushed the already-failed cout and caused termination. The final catch path disables cout exceptions before reporting to cerr. Current stdout_error requires exit **1** and a single error diagnostic, rejecting additional unrelated runtime diagnostics.
3. **Strengthened CLI verification:** the old PASS_REGULAR_EXPRESSION test could accept a printed demo summary even when the process exited nonzero; generic WILL_FAIL cases could accept unrelated errors. CMake wrappers now check exact expected exit codes and stderr, plus the valid demo summary. This was particularly relevant when leak-runtime failure appeared after otherwise valid output.
4. **Added focused tests:** empty source, one sample, 10,000 samples, sequence-audit negative controls, close-versus-wait, forced queued work, exact worker exception propagation, generated/replay CSV equality, failed stdout and actual SIGINT shutdown.
5. **Added build automation:** one Bash helper, warning-as-error/sanitizer CMake options, separate build directories, ShellCheck and a configured Linux Debug/Release GitHub Actions workflow.

The queue, data domains, rules, replay formats, component architecture, worker count, CLI options and normal demo output are preserved. No concrete queue data race was found during source inspection or the executed TSan tests. The queue remains unbounded; backpressure is not implemented.

## Reproducible commands

From the project root, original baseline before editing:

```bash
cmake -S . -B build-baseline -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-Werror
cmake --build build-baseline --parallel 2
ctest --test-dir build-baseline --output-on-failure
./build-baseline/vitalwatch --deterministic --count 8 --interval-ms 0 --log-dir logs-baseline-full > /dev/full
echo "$?"
```

The last historical command returned 0; the repaired program returns 1. `build-baseline` is generated evidence, not included in the ZIP. To reconstruct that baseline, use the supplied original archive rather than current modified source.

Current checks:

```bash
bash tools/check.sh release
bash tools/check.sh debug
bash tools/check.sh ubsan
bash tools/check.sh tsan
shellcheck tools/*.sh
```

The helper resolves the project directory, checks cmake/ctest/compiler prerequisites, configures a separate directory with warnings-as-errors, builds and runs CTest with output on failure. It enters the build directory before CTest, retaining compatibility with CMake 3.16 rather than requiring newer --test-dir support. A Make/Ninja generator tool must also be installed. Invalid usage/profile returns 2; missing prerequisite returns 127; real configure/build/test failures propagate. Exit checks are in script-exits.txt.

Sanitizers are separate configurations. `asan-ubsan` compiles/links with `-fsanitize=address,undefined`, `ubsan` with `-fsanitize=undefined`, and `tsan` with `-fsanitize=thread`. They also use `-fno-omit-frame-pointer` and `-fno-sanitize-recover=all`. Default runtime settings are halt_on_error=1, with ASan leak detection enabled and UBSan stack traces enabled. User-supplied runtime settings are retained, not silently overridden.

## Final executed results

| Configuration / workload | Passed | Failed | Skipped | Evidence under evidence/thales/ |
| --- | --- | --- | --- | --- |
| Original Release baseline | 23 | 0 | 0 | baseline-ctest.txt |
| Current Release | 33 | 0 | 0 | release-final.txt |
| Current Debug | 33 | 0 | 0 | debug-final.txt |
| Current Debug + UBSan | 33 | 0 | 0 | ubsan-final.txt |
| Current Debug + TSan | 33 | 0 | 0 | tsan-final.txt |
| Current Debug + ASan/UBSan, leak detection explicitly disabled | 33 | 0 | 0 | asan-ubsan-no-leaks.txt |
| Default leak-enabled ASan/UBSan | 0 | 33, with leak-runtime failure | 0 | asan-ubsan-leaks-final.txt; diagnostic-exits.txt; not a passing configuration |
| ShellCheck | Exit 0, no diagnostics | 0 reported | N/A | shellcheck-status.txt; shellcheck.txt |

The 33 cases consist of the original 20 component/integration cases, 3 stricter CLI checks, 8 new C++ cases and 2 Linux-specific checks. With non-Linux CMake targets, the portable registration is 31 cases, **not** 33 with two skipped. On Linux without Bash the signal check is not registered; the 33-case result requires the recorded Bash environment.

No crash, timeout or sanitizer race/undefined-behavior diagnostic was reported in the final successful configurations. Initial failed development runs are retained rather than overwritten: release-first-attempt.txt/debug-first-attempt.txt show the secondary cout/cerr bug. The initial TSan build had a non-executable application artifact while its two test executables ran; a normal clean rebuild restored the executable mode (tsan-rebuild.txt), and the final full TSan run succeeded. That artifact issue is not described as a source concurrency defect.

## Address checks versus leak checks

The default command **was executed and failed**:

```bash
bash tools/check.sh asan-ubsan
```

LeakSanitizer reported that it could not inspect `/proc/.../task`, emitted a fatal error and a hint about ptrace restrictions. This runtime message is not treated as proof that the project leaks memory, nor as proof of the exact host restriction. To distinguish environment failure from application code, this minimal non-leaking allocation/delete program was compiled and executed:

```bash
g++ -std=c++17 -O0 -g -fsanitize=address,undefined evidence/thales/lsan_probe.cpp -o build-probes/lsan_probe
ASAN_OPTIONS=halt_on_error=1:detect_leaks=1 ./build-probes/lsan_probe
ASAN_OPTIONS=halt_on_error=1:detect_leaks=0 ./build-probes/lsan_probe
```

The leak-enabled probe failed with the same runtime message and exit 1; the address-check-only probe exited 0. Raw evidence: lsan-probe.txt and asan-probe-no-leaks.txt.

Then a **distinct, explicitly limited** run was executed:

```bash
ASAN_OPTIONS=halt_on_error=1:detect_leaks=0 bash tools/check.sh asan-ubsan
```

It passed 33/33 with no reported address/UB diagnostics. This does not turn the default failure into a pass. Leak detection remains **unverified** and should be rerun on a compatible host. No suppressions were added to hide application errors; the raw failed default run is retained. Report the exact scope if citing ASan results.

## Workload, order and shutdown evidence

| Test | Workload and expected behavior | Actual recorded result / boundary |
| --- | --- | --- |
| shutdown (original) | 1,000 consecutive timestamped samples, complete FIFO sequence, then empty source | Passed again; finite tested run, not universal exactly-once proof |
| empty_pipeline | Source immediately ends; sink must not run | Passed |
| one_sample | Exactly one generated/processed ID | Passed |
| large_fifo | 10,000 generated/processed IDs; no missing, duplicate, unexpected or decreasing IDs | All expected; large-fifo.txt records counts. In-memory pipeline workload, not a 10,000-row CSV benchmark |
| sequence_audit | Deliberately drop, duplicate, reorder and inject an unexpected ID | All four corruptions detected; sequence-negative-controls.txt and source assertions |
| queue_wait_close | Worker announces entry before pop; controller closes with no sleep | Passed for executed schedule; covers close/pop race without asserting the OS physically parked the worker |
| queued_drain | Hold first callback until producer enqueues remaining 15 samples and exhausts source | All 16 completed in order after release |
| exact_worker_errors | Producer throws after 3 accepted samples; consumer throws a distinct sentinel | Exact runtime_error messages propagated; 3 producer-accepted samples drained |
| deterministic_replay | Known 8-row fixture and deterministic generator through queue/validator/rules/logger | 8 readings and 6 events each; reading/event CSV contents equal |
| stdout_error | Redirect output to /dev/full | Repaired app exits 1 with a single error diagnostic |
| signal_shutdown | Wait for a flushed CSV sample, send SIGINT, compare summary and CSV row count | Exit 0, one sample recorded in the captured Release run; signal-console.txt and signal CSVs |

All CTest cases have ten-second timeouts; CLI wrappers additionally use five-second execute_process timeouts. C++ coordination uses promises/futures and bounded readiness waits, not sleeps to guess a schedule. The signal check polls observable CSV readiness with a short polling interval; its correctness does not require startup to finish within an arbitrary fixed delay. Interrupt sample count depends on timing and is not a benchmark.

The eight-row CLI was also executed after changes; the two replay CSVs matched its output byte-for-byte. Evidence: demo-console.txt, demo-readings.csv, demo-events.csv, replay-console.txt and replay-comparison.txt. The new large workload is a targeted correctness test; no endless stress loop, performance benchmark, throughput, latency or coverage measurement was added.

## Skill evidence table

| Skill | Implementation | Verification | Result | Limitation |
| --- | --- | --- | --- | --- |
| C++ / ownership | C++17 value objects, optional channels, RAII locks/streams, joined worker lifetimes | Strict Debug/Release build and tests | 33/33 in each | Native Windows untested |
| DSA reasoning | deque-backed std::queue; vector replay; O(n+k) bitmap sequence audit | One/1,000/10,000 sample checks plus corrupt-sequence controls | Expected counts/order and controls passed | Unbounded queue; no backpressure claim |
| Threads / synchronization | One producer, one consumer; mutex protects container and closed flag; predicate CV wait | Close/pop, held-sink drain, worker failures, TSan | 33/33 TSan configuration | Finite schedules only; no universal race-free guarantee |
| OS concepts | Shared address space, blocking wait, joins, minimal atomic SIGINT handler | Automated signal check and ownership audit | Exit 0 with summary/CSV agreement | No hard real-time deadline; blocked callbacks remain a risk |
| Linux / scripting | Bash configure/build/test profiles; prerequisite and exit checks | ShellCheck 0.9.0 plus executed helper profiles | ShellCheck exit 0; usage 2/missing prerequisite 127 | Requires suitable compiler/runtime and Make/Ninja |
| Testing / debugging | Throwing Release checks; exact CLI results; reproduced stdout bug | Full suites, /dev/full regression, sentinel errors | Current suites pass in stated scope | Default leak scan environmentally blocked |
| Build automation / CI | CMake/CTest and Linux GitHub Actions Debug/Release workflow | Local script executed; workflow source inspected | Local build/test verified, workflow configured | No hosted Actions run or deployment verified |

## Native Windows handoff (CMD)

With VS 2022 Build Tools, Desktop development with C++, a Windows SDK and CMake installed, run from the project root:

```bat
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 -DVITALWATCH_WARNINGS_AS_ERRORS=ON
cmake --build build-msvc --config Release --parallel 2
cd build-msvc
ctest -C Release --output-on-failure
cd ..
build-msvc\Release\vitalwatch.exe --deterministic --count 8 --interval-ms 0 --log-dir logs-msvc
type logs-msvc\readings.csv
type logs-msvc\events.csv
```

Expected portable registration is 31 cases; the /dev/full and Linux signal checks are not registered. Manually run 100 samples with a 1,000-ms interval, press Ctrl+C once after a few rows, then immediately `echo %ERRORLEVEL%` and compare the summary with CSV data-row count. No Windows/MSVC compilation, sanitizer execution or Ctrl+C result was performed here.

## Reproduce source identity

From project root:

```bash
sha256sum -c evidence/thales/source-files.sha256
sha256sum evidence/thales/source-files.sha256
```

The first checks code/build/helper contents against the manifest; the second reproduces the recorded content revision. README/report edits are deliberately outside that code manifest. The updated ZIP excludes binaries and build directories. See [CONCURRENCY.md](CONCURRENCY.md) and [THALES_INTERVIEW.md](THALES_INTERVIEW.md) for explanations and the three final resume bullets.
