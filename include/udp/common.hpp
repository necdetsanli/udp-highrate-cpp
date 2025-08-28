
#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <chrono>

namespace udp {

#pragma pack(push, 1)
struct PacketHeader {
    uint64_t seq;         // sequence number
    uint64_t send_ts_ns;  // sender timestamp (ns)
    uint32_t magic;       // magic for sanity
};
#pragma pack(pop)

static constexpr uint32_t kMagic = 0xC0DEF00D;

inline uint64_t now_ns() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

inline std::string human_rate(double v) {
    char buf[64];
    if (v > 1e6) snprintf(buf, sizeof(buf), "%.2f Mpps", v / 1e6);
    else if (v > 1e3) snprintf(buf, sizeof(buf), "%.2f kpps", v / 1e3);
    else snprintf(buf, sizeof(buf), "%.2f pps", v);
    return std::string(buf);
}

} // namespace udp
