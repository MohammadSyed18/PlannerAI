#include "calendar.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
namespace plannerai {
CalendarResult compile_calendar(const std::vector<Availability>& availability,
                                const std::vector<BusyWindow>& busy,
                                int buffer_before, int buffer_after, int minimum_window) {
    Scheduler::validate({},availability);
    if(buffer_before<0 || buffer_after<0 || minimum_window<1)
        throw std::invalid_argument("buffers must be nonnegative and minimum window positive");
    using Wide = long long;
    std::vector<std::pair<Wide,Wide>> intervals;
    for(const auto& item:busy) {
        if(item.end_minute<=item.start_minute) throw std::invalid_argument("busy window must have positive length");
        intervals.emplace_back(static_cast<Wide>(item.start_minute)-buffer_before,
                               static_cast<Wide>(item.end_minute)+buffer_after);
    }
    std::sort(intervals.begin(),intervals.end());
    std::vector<std::pair<Wide,Wide>> merged;
    for(const auto& interval:intervals) {
        if(!merged.empty() && interval.first<=merged.back().second)
            merged.back().second=std::max(merged.back().second,interval.second);
        else merged.push_back(interval);
    }
    CalendarResult result;
    Wide original=0,blocked=0,discarded=0;
    auto sorted=availability;
    std::sort(sorted.begin(),sorted.end(),[](const auto& a,const auto& b){return a.start_minute<b.start_minute;});
    for(const auto& slot:sorted) {
        original+=static_cast<Wide>(slot.end_minute)-slot.start_minute;
        Wide cursor=slot.start_minute;
        const Wide end=slot.end_minute;
        auto append=[&](Wide first,Wide last) {
            if(last<=first)return;
            if(last-first<minimum_window)discarded+=last-first;
            else result.free.push_back({static_cast<int>(first),static_cast<int>(last),slot.energy});
        };
        for(const auto& interval:merged) {
            if(interval.second<=cursor)continue;
            if(interval.first>=end)break;
            const Wide first=std::max(cursor,interval.first);
            const Wide last=std::min(end,interval.second);
            append(cursor,first);
            blocked+=last-first;
            cursor=last;
            if(cursor==end)break;
        }
        append(cursor,end);
    }
    if(original>std::numeric_limits<int>::max())throw std::overflow_error("calendar total exceeds supported minutes");
    result.original_minutes=static_cast<int>(original);
    result.blocked_minutes=static_cast<int>(blocked);
    result.discarded_minutes=static_cast<int>(discarded);
    return result;
}
}
