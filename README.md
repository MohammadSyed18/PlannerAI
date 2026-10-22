# PlannerAI

A C++ scheduler for tasks that have deadlines, prerequisites, and different energy requirements. It fits work into available blocks and can split a long task across them.

The name aside, the scheduling here is heuristic: it scores work by urgency, priority, and energy rather than calling an AI model.

```sh
cmake -S . -B build
cmake --build build
./build/planner data/tasks.csv data/availability.csv
ctest --test-dir build --output-on-failure
```

CSV input, dependency checks, scheduling, and JSON output are implemented. Calendar import and interactive replanning are still in progress.
