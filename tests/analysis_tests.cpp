#include "analysis.hpp"
#include <cassert>
#include <algorithm>
#include <iostream>
using namespace plannerai;
bool has(const PlanAnalysis& report,const std::string& code) {
    return std::any_of(report.issues.begin(),report.issues.end(),[&](const auto& issue){return issue.code==code;});
}
int main() {
    const std::vector<Task> tasks{{"a","Research",60,4,530,20,.5,{}},{"b","Draft",30,3,600,15,.5,{"a"}}};
    const std::vector<Availability> slots{{480,600,.5}};
    const auto generated=Scheduler().build(tasks,slots);
    const auto valid=analyze_plan(tasks,slots,generated);
    assert(valid.issues.empty());
    assert(valid.scheduled_minutes==90);
    assert(valid.tasks[0].late_minutes==10);
    assert(valid.utilization==.75);
    Plan broken;
    broken.blocks={{"a","Research",500,560,1},{"b","Draft",510,540,1},{"unknown","",480,500,1}};
    auto report=analyze_plan(tasks,slots,broken);
    assert(has(report,"overlap"));
    assert(has(report,"dependency_order"));
    assert(has(report,"unknown_task"));
    Plan outside;
    outside.blocks={{"a","Research",470,490,1}};
    report=analyze_plan(tasks,slots,outside);
    assert(has(report,"outside_availability"));
    assert(has(report,"remaining_mismatch"));
    assert(report.tasks[1].blocking_dependencies.size()==1);
    assert(analysis_to_text(valid).find("90/120")!=std::string::npos);
    std::cout << "plan analysis checks passed\n";
}
