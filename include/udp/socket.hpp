
#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <atomic>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace udp {

class ISocket {
public:
    virtual ~ISocket() = default;
    virtual int fd() const = 0;
    virtual void bind(uint16_t port, bool reuseport) = 0;
    virtual void connect(const std::string& ip, uint16_t port) = 0;
    virtual ssize_t recv_batch(std::vector<std::vector<uint8_t>>& bufs) = 0;
    virtual ssize_t send_batch(const std::vector<std::vector<uint8_t>>& bufs,
                               const sockaddr_in* addr = nullptr) = 0;
    virtual void set_rcvbuf(int bytes);
    virtual void set_sndbuf(int bytes);
};

class UdpSocket : public ISocket {
public:
    explicit UdpSocket(int batch_hint = 64);
    ~UdpSocket() override;

    int fd() const override { return sockfd_; }
    void bind(uint16_t port, bool reuseport) override;
    void connect(const std::string& ip, uint16_t port) override;
    ssize_t recv_batch(std::vector<std::vector<uint8_t>>& bufs) override;
    ssize_t send_batch(const std::vector<std::vector<uint8_t>>& bufs,
                       const sockaddr_in* addr = nullptr) override;
    void set_rcvbuf(int bytes) override;
    void set_sndbuf(int bytes) override;
private:
    int sockfd_;
    int batch_hint_;
    bool connected_;
    sockaddr_in peer_{};
};

class MockSocket : public ISocket {
public:
    MockSocket() : recv_cursor_(0) {}
    int fd() const override { return -1; }
    void bind(uint16_t, bool) override {}
    void connect(const std::string&, uint16_t) override {}
    ssize_t recv_batch(std::vector<std::vector<uint8_t>>& bufs) override;
    ssize_t send_batch(const std::vector<std::vector<uint8_t>>& bufs,
                       const sockaddr_in* addr = nullptr) override;
    void set_rcvbuf(int) override {}
    void set_sndbuf(int) override {}

    // test hooks
    void preload_recv(const std::vector<uint8_t>& pkt) { rx_store_.push_back(pkt); }
    size_t sent_count() const { return tx_store_.size(); }
    const std::vector<std::vector<uint8_t>>& sent() const { return tx_store_; }
private:
    std::vector<std::vector<uint8_t>> rx_store_;
    std::vector<std::vector<uint8_t>> tx_store_;
    size_t recv_cursor_;
};

} // namespace udp
