# VitalWatch interview notes for Thales graduate engineering

This project supplies evidence for C++, STL/DSA reasoning, OS threads, Linux CLI work, build automation, testing and debugging. It has no network protocol, database, Kubernetes or cloud component; those JD topics need separate preparation. The questions below are project preparation, not a verified list of Thales interview questions.

## Short project explanation

“VitalWatch is a small C++17 simulator that generates synthetic monitoring readings. One thread produces readings and a second thread validates them, detects demo faults and writes CSV output. They communicate through a queue protected by a mutex and a condition variable. I can reproduce scenarios with deterministic data and CSV replay. The current Linux suite has 33 checks, including a 10,000-sample ordering test and shutdown tests. An improvement in this version catches failed console writes instead of returning success. The project helped me understand thread ownership, queue behavior and reproducible debugging. It is an educational simulator, not a medical device.”

Use this explanation after studying and reproducing the source. Do not claim sole authorship, a team role or production experience that you have not had.

## Questions and simple answers

| Question | Answer |
| --- | --- |
| Why did you select a queue? | Readings should be handled in arrival order. A FIFO queue exposes the two operations I need: add at the back and take from the front. Its default deque backing gives constant-time end operations |
| Why not erase from a vector's front? | That would shift the remaining elements. I use a vector for preloaded replay and test results, where indexed access or append is useful |
| What is a process versus a thread? | The simulator is one running process with its own address space. Its main, producer and consumer threads are execution paths inside that process and share memory |
| What is a race condition? | Two threads can interfere if they access shared mutable state without suitable coordination. Here, the queue's container and closed flag always use the same mutex |
| What does the mutex protect? | The queue and its closed flag. It keeps operations such as checking and removing the front consistent. It does not need to protect the simulator or CSV streams because those each have one worker owner |
| What does the condition variable do? | It lets the consumer wait without busy polling. New work or close changes the predicate and triggers a notification. The consumer rechecks the predicate under the lock |
| Why use a wait predicate? | A thread may wake without a new item, and notifications can happen before waiting starts. The predicate checks actual state and handles both cases |
| Where could a deadlock occur? | For example, if main joined a worker while holding a lock that the worker needed, or if close were omitted. Here joins happen outside the queue lock and close wakes the consumer. Blocking callbacks remain a limitation |
| Does shutdown drain or cancel? | Normal completion, SIGINT and producer failure drain accepted work. Consumer failure stops processing and may leave queued work unprocessed; it reports failure |
| What happens when a worker throws? | The worker captures an exception_ptr, closes the queue and exits. Main waits for both workers, then the pipeline rethrows for the application's error handler |
| Why not log from the signal handler? | It could interrupt code already holding stream or allocation locks. The handler only stores true in a lock-free atomic flag; normal code does the shutdown work |
| Is FIFO completion guaranteed? | In this implementation, one consumer finishes each synchronous callback before the next, so successful processing and output follow FIFO order. Adding multiple consumers would require separate reasoning about completion order |
| What did the 1,000-sample test demonstrate? | That the tested run delivered all 1,000 timestamps once and in order and terminated. It does not prove every possible scheduling order or performance requirement |
| What does the larger test add? | It checks a 10,000-item run using a bitmap for missing/duplicate IDs and checks order. Negative controls deliberately corrupt sequences to verify the checker detects problems |
| What bug did you investigate? | A failed redirected console write initially returned 0. Enabling stream errors exposed that cerr automatically flushed the failed cout. The final error path disables cout exceptions before reporting to cerr and flushes before success |
| What does ThreadSanitizer passing mean? | No race report appeared in the executed instrumented tests. It does not prove the entire program is race-free for every input or schedule |
| Why did LeakSanitizer fail? | Its runtime could not inspect process/thread state in this environment. A separate tiny program showed the same failure. The default run is recorded as failed, and address checks without leak detection are reported separately |
| How do you reproduce the build? | From the project root, run bash tools/check.sh release. Debug, UBSan, ASan+UBSan and TSan profiles use separate directories and fail on compiler warnings or test failures |
| What would you improve next? | For longer acquisition runs I would consider a bounded queue with a clear backpressure policy, then verify its blocking and cancellation behavior. I kept this version focused |

## Evidence to show

Open docs/CONCURRENCY.md, tests/concurrency_tests.cpp, tools/check.sh and evidence/thales/. Demonstrate the original 1,000-item test, the larger sequence audit, the queued-drain test, the failed-stdout regression and the sanitizer report. Explain which checks actually ran locally and that the new GitHub Actions workflow has not been run on GitHub.

## Three concise resume bullets

- Built a modular C++17 simulator for **3 sensor channels** by integrating OOP, STL containers, input validation and deterministic replay.
- Verified FIFO transfer of **10,000 samples with zero missing, duplicate or out-of-order IDs** using synchronized producer-consumer threads, a mutex and a condition variable.
- Strengthened reliability with **33/33 Linux CTest checks in Debug, Release, UBSan and TSan builds**, automating checks with Bash and fixing console-write error handling.

These refer to the specific executed workloads and configurations in THALES_VERIFICATION.md, not a universal delivery guarantee. The larger sequence is an in-memory pipeline correctness check, not a timed full-application benchmark. ASan leak detection, Windows and hosted CI are deliberately not claimed in these bullets.
