#include "calendar.hpp"
#include "analysis.hpp"
#include <cassert>
#include <limits>
#include <stdexcept>
int main() {
    using namespace plannerai;
    auto result=compile_calendar({{0,120,.8}},{{20,40},{30,60},{60,80}});
    assert(result.original_minutes==120 && result.blocked_minutes==60);
    assert(result.free.size()==2 && result.free[0].end_minute==20 && result.free[1].start_minute==80);
    assert(result.free[1].energy==.8);
    result=compile_calendar({{0,120,.8}},{{20,40}},10,10,15);
    assert(result.blocked_minutes==40 && result.discarded_minutes==10 && result.free.size()==1);
    result=compile_calendar({{0,120,.5}},{{-100,1000}});
    assert(result.free.empty() && result.blocked_minutes==120);
    result=compile_calendar({{0,120,.5}},{{std::numeric_limits<int>::min(),0}},100,5);
    assert(result.free[0].start_minute==5);
    bool rejected=false;
    try {compile_calendar({{0,60,.5},{30,80,.5}},{});} catch(const std::invalid_argument&) {rejected=true;}
    assert(rejected);
    result=compile_calendar({{0,120,.5}},{{30,60}});
    const std::vector<Task> tasks={{"a","Work",60,3,120,15,.5,{}}};
    const auto plan=Scheduler().build(tasks,result.free);
    assert(analyze_plan(tasks,result.free,plan).issues.empty());
    for(const auto& block:plan.blocks)assert(block.end_minute<=30 || block.start_minute>=60);
}
