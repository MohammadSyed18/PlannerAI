#pragma once
#include "planner.hpp"
#include <vector>
namespace plannerai {
struct BusyWindow { int start_minute{}; int end_minute{}; };
struct CalendarResult {
    std::vector<Availability> free;
    int original_minutes{};
    int blocked_minutes{};
    int discarded_minutes{};
};
// Busy periods are half-open; overlaps and touching periods merge before subtraction.
CalendarResult compile_calendar(const std::vector<Availability>& availability,
                                const std::vector<BusyWindow>& busy,
                                int buffer_before=0, int buffer_after=0,
                                int minimum_window=1);
}
