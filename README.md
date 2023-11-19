# plannerai

tasks + deadlines + dependencies + available time -> schedule

C++. can split tasks across blocks.
scores urgency, priority, energy requirements.

the name is a bit broad — this version uses a scheduling heuristic, not an AI model.

```sh
cmake -S . -B build
cmake --build build
./build/planner data/tasks.csv data/availability.csv
ctest --test-dir build --output-on-failure
```

CSV input, dependency checks, scheduling, and JSON output are implemented. calendar import and interactive replanning are still in progress.

## Implementation details

Availability windows are checked for overlap to prevent duplicate time allocation. CSV input supports quoted titles and empty fields, and JSON output escapes control characters.

## Auditing a schedule

[analysis.cpp](src/analysis.cpp) independently checks a plan against tasks and availability. It reports capacity utilization, per-task scheduled/remaining minutes, actual minutes after deadlines, fragmentation, and incomplete prerequisites. It detects overlapping blocks, work outside availability, invalid task IDs, over-allocation, short chunks, dependency-order violations, and inconsistent remainder counts.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/planner data/tasks.csv data/availability.csv --audit
```

JSON stays on standard output and the audit goes to standard error. Late work is reported rather than forbidden: deadlines are part of the scheduler's heuristic score, not a hard feasibility constraint. The auditor checks structural consistency independently of that heuristic and can also inspect externally assembled `Plan` objects through the C++ API.

## Planning around fixed commitments

```sh
./build/planner data/tasks.csv data/availability.csv --busy data/busy.example.csv --audit
```

`calendar.hpp` / `calendar.cpp` compile availability after subtracting busy periods. Busy CSV uses the availability schema (`start_minute,end_minute,energy`); its energy column is ignored. Busy windows may overlap or touch and are merged before subtraction, while original availability must remain non-overlapping. Intervals are half-open, so a task can end exactly when a commitment begins. The CLI prints removed minutes and schedules against the resulting free windows.

The library additionally accepts pre/post buffers and a minimum remaining window. It separately reports blocked minutes and short fragments discarded by that minimum; retained windows keep their original energy values. Intermediate arithmetic uses wider integers and rejects unrepresentable totals. Tests check overlap merging, complete blocking, extreme endpoints, energy preservation, and integration with the independent plan audit.
