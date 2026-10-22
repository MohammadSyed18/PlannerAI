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
