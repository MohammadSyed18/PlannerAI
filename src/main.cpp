#include "planner.hpp"
#include "analysis.hpp"
#include "calendar.hpp"

#include <exception>
#include <stdexcept>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: planner <tasks.csv> <availability.csv> [--audit] [--busy busy.csv]\n";
        return 2;
    }
    try {
        plannerai::Scheduler scheduler;
        const auto tasks = plannerai::read_tasks_csv(argv[1]);
        auto availability = plannerai::read_availability_csv(argv[2]);
        bool audit=false;
        for(int index=3;index<argc;++index) {
            const std::string argument=argv[index];
            if(argument=="--audit")audit=true;
            else if(argument=="--busy" && index+1<argc) {
                std::vector<plannerai::BusyWindow> busy;
                for(const auto& slot:plannerai::read_availability_csv(argv[++index]))busy.push_back({slot.start_minute,slot.end_minute});
                const auto calendar=plannerai::compile_calendar(availability,busy);
                availability=calendar.free;
                std::cerr<<"Calendar: "<<calendar.blocked_minutes<<" busy minutes removed\n";
            } else throw std::invalid_argument("unknown or incomplete option");
        }
        const auto plan = scheduler.build(tasks, availability);
        if (audit) {
            std::cerr << plannerai::analysis_to_text(plannerai::analyze_plan(tasks, availability, plan));
        }
        std::cout << plannerai::plan_to_json(plan);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "planner: " << error.what() << '\n';
        return 1;
    }
}

