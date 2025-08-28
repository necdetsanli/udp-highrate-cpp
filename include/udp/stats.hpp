
#pragma once
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sstream>

namespace udp {

struct ClientKey {
    uint32_t addr;
    uint16_t port;
    bool operator==(const ClientKey& o) const { return addr==o.addr && port==o.port; }
};

struct ClientKeyHash {
    size_t operator()(const ClientKey& k) const {
        return (static_cast<size_t>(k.addr) << 16) ^ k.port;
    }
};

class Stats {
public:
    void inc_sent(uint64_t n) { sent_.fetch_add(n, std::memory_order_relaxed); }
    void inc_recv(uint64_t n) { recv_.fetch_add(n, std::memory_order_relaxed); }
    void add_rx_bytes(uint64_t n) { rx_bytes_.fetch_add(n, std::memory_order_relaxed); }
    void add_tx_bytes(uint64_t n) { tx_bytes_.fetch_add(n, std::memory_order_relaxed); }
    void note_client(uint32_t addr, uint16_t port) {
        std::lock_guard<std::mutex> lg(mu_);
        clients_[ClientKey{addr,port}]++;
    }
    size_t unique_clients() const {
        std::lock_guard<std::mutex> lg(mu_);
        return clients_.size();
    }
    uint64_t sent() const { return sent_.load(); }
    uint64_t recv() const { return recv_.load(); }
    uint64_t rx_bytes() const { return rx_bytes_.load(); }
    uint64_t tx_bytes() const { return tx_bytes_.load(); }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "recv=" << recv() << " sent=" << sent()
            << " unique_clients=" << unique_clients()
            << " rx_bytes=" << rx_bytes() << " tx_bytes=" << tx_bytes();
        return oss.str();
    }
private:
    std::atomic<uint64_t> sent_{0}, recv_{0}, rx_bytes_{0}, tx_bytes_{0};
    mutable std::mutex mu_;
    std::unordered_map<ClientKey, uint64_t, ClientKeyHash> clients_;
};

} // namespace udp
