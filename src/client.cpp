#include "udp/client.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <cstring>
#include <sys/time.h>

namespace udp {

UdpClient::UdpClient(std::unique_ptr<ISocket> sock, ClientConfig cfg)
: sock_(std::move(sock)), cfg_(cfg) {
    sock_->connect(cfg_.server_ip, cfg_.port);
    sock_->set_sndbuf(1<<20);
}

UdpClient::~UdpClient() { stop(); }

void UdpClient::start() {
    running_ = true;
    th_ = std::thread(&UdpClient::run_loop, this);
}

void UdpClient::stop() {
    if (th_.joinable()) {
        running_ = false;
        th_.join();
    }
}

void UdpClient::join() {
    // Wait until the worker thread exits naturally (e.g., after --seconds duration)
    if (th_.joinable()) {
        th_.join();
    }
}

void UdpClient::run_loop() {
    const uint64_t interval_ns = 1'000'000'000ull / (cfg_.pps ? cfg_.pps : 1);
    uint64_t next_ts = now_ns();
    auto start = std::chrono::steady_clock::now();
    auto end = start + std::chrono::seconds(cfg_.seconds);

    std::vector<std::vector<uint8_t>> batch;
    batch.reserve(cfg_.batch);

    while (running_ && std::chrono::steady_clock::now() < end) {
        // Prepare a batch of packets with header
        batch.clear();
        for (int i=0; i<cfg_.batch; ++i) {
            std::vector<uint8_t> pkt(std::max(cfg_.payload, (int)sizeof(PacketHeader)), 0);
            PacketHeader* hdr = reinterpret_cast<PacketHeader*>(pkt.data());
            hdr->seq = ++seq_;
            hdr->send_ts_ns = now_ns();
            hdr->magic = kMagic;
            batch.push_back(std::move(pkt));
        }
        auto s = sock_->send_batch(batch, nullptr);
        if (s > 0) {
            stats_.inc_sent(s);
            size_t total_bytes = 0; for (auto& b: batch) total_bytes += b.size();
            stats_.add_tx_bytes(total_bytes);
        }

        // Pace to target pps
        next_ts += interval_ns * cfg_.batch;
        uint64_t now = now_ns();
        if (next_ts > now) {
            uint64_t sleep_ns = next_ts - now;
            timespec ts{ (time_t)(sleep_ns/1'000'000'000ull), (long)(sleep_ns%1'000'000'000ull) };
            clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, nullptr);
        }

        static uint64_t last_print_ns = now_ns();
        if (cfg_.verbose && now - last_print_ns > 1'000'000'000ull) {
            std::cout << "[client " << cfg_.id << "] sent=" << stats_.sent()
                      << " tx_bytes=" << stats_.tx_bytes() << "\n";
            last_print_ns = now;
        }
    }
}

} // namespace udp
