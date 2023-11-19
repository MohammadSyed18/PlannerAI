#pragma once
#include "planner.hpp"
#include <string>
#include <vector>

namespace plannerai {
struct PlanIssue {
    std::string code;
    std::string task_id;
    std::string detail;
};
struct TaskAnalysis {
    std::string id;
    int scheduled_minutes{};
    int remaining_minutes{};
    int late_minutes{};
    int fragments{};
    int earliest_finish{};
    std::vector<std::string> blocking_dependencies;
};
struct PlanAnalysis {
    int available_minutes{};
    int scheduled_minutes{};
    double utilization{};
    std::vector<PlanIssue> issues;
    std::vector<TaskAnalysis> tasks;
};
PlanAnalysis analyze_plan(const std::vector<Task>& tasks,
                          const std::vector<Availability>& availability,
                          const Plan& plan);
std::string analysis_to_text(const PlanAnalysis& analysis);
} // namespace plannerai
