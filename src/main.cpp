#include "planner.hpp"
#include "analysis.hpp"

#include <exception>
#include <stdexcept>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) {
        std::cerr << "usage: planner <tasks.csv> <availability.csv> [--audit]\n";
        return 2;
    }
    try {
        plannerai::Scheduler scheduler;
        const auto tasks = plannerai::read_tasks_csv(argv[1]);
        const auto availability = plannerai::read_availability_csv(argv[2]);
        const auto plan = scheduler.build(tasks, availability);
        if (argc == 4) {
            if (std::string(argv[3]) != "--audit") throw std::invalid_argument("unknown option");
            std::cerr << plannerai::analysis_to_text(plannerai::analyze_plan(tasks, availability, plan));
        }
        std::cout << plannerai::plan_to_json(plan);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "planner: " << error.what() << '\n';
        return 1;
    }
}

