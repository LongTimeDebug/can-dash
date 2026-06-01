// tests/test_time_util.cpp
// Unit tests for src/layer2/time_util.{h,cpp} — monotonic clock + ISO-8601 formatter.
//
// Why this matters: vehicle logic uses now_monotonic_ms() to decide when
// alarms time out, indicators blink, and the seat-belt chime repeats.
// If this drifts with NTP/wall-clock changes, drivers see the chime fire
// twice or skip. So we lock the monotonic property in a test.

#include "layer2/time_util.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <thread>
#include <chrono>
#include <regex>

namespace {

void test_monotonic_ms_advances() {
    const uint64_t a = candash::now_monotonic_ms();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const uint64_t b = candash::now_monotonic_ms();
    // 50ms sleep must produce a non-zero, reasonably accurate delta.
    assert(b > a);
    const uint64_t delta = b - a;
    if (delta < 40 || delta > 500) {
        std::fprintf(stderr, "monotonic delta out of range: %llu ms\n",
                     static_cast<unsigned long long>(delta));
        std::abort();
    }
    std::printf("  monotonic delta over 50ms sleep: %llu ms (OK)\n",
                static_cast<unsigned long long>(delta));
}

void test_monotonic_us_resolution() {
    const uint64_t a = candash::now_monotonic_us();
    std::this_thread::sleep_for(std::chrono::microseconds(200));
    const uint64_t b = candash::now_monotonic_us();
    assert(b > a);
    if ((b - a) < 100) {
        std::fprintf(stderr, "us resolution too coarse: %llu\n",
                     static_cast<unsigned long long>(b - a));
        std::abort();
    }
    std::printf("  us delta over 200us sleep: %llu us (OK)\n",
                static_cast<unsigned long long>(b - a));
}

void test_format_wall_clock_iso8601() {
    char buf[64] = {0};
    candash::format_wall_clock(buf, sizeof(buf));
    // Expect: YYYY-MM-DDTHH:MM:SS.mmmZ  (24 chars + NUL)
    static const std::regex iso(
        R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}Z$)");
    if (!std::regex_match(buf, iso)) {
        std::fprintf(stderr, "wall_clock not ISO-8601: '%s'\n", buf);
        std::abort();
    }
    std::printf("  wall_clock format: '%s' (OK)\n", buf);
}

void test_format_wall_clock_handles_small_buffer() {
    // buf too small must not overrun — implementation should truncate.
    char tiny[8] = {'X','X','X','X','X','X','X','X'};
    candash::format_wall_clock(tiny, sizeof(tiny));
    assert(tiny[sizeof(tiny) - 1] == '\0' || std::strlen(tiny) < sizeof(tiny));
    std::printf("  wall_clock with 8-byte buf: '%s' (no overrun, OK)\n", tiny);
}

}  // namespace

int main() {
    std::printf("test_time_util\n");
    test_monotonic_ms_advances();
    test_monotonic_us_resolution();
    test_format_wall_clock_iso8601();
    test_format_wall_clock_handles_small_buffer();
    std::printf("ALL PASS\n");
    return 0;
}
