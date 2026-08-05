#include "planner.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace plannerai;

void dependency_order_is_preserved() {
    std::vector<Task> tasks{
        {"research", "Research", 60, 4, 1000, 30, .8, {}},
        {"draft", "Draft", 60, 5, 1100, 30, .7, {"research"}},
    };
    Plan plan = Scheduler().build(tasks, {{480, 650, .8}});
    assert(plan.blocks.size() == 2);
    assert(plan.blocks[0].task_id == "research");
    assert(plan.blocks[1].task_id == "draft");
}

void impossible_capacity_is_reported() {
    std::vector<Task> tasks{{"long", "Long task", 180, 3, 1000, 30, .5, {}}};
    Plan plan = Scheduler().build(tasks, {{480, 540, .5}});
    assert(plan.unscheduled_minutes.at("long") == 120);
}

void cycles_are_rejected() {
    std::vector<Task> tasks{
        {"a", "A", 30, 3, 1000, 15, .5, {"b"}},
        {"b", "B", 30, 3, 1000, 15, .5, {"a"}},
    };
    bool threw = false;
    try { Scheduler().build(tasks, {{0, 120, .5}}); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
}

void hard_deadlines_are_never_crossed() {
    std::vector<Task> tasks{{"due", "Due soon", 90, 5, 540, 30, .8, {}}};
    Plan plan = Scheduler().build(tasks, {{510, 600, .8}});
    assert(plan.blocks.size() == 1);
    assert(plan.blocks[0].end_minute == 540);
    assert(plan.unscheduled_minutes.at("due") == 60);
    assert(plan.unscheduled_reasons.at("due") == "deadline_capacity");
}

void overlapping_availability_is_rejected() {
    bool threw = false;
    try { Scheduler().build({}, {{480, 540, .5}, {530, 600, .7}}); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
}

int main() {
    dependency_order_is_preserved();
    impossible_capacity_is_reported();
    cycles_are_rejected();
    hard_deadlines_are_never_crossed();
    overlapping_availability_is_rejected();
    std::cout << "5 planner tests passed\n";
}
