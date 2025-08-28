
#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include <memory>
#include "udp/socket.hpp"
#include "udp/stats.hpp"
#include "udp/common.hpp"
#include "udp/metrics_http.hpp"

namespace udp {

struct ServerConfig {
    uint16_t port = 9000;
    int batch = 64;
    bool echo = false;
    bool reuseport = false;
    bool verbose = true;
    uint16_t metrics_port = 9100;
};

class UdpServer {
public:
    explicit UdpServer(std::unique_ptr<ISocket> sock, ServerConfig cfg);
    ~UdpServer();
    void start();
    void stop();
    double last_rate_pps() const { return last_rate_pps_; }
    const Stats& stats() const { return stats_; }
private:
    void run_loop();
    std::unique_ptr<ISocket> sock_;
    ServerConfig cfg_;
    Stats stats_;
    std::unique_ptr<MetricsHttpServer> metrics_;
    std::thread th_;
    std::atomic<bool> running_{false};
    double last_rate_pps_{0.0};
};

} // namespace udp
