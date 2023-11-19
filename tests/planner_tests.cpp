#include "planner.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <filesystem>

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

int main() {
    dependency_order_is_preserved();
    impossible_capacity_is_reported();
    cycles_are_rejected();
    bool overlap_rejected=false;
    try { Scheduler().build({},{{0,60,.5},{30,90,.5}}); } catch(const std::invalid_argument&) { overlap_rejected=true; }
    assert(overlap_rejected);
    const auto path=std::filesystem::temp_directory_path()/"planner-quoted-csv-test.csv";
    {std::ofstream file(path);file<<"id,title,duration,priority,deadline,chunk,load,deps\n"<<"a,\"Read, think and write\",30,3,500,10,0.5,-\n";}
    const auto tasks=read_tasks_csv(path.string());std::filesystem::remove(path);
    assert(tasks[0].title=="Read, think and write");
    Plan escaped;escaped.blocks.push_back({"a","First\nsecond\tline",0,10,1});
    assert(plan_to_json(escaped).find("\\u000a")!=std::string::npos);
    std::cout << "6 planner tests passed\n";
}

