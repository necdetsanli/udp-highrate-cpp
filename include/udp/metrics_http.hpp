
#pragma once
#include <thread>
#include <atomic>
#include <string>
#include "udp/stats.hpp"

namespace udp {

class MetricsHttpServer {
public:
    MetricsHttpServer(Stats& stats, uint16_t port);
    ~MetricsHttpServer();
    void start();
    void stop();
private:
    void run();
    std::string render();
    Stats& stats_;
    uint16_t port_;
    std::thread th_;
    std::atomic<bool> running_{false};
};

} // namespace udp
