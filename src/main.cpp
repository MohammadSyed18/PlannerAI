#include "planner.hpp"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: planner <tasks.csv> <availability.csv>\n";
        return 2;
    }
    try {
        plannerai::Scheduler scheduler;
        const auto plan = scheduler.build(plannerai::read_tasks_csv(argv[1]), plannerai::read_availability_csv(argv[2]));
        std::cout << plannerai::plan_to_json(plan);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "planner: " << error.what() << '\n';
        return 1;
    }
}

