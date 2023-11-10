# PlannerAI

PlannerAI is a C++ scheduling engine that turns tasks, deadlines, dependencies, available time blocks, and an hourly energy curve into a realistic work plan. The optimizer favors urgent/high-value work, avoids prerequisite violations, and can split long tasks across multiple blocks.

Current state: CSV input, constraint checks, scoring, scheduling, JSON output, a demo dataset, and core tests are working. Calendar import and interactive replanning are still in progress.

```bash
cmake -S . -B build && cmake --build build
./build/planner data/tasks.csv data/availability.csv
ctest --test-dir build --output-on-failure
```

