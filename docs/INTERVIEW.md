# Project explanation and interview preparation

## 60-second explanation

“VitalWatch is a small C++17 educational patient-monitoring simulator I can use to demonstrate software engineering fundamentals. It generates synthetic heart rate, oxygen saturation and temperature readings. A producer thread generates each sample and puts it into a shared queue. A consumer thread takes samples out, checks for missing, invalid or disconnected sensor data, applies configurable demonstration thresholds and writes readings and events to CSV files. The console shows the same readings and event labels. A mutex protects the queue and a condition variable lets the consumer sleep until work arrives. On completion, the queue closes, accepted samples drain and both threads join. The project includes deterministic scenarios, strict replay parsing and 23 CTest cases with checks that remain active in Release builds. These are non-clinical rules: it is a software simulator, not a medical device.”

## Architecture walkthrough

Start at `main.cpp`. It parses arguments, loads any config/replay, constructs the components and installs a stop handler. It passes two callbacks to `runPipeline()`.

The producer callback owns `SensorSimulator::next()` or advances through replay rows. The simulator returns a `Reading`: timestamp, three optional values and a connection flag. `ThreadSafeQueue` transfers a value copy to the consumer.

The consumer calls `ReadingValidator` for data faults, then `AlertEngine` for threshold violations. It combines events in a `vector`, sends them to `EventLogger`, and prints the dashboard row. Errors return to main only after both workers have joined.

Read the files in this order: Types.h → SensorSimulator.cpp → ReadingValidator.cpp → AlertEngine.cpp → ThreadSafeQueue.h → Pipeline.cpp → EventLogger.cpp → Replay.cpp → main.cpp → tests.cpp.

## Why C++ instead of Python?

The target role emphasizes C/C++. This project gives practice with C++ value types, STL containers, object lifetime, standard threads, synchronization and compiled component integration. The choice demonstrates the role's required skills; it does not claim that this tiny workload needs C++ performance or that Python cannot implement it.

## How threads work here

The producer and consumer can run independently, although the OS decides their actual scheduling. The consumer may validate a previous sample while the producer prepares another. The main thread launches and joins them through `runPipeline()`. There are two worker threads, plus main.

## What the mutex protects

It protects `items_` and `closed_` in `ThreadSafeQueue`. Operations like checking emptiness and popping must happen together under the same lock. The condition variable coordinates waiting; it does not replace the mutex.

## How race conditions are avoided

All queue state uses the mutex. Other mutable objects have a single worker owner. The stop request is a lock-free atomic. Main accesses each worker's exception pointer and final counters only after `join()`. No two workers write a file stream concurrently. This is an ownership-and-synchronization argument, supported by tests; it is not a claim that testing proves every possible schedule.

## How sensor faults are detected

Disconnection takes precedence and suppresses interpretation of stale values. On a connected unit, an empty `optional` means missing. Nonfinite numbers or values outside the simulator's accepted domains mean invalid. Only present, valid, connected values reach demo alert logic. A demo alert means a configured interval was crossed, not a clinical diagnosis.

## Unit and integration tests

Component cases give known values to the validator and engine, check exact bounds, and verify the deterministic simulator. Integration cases run real producer/consumer callbacks, test FIFO delivery of 1,000 samples and injected worker errors, and read real CSV output. CLI cases run the executable. Throwing `require()` checks are independent of `assert` and `NDEBUG`. CTest runs each named case as a process and enforces timeouts.

## How the application was debugged

The actual first compile revealed a missing closing namespace brace in SensorSimulator.cpp. The compiler pointed to its opening brace. Adding the closing brace resolved that syntax error. Warning cleanup separated a misleadingly indented return in replay parsing. A strict GCC compile and the complete runtime suite then checked the repaired source. For future logic problems, first reproduce with deterministic input, identify the failing component, add a focused regression check, then rerun the relevant tests.

## How SDLC was followed

Requirements established three synthetic channels, separate fault categories, inspectable output and reliable termination. Design separated acquisition, transfer, validation, alert rules and logging. Implementation integrated those components incrementally. Verification used compiler diagnostics, Release tests and a deterministic end-to-end run. Traceability connects requirements to named checks. Maintenance notes identify where to change thresholds and how to preserve schemas and thread ownership.

## 15 likely technical interview questions

These are project-based preparation questions aligned with the supplied role requirements, not a verified list of questions Philips asks.

| # | Question | Concise answer |
| --- | --- | --- |
| 1 | What problem does VitalWatch solve? | It demonstrates validating synthetic monitoring data, distinguishing faults from demo alerts, and recording the result. |
| 2 | Which OOP concepts did you use? | Encapsulation separates simulator state, rule configuration and logger streams. Composition integrates the objects; inheritance was unnecessary. |
| 3 | Why use `optional<double>`? | A missing value differs from zero. Optional expresses that explicitly and is checked before dereference. |
| 4 | Why a producer-consumer queue? | It separates acquisition from processing while preserving sample order. |
| 5 | What is a race condition here? | Unsynchronized queue access could make one worker read state while another changes it. The mutex serializes those operations. |
| 6 | Why a condition variable? | The consumer can wait without repeatedly polling. It wakes when work is available or the queue closes. |
| 7 | Why does `wait()` need a predicate? | A wakeup may occur without work. The predicate rechecks queue state under the lock and also recognizes shutdown. |
| 8 | What happens at shutdown? | Close rejects new pushes, queued values drain, then pop returns false and both workers join. |
| 9 | What if a worker throws? | It captures the exception. The queue is closed, workers join, and the exception is rethrown to main for a nonzero exit. |
| 10 | What does RAII do in this project? | Lock guards release mutexes and stream objects close file handles when scope ends. Worker threads are explicitly joined. |
| 11 | How is invalid data different from an alert? | Invalid data fails a simulator input-domain check. An alert needs usable data outside a configurable demo interval. |
| 12 | How do you reproduce a reported defect? | Use deterministic mode or a small replay CSV, retain config and inspect the timestamped events. |
| 13 | Why CMake and CTest? | CMake expresses portable build dependencies; CTest registers and runs named checks consistently. |
| 14 | What does the 1,000-sample test prove? | That tested run delivered every timestamp once in FIFO order and terminated. It is not a throughput benchmark or proof of every schedule. |
| 15 | What would you improve next? | Add bounded-queue backpressure, a missing-data watchdog, log rotation and native Windows CI, keeping fault rules separately testable. |

## Exactly three resume bullets

Use these only after studying the implementation and reproducing the checks yourself. The counts refer to this delivered implementation, not clinical or performance validation.

- Delivered a modular C++17 monitoring simulator, measured by support for **3 synthetic vital-sign channels**, by integrating sensor simulation, validation, configurable alert rules and CSV logging through maintainable components.
- Implemented synchronized data transfer, measured by **1,000 samples verified in FIFO order**, by coordinating producer and consumer threads with a mutex, condition variable and close-and-drain queue.
- Verified application behavior, measured by **23/23 CTest cases passing in Release**, by testing sensor faults, malformed inputs, file output and worker exceptions and resolving compiler-detected defects.
