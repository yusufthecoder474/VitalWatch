# Testing and debugging

## Run the suite

From project root, after the README build commands:

```powershell
ctest --test-dir build -C Release --output-on-failure
ctest --test-dir build -C Release -R "^(shutdown|producer_error|consumer_error)$" --output-on-failure
.\build\Release\vitalwatch_tests.exe malformed
```

On Linux use the same CTest commands without `-C Release`, and `./build/vitalwatch_tests malformed`. There are **23 registered CTest cases**: 20 named core/component/integration cases and 3 CLI cases. A case can perform several checks. Do not describe these as 23 unit tests or as a coverage percentage.

## Strategy

- Unit/component checks exercise input domains, every demo channel rule, exact bounds, configuration, deterministic generation and nonfinite values.
- Integration checks verify FIFO delivery of 1,000 samples, empty-source shutdown, close/drain semantics, producer exceptions, consumer exceptions, strict replay parsing, real file output and a failed output path.
- CLI checks exercise the deterministic demo, invalid count and a missing argument.
- `require()` throws on failure and the test runner returns 1. No checks depend on C `assert`, so `-DNDEBUG` cannot disable them.
- Each CTest process has a 10-second timeout. The `shutdown` case verifies all 1,000 timestamps in order, catching lost/duplicated/reordered deliveries as well as hangs.
- The CSV case compares the complete known sample row and verifies quote escaping. Generated fixtures use case-specific directories and remove them after execution.
- `WILL_FAIL` CLI cases expect a nonzero exit; they do not validate exact diagnostic wording. CTest also reports exceptional process failures.

## Repeatable demo expectations

Eight deterministic samples produce 8 data rows in readings.csv and 6 event rows in events.csv:

| Sample index | Expected behavior |
| --- | --- |
| 0 | Connected normal values |
| 1 | HR demo alert |
| 2 | Oxygen demo alert |
| 3 | Temperature demo alert |
| 4 | Invalid HR; no demo alert for that value |
| 5 | Missing oxygen |
| 6 | Disconnected unit; no missing-value duplicates |
| 7 | Connected normal values again |

## Actual defect investigation example

**Symptom:** The first compiler run reported `expected '}' at end of input` in SensorSimulator.cpp and identified the opening `namespace vitalwatch` brace.

**Investigation:** The `next()` function closed correctly; the surrounding namespace did not. This was a source-structure error before runtime, unrelated to threading.

**Fix:** Add the namespace closing brace. The same compiler pass flagged misleading indentation in the replay line-reading lambda. Separate the return from the conditional line-ending cleanup. Expanded similar one-line branches in the test callbacks and compiled the application with `-Wall -Wextra -Wpedantic -Werror`.

**Verification:** Rebuild, run the deterministic generator check and the complete CTest suite. Compiler diagnostics and tests answer different questions: compilation catches syntax/type issues, while tests check runtime behavior.

## Fault investigation practice

These are reproducible exercises, not claims about past production incidents:

- Set oxygen to `101`: expect `INVALID`, with no `DEMO_ALERT` for oxygen. Trace `validValue()` in both validator and engine.
- Blank the oxygen field: expect `MISSING`. Empty optional values are checked before dereferencing.
- Set `connected=0` with a high stale HR: expect only disconnection. The dedicated regression case checks precedence.
- Supply a malformed replay value: expect the file line number and a nonzero process exit before worker startup.
- Remove `close()` from a local experimental copy: an empty consumer may wait forever. Restore it; CTest timeout and shutdown cases detect this behavior.

## Manual Windows checklist

Build with MSVC Release; run all 23 CTest cases; run the sample replay; inspect both CSVs; run 100 samples with 1000 ms interval and press Ctrl+C after a few rows. Confirm a `Processed ...` summary and exit without a hang. A Linux pass does not substitute for this native Windows check.
