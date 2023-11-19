#include "analysis.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace plannerai {
PlanAnalysis analyze_plan(const std::vector<Task>& tasks,
                          const std::vector<Availability>& availability,
                          const Plan& plan) {
    Scheduler::validate(tasks, availability);
    PlanAnalysis result;
    std::map<std::string,const Task*> by_id;
    std::map<std::string,TaskAnalysis> details;
    std::map<std::string,int> completion;
    std::map<std::string,int> first_start;
    for (const auto& task : tasks) {
        by_id[task.id] = &task;
        TaskAnalysis item;
        item.id = task.id;
        item.remaining_minutes = task.duration_minutes;
        details[task.id] = item;
    }
    for (const auto& window : availability) result.available_minutes += window.end_minute - window.start_minute;
    auto blocks = plan.blocks;
    std::sort(blocks.begin(),blocks.end(),[](const auto& a,const auto& b) {
        return a.start_minute < b.start_minute || (a.start_minute == b.start_minute && a.end_minute < b.end_minute);
    });
    int previous_end = -1;
    for (const auto& block : blocks) {
        if (!by_id.contains(block.task_id)) {
            result.issues.push_back({"unknown_task",block.task_id,"block references a missing task"});
            continue;
        }
        if (block.end_minute <= block.start_minute) {
            result.issues.push_back({"invalid_interval",block.task_id,"block has no positive duration"});
            continue;
        }
        if (block.start_minute < previous_end) result.issues.push_back({"overlap",block.task_id,"scheduled blocks overlap"});
        previous_end = std::max(previous_end,block.end_minute);
        const bool contained = std::any_of(availability.begin(),availability.end(),[&](const auto& slot) {
            return block.start_minute >= slot.start_minute && block.end_minute <= slot.end_minute;
        });
        if (!contained) result.issues.push_back({"outside_availability",block.task_id,"block extends outside a single available window"});
        auto& item = details.at(block.task_id);
        const auto& task = *by_id.at(block.task_id);
        const int duration = block.end_minute - block.start_minute;
        if (!first_start.contains(task.id)) first_start[task.id] = block.start_minute;
        item.scheduled_minutes += duration;
        item.remaining_minutes = std::max(0,task.duration_minutes - item.scheduled_minutes);
        item.late_minutes += std::max(0,block.end_minute - std::max(block.start_minute,task.deadline_minute));
        item.fragments++;
        item.earliest_finish = std::max(item.earliest_finish,block.end_minute);
        result.scheduled_minutes += duration;
        if (item.scheduled_minutes > task.duration_minutes) result.issues.push_back({"overallocated",task.id,"scheduled duration exceeds required work"});
        if (duration < std::min(task.minimum_chunk,task.duration_minutes - (item.scheduled_minutes - duration))) {
            result.issues.push_back({"short_chunk",task.id,"fragment is below the minimum nonfinal chunk"});
        }
        if (item.remaining_minutes == 0) completion[task.id] = block.end_minute;
    }
    for (const auto& task : tasks) {
        auto& item = details.at(task.id);
        for (const auto& dependency : task.dependencies) {
            if (!completion.contains(dependency)) item.blocking_dependencies.push_back(dependency);
            if (first_start.contains(task.id) && (!completion.contains(dependency) || completion[dependency] > first_start[task.id])) {
                result.issues.push_back({"dependency_order",task.id,"work begins before dependency " + dependency + " finishes"});
            }
        }
        const auto remaining = plan.unscheduled_minutes.find(task.id);
        const int reported = remaining == plan.unscheduled_minutes.end() ? 0 : remaining->second;
        if (reported != item.remaining_minutes) result.issues.push_back({"remaining_mismatch",task.id,"reported remaining minutes disagree with blocks"});
        result.tasks.push_back(item);
    }
    for (const auto& [id, minutes] : plan.unscheduled_minutes) {
        if (!by_id.contains(id) || minutes < 0) result.issues.push_back({"invalid_remaining",id,"unknown task or negative remainder"});
    }
    result.utilization = result.available_minutes ? static_cast<double>(result.scheduled_minutes) / result.available_minutes : 0;
    return result;
}
std::string analysis_to_text(const PlanAnalysis& report) {
    std::ostringstream out;
    out << "Capacity: " << report.scheduled_minutes << '/' << report.available_minutes
        << " min (" << report.utilization * 100 << "%)\n";
    for (const auto& task : report.tasks) {
        out << task.id << ": " << task.scheduled_minutes << " scheduled, " << task.remaining_minutes
            << " remaining, " << task.late_minutes << " late, " << task.fragments << " fragments\n";
        for (const auto& dependency : task.blocking_dependencies) out << "  blocked by " << dependency << '\n';
    }
    for (const auto& issue : report.issues) out << "Issue " << issue.code << " [" << issue.task_id << "]: " << issue.detail << '\n';
    return out.str();
}
} // namespace plannerai
