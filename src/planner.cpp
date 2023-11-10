#include "planner.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace plannerai {
namespace {

std::vector<std::string> split(const std::string& value, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string part;
    while (std::getline(stream, part, delimiter)) {
        if (!part.empty()) parts.push_back(part);
    }
    return parts;
}

std::string escape_json(const std::string& value) {
    std::string escaped;
    for (char ch : value) {
        if (ch == '"' || ch == '\\') escaped.push_back('\\');
        escaped.push_back(ch);
    }
    return escaped;
}

bool dependency_cycle(const std::vector<Task>& tasks) {
    std::unordered_map<std::string, std::vector<std::string>> edges;
    std::unordered_map<std::string, int> indegree;
    for (const auto& task : tasks) indegree[task.id] = 0;
    for (const auto& task : tasks) {
        for (const auto& dependency : task.dependencies) {
            edges[dependency].push_back(task.id);
            ++indegree[task.id];
        }
    }
    std::queue<std::string> ready;
    for (const auto& [id, degree] : indegree) if (degree == 0) ready.push(id);
    std::size_t visited = 0;
    while (!ready.empty()) {
        auto id = ready.front();
        ready.pop();
        ++visited;
        for (const auto& child : edges[id]) if (--indegree[child] == 0) ready.push(child);
    }
    return visited != tasks.size();
}

}  // namespace

void Scheduler::validate(const std::vector<Task>& tasks, const std::vector<Availability>& availability) {
    std::unordered_set<std::string> ids;
    for (const auto& task : tasks) {
        if (task.id.empty() || !ids.insert(task.id).second) throw std::invalid_argument("task ids must be unique");
        if (task.duration_minutes <= 0 || task.minimum_chunk <= 0) throw std::invalid_argument("durations must be positive");
        if (task.priority < 1 || task.priority > 5) throw std::invalid_argument("priority must be 1..5");
        if (task.cognitive_load < 0 || task.cognitive_load > 1) throw std::invalid_argument("cognitive load must be 0..1");
    }
    for (const auto& task : tasks) {
        for (const auto& dependency : task.dependencies) {
            if (!ids.contains(dependency)) throw std::invalid_argument("unknown dependency: " + dependency);
        }
    }
    if (dependency_cycle(tasks)) throw std::invalid_argument("task dependency cycle detected");
    for (const auto& slot : availability) {
        if (slot.end_minute <= slot.start_minute) throw std::invalid_argument("availability must have positive length");
        if (slot.energy < 0 || slot.energy > 1) throw std::invalid_argument("energy must be 0..1");
    }
}

double Scheduler::score(const Task& task, const Availability& slot, int cursor, int remaining) {
    const auto minutes_until_due = std::max(1, task.deadline_minute - cursor);
    const double urgency = std::min(3.0, 720.0 / minutes_until_due);
    const double priority = task.priority * 1.8;
    const double energy_match = 1.0 - std::abs(task.cognitive_load - slot.energy);
    const double completion_bonus = remaining <= (slot.end_minute - cursor) ? 1.4 : 0.0;
    const double overdue_penalty = cursor >= task.deadline_minute ? 4.0 : 0.0;
    return priority + urgency + 2.5 * energy_match + completion_bonus - overdue_penalty;
}

Plan Scheduler::build(std::vector<Task> tasks, std::vector<Availability> availability) const {
    validate(tasks, availability);
    std::sort(availability.begin(), availability.end(), [](const auto& a, const auto& b) {
        return a.start_minute < b.start_minute;
    });
    std::unordered_map<std::string, Task> by_id;
    std::unordered_map<std::string, int> remaining;
    for (const auto& task : tasks) {
        by_id[task.id] = task;
        remaining[task.id] = task.duration_minutes;
    }
    std::unordered_set<std::string> completed;
    Plan plan;

    for (const auto& slot : availability) {
        int cursor = slot.start_minute;
        while (cursor < slot.end_minute) {
            const Task* best = nullptr;
            double best_score = -1e9;
            for (const auto& task : tasks) {
                if (remaining[task.id] <= 0) continue;
                const bool ready = std::all_of(task.dependencies.begin(), task.dependencies.end(), [&](const auto& dep) {
                    return completed.contains(dep);
                });
                if (!ready) continue;
                const int free = slot.end_minute - cursor;
                if (free < std::min(task.minimum_chunk, remaining[task.id])) continue;
                const double candidate = score(task, slot, cursor, remaining[task.id]);
                if (candidate > best_score || (candidate == best_score && task.deadline_minute < best->deadline_minute)) {
                    best = &task;
                    best_score = candidate;
                }
            }
            if (!best) break;
            const int chunk = std::min(remaining[best->id], slot.end_minute - cursor);
            plan.blocks.push_back({best->id, best->title, cursor, cursor + chunk, best_score});
            plan.objective += best_score * chunk / 60.0;
            remaining[best->id] -= chunk;
            cursor += chunk;
            if (remaining[best->id] == 0) completed.insert(best->id);
        }
    }
    for (const auto& [id, minutes] : remaining) if (minutes > 0) plan.unscheduled_minutes[id] = minutes;
    return plan;
}

std::vector<Task> read_tasks_csv(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open tasks file: " + path);
    std::vector<Task> tasks;
    std::string line;
    std::getline(file, line);
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto columns = split(line, ',');
        if (columns.size() < 7) throw std::runtime_error("task row needs 7 columns");
        Task task{columns[0], columns[1], std::stoi(columns[2]), std::stoi(columns[3]),
                  std::stoi(columns[4]), std::stoi(columns[5]), std::stod(columns[6]), {}};
        if (columns.size() > 7 && columns[7] != "-") task.dependencies = split(columns[7], '|');
        tasks.push_back(std::move(task));
    }
    return tasks;
}

std::vector<Availability> read_availability_csv(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open availability file: " + path);
    std::vector<Availability> values;
    std::string line;
    std::getline(file, line);
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto columns = split(line, ',');
        values.push_back({std::stoi(columns.at(0)), std::stoi(columns.at(1)), std::stod(columns.at(2))});
    }
    return values;
}

std::string minute_to_clock(int minute) {
    const int day = minute / 1440;
    const int within_day = minute % 1440;
    std::ostringstream out;
    out << "D" << day + 1 << " " << std::setw(2) << std::setfill('0') << within_day / 60
        << ":" << std::setw(2) << within_day % 60;
    return out.str();
}

std::string plan_to_json(const Plan& plan) {
    std::ostringstream out;
    out << "{\n  \"objective\": " << std::fixed << std::setprecision(2) << plan.objective << ",\n  \"blocks\": [\n";
    for (std::size_t i = 0; i < plan.blocks.size(); ++i) {
        const auto& block = plan.blocks[i];
        out << "    {\"task_id\": \"" << escape_json(block.task_id) << "\", \"title\": \""
            << escape_json(block.title) << "\", \"start\": \"" << minute_to_clock(block.start_minute)
            << "\", \"end\": \"" << minute_to_clock(block.end_minute) << "\", \"score\": "
            << block.score << "}" << (i + 1 == plan.blocks.size() ? "" : ",") << "\n";
    }
    out << "  ],\n  \"unscheduled_minutes\": {";
    std::size_t index = 0;
    for (const auto& [id, minutes] : plan.unscheduled_minutes) {
        out << (index++ ? ", " : "") << "\"" << escape_json(id) << "\": " << minutes;
    }
    out << "}\n}\n";
    return out.str();
}

}  // namespace plannerai

