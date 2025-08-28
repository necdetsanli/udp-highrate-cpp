
#include "udp/metrics_http.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <thread>
#include <chrono>

namespace udp {

MetricsHttpServer::MetricsHttpServer(Stats& stats, uint16_t port)
: stats_(stats), port_(port) {}

MetricsHttpServer::~MetricsHttpServer() { stop(); }

void MetricsHttpServer::start() {
    if (port_ == 0) return;
    running_ = true;
    th_ = std::thread(&MetricsHttpServer::run, this);
}

void MetricsHttpServer::stop() {
    if (th_.joinable()) {
        running_ = false;
        th_.join();
    }
}

std::string MetricsHttpServer::render() {
    std::ostringstream oss;
    oss << "# HELP udp_packets_received_total Total UDP packets received\n";
    oss << "# TYPE udp_packets_received_total counter\n";
    oss << "udp_packets_received_total " << stats_.recv() << "\n";
    oss << "# HELP udp_packets_sent_total Total UDP packets sent\n";
    oss << "# TYPE udp_packets_sent_total counter\n";
    oss << "udp_packets_sent_total " << stats_.sent() << "\n";
    oss << "# HELP udp_unique_clients Unique client count\n";
    oss << "# TYPE udp_unique_clients gauge\n";
    oss << "udp_unique_clients " << stats_.unique_clients() << "\n";
    oss << "# HELP udp_rx_bytes_total Total received bytes\n";
    oss << "# TYPE udp_rx_bytes_total counter\n";
    oss << "udp_rx_bytes_total " << stats_.rx_bytes() << "\n";
    oss << "# HELP udp_tx_bytes_total Total sent bytes\n";
    oss << "# TYPE udp_tx_bytes_total counter\n";
    oss << "udp_tx_bytes_total " << stats_.tx_bytes() << "\n";
    return oss.str();
}

void MetricsHttpServer::run() {
    int s = ::socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
#ifdef SO_REUSEPORT
    setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
#endif
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port_);
    bind(s, (sockaddr*)&addr, sizeof(addr));
    listen(s, 8);
    while (running_) {
        sockaddr_in peer{};
        socklen_t plen=sizeof(peer);
        int c = accept(s, (sockaddr*)&peer, &plen);
        if (c < 0) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); continue; }
        std::string body = render();
        std::ostringstream resp;
        resp << "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " << body.size()
             << "\r\nConnection: close\r\n\r\n" << body;
        auto sstr = resp.str();
        (void)send(c, sstr.data(), sstr.size(), 0);
        close(c);
    }
    close(s);
}

} // namespace udp
