#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace plannerai {

using Minutes = std::chrono::minutes;

struct Task {
    std::string id;
    std::string title;
    int duration_minutes{};
    int priority{};
    int deadline_minute{};
    int minimum_chunk{25};
    double cognitive_load{0.5};
    std::vector<std::string> dependencies;
};

struct Availability {
    int start_minute{};
    int end_minute{};
    double energy{0.5};
};

struct ScheduledBlock {
    std::string task_id;
    std::string title;
    int start_minute{};
    int end_minute{};
    double score{};
};

struct Plan {
    std::vector<ScheduledBlock> blocks;
    std::unordered_map<std::string, int> unscheduled_minutes;
    std::unordered_map<std::string, std::string> unscheduled_reasons;
    double objective{};
};

class Scheduler {
public:
    Plan build(std::vector<Task> tasks, std::vector<Availability> availability) const;
    static void validate(const std::vector<Task>& tasks, const std::vector<Availability>& availability);

private:
    static double score(const Task& task, const Availability& slot, int cursor, int remaining);
};

std::vector<Task> read_tasks_csv(const std::string& path);
std::vector<Availability> read_availability_csv(const std::string& path);
std::string plan_to_json(const Plan& plan);
std::string minute_to_clock(int minute);

}  // namespace plannerai
