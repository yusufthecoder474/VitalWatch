# Actual verification results

Session date: 2026-10-15. Environment: Linux x86-64, GCC 13.3.0, CMake/CTest 3.28.3. This is verification of an educational software simulator, not clinical validation.

## Build and automated tests

Executed from the project root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-Werror
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Result: **23 tests executed, 23 passed, 0 failed.** There are 20 core/component/integration cases and 3 CLI cases. The test executable was built with `-O3 -DNDEBUG -std=c++17 -Wall -Wextra -Wpedantic -Werror`. Thus checks were exercised in Release with assertions ordinarily disabled; the suite uses throwing checks instead.

The build completed without warnings under warnings-as-errors. See `evidence/configure.txt`, `evidence/build.txt` and `evidence/ctest-release.txt` for captured output. Build time/test elapsed output is not a performance benchmark.

## End-to-end checks

| Check | Actual result | Evidence |
| --- | --- | --- |
| Deterministic 8-sample run | Exit 0; 8 reading rows and 6 event rows | demo-console.txt; demo-readings.csv; demo-events.csv |
| Replay sample with provided config | Exit 0; both CSV files byte-identical to deterministic demo output | replay-console.txt and file comparison during verification |
| Provided malformed replay file | Exit 1; diagnostic identifies replay line 2 | malformed-input.txt |
| SIGINT during a 100-sample, 50 ms run | Exit 0 after 4 samples; summary count matches 4 CSV rows; no forced kill | sigint-console.txt |
| FIFO integration | All 1,000 timestamps received once and in order | `shutdown` CTest case |
| Worker failures | Producer and consumer exception cases passed without timeout | producer_error / consumer_error CTest cases |

Evidence filenames in the table are inside `evidence/`. The 4-sample SIGINT count belongs to this particular manual run; interrupt timing is intentionally not a deterministic test expectation. The deterministic eight-reading scenario is independent of interrupt timing.

## Verification limits

- Native Windows 11/MSVC build and terminal Ctrl+C behavior were **not executed** here. Source uses C++17 standard facilities plus the Windows `gmtime_s` branch, and the README supplies a Visual Studio 2022 x64 procedure. Run it on Windows before claiming Windows validation.
- No live GitHub repository was created or uploaded. Instructions are included for the user's account.
- No hardware, patient data, medical datasets, clinical thresholds, device standards validation, performance benchmarks or code-coverage measurements were used.
- The test suite supports correctness of the exercised scenarios. It does not prove all thread schedules, hard real-time deadlines or filesystem durability during process/power loss.

The deliverable ZIP contains source, synthetic fixtures, documentation and captured text/CSV evidence. It excludes Linux binaries, local build directories and generated scratch log directories. Rebuild on your own operating system.
