
#include "udp/server.hpp"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>

namespace udp {

UdpServer::UdpServer(std::unique_ptr<ISocket> sock, ServerConfig cfg)
: sock_(std::move(sock)), cfg_(cfg) {
    sock_->bind(cfg_.port, cfg_.reuseport);
    sock_->set_rcvbuf(1<<20);
    sock_->set_sndbuf(1<<20);
    if (cfg_.metrics_port) {
        metrics_ = std::make_unique<MetricsHttpServer>(stats_, cfg_.metrics_port);
    }
}

UdpServer::~UdpServer() {
    stop();
}

void UdpServer::start() {
    if (metrics_) metrics_->start();
    running_ = true;
    th_ = std::thread(&UdpServer::run_loop, this);
}

void UdpServer::stop() {
    if (th_.joinable()) {
        running_ = false;
        th_.join();
    }
    if (metrics_) metrics_->stop();
}

void UdpServer::run_loop() {
    std::vector<std::vector<uint8_t>> bufs(cfg_.batch, std::vector<uint8_t>(2048));
    uint64_t last_recv_total = 0;
    auto last_ts = std::chrono::steady_clock::now();
    while (running_) {
        ssize_t r = sock_->recv_batch(bufs);
        if (r < 0) continue;
        if (r > 0) {
            for (ssize_t i=0;i<r;i++) {
                // Track client from a fake header: in real world we would read src addr from recvmmsg
                // Here we approximate by requiring clients to include magic header at start
                if (bufs[i].size() >= sizeof(PacketHeader)) {
                    PacketHeader* hdr = reinterpret_cast<PacketHeader*>(bufs[i].data());
                    if (hdr->magic == kMagic) {
                        // Cannot access peer addr without msghdr name here (already set in socket), so skip addr track
                    }
                }
                stats_.inc_recv(1);
                stats_.add_rx_bytes(bufs[i].size());
            }
            if (cfg_.echo) {
                // Echo back
                std::vector<std::vector<uint8_t>> out;
                out.reserve(r);
                for (ssize_t i=0;i<r;i++) out.push_back(bufs[i]);
                ssize_t s = sock_->send_batch(out, nullptr);
                if (s > 0) {
                    stats_.inc_sent(s);
                    size_t total_bytes = 0; for (auto& b: out) total_bytes += b.size();
                    stats_.add_tx_bytes(total_bytes);
                }
            }
        }
        auto now = std::chrono::steady_clock::now();
        if (now - last_ts >= std::chrono::seconds(1)) {
            uint64_t recv_total = stats_.recv();
            uint64_t delta = recv_total - last_recv_total;
            last_rate_pps_ = static_cast<double>(delta);
            if (cfg_.verbose) {
                std::cout << "[server] " << stats_.to_string()
                          << " rate=" << human_rate(last_rate_pps_) << "\n";
            }
            last_recv_total = recv_total;
            last_ts = now;
        }
    }
}

} // namespace udp
