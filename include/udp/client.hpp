
#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include <memory>
#include "udp/socket.hpp"
#include "udp/stats.hpp"
#include "udp/common.hpp"

namespace udp {

struct ClientConfig {
    std::string server_ip = "127.0.0.1";
    uint16_t port = 9000;
    uint64_t pps = 10000;
    int seconds = 5;
    int payload = 64;
    int batch = 64;
    int id = 0;
    bool verbose = false;
};

class UdpClient {
public:
    explicit UdpClient(std::unique_ptr<ISocket> sock, ClientConfig cfg);
    ~UdpClient();
    void start();
    void stop();
    void join();
    const Stats& stats() const { return stats_; }
private:
    void run_loop();
    std::unique_ptr<ISocket> sock_;
    ClientConfig cfg_;
    Stats stats_;
    std::thread th_;
    std::atomic<bool> running_{false};
    uint64_t seq_{0};
};

} // namespace udp
