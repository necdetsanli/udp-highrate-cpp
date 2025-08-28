
#include "udp/socket.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <cerrno>
#include <sys/types.h>
#include <fcntl.h>

namespace udp {

void ISocket::set_rcvbuf(int bytes) {
    (void)bytes;
}

void ISocket::set_sndbuf(int bytes) {
    (void)bytes;
}

static int make_socket() {
    int s = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) throw std::runtime_error("socket() failed");
    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);
    return s;
}

UdpSocket::UdpSocket(int batch_hint) : sockfd_(make_socket()), batch_hint_(batch_hint), connected_(false) {
    int one = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
}

UdpSocket::~UdpSocket() {
    if (sockfd_ >= 0) ::close(sockfd_);
}

void UdpSocket::bind(uint16_t port, bool reuseport) {
    if (reuseport) {
#ifdef SO_REUSEPORT
        int one = 1;
        setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
#endif
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (::bind(sockfd_, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed: " + std::string(strerror(errno)));
}

void UdpSocket::connect(const std::string& ip, uint16_t port) {
    memset(&peer_, 0, sizeof(peer_));
    peer_.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &peer_.sin_addr);
    peer_.sin_port = htons(port);
    if (::connect(sockfd_, (sockaddr*)&peer_, sizeof(peer_)) < 0)
        throw std::runtime_error("connect() failed: " + std::string(strerror(errno)));
    connected_ = true;
}

ssize_t UdpSocket::recv_batch(std::vector<std::vector<uint8_t>>& bufs) {
#if defined(__linux__)
    // Use recvmmsg if available
    const size_t n = bufs.size();
    std::vector<iovec> iov(n);
    std::vector<mmsghdr> msgs(n);
    std::vector<sockaddr_in> addrs(n);
    std::vector<char> ctrl(64 * n);

    for (size_t i=0;i<n;i++) {
        iov[i].iov_base = bufs[i].data();
        iov[i].iov_len = bufs[i].size();
        memset(&msgs[i], 0, sizeof(mmsghdr));
        msgs[i].msg_hdr.msg_iov = &iov[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
        msgs[i].msg_hdr.msg_name = &addrs[i];
        msgs[i].msg_hdr.msg_namelen = sizeof(sockaddr_in);
        msgs[i].msg_hdr.msg_control = ctrl.data() + i*64;
        msgs[i].msg_hdr.msg_controllen = 64;
    }
    int r = recvmmsg(sockfd_, msgs.data(), n, 0, nullptr);
    if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
    if (r < 0) return -1;
    return r;
#else
    // Fallback to single recvfrom
    sockaddr_in addr{};
    socklen_t alen = sizeof(addr);
    ssize_t r = recvfrom(sockfd_, bufs[0].data(), bufs[0].size(), 0, (sockaddr*)&addr, &alen);
    if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
    if (r < 0) return -1;
    return 1;
#endif
}

ssize_t UdpSocket::send_batch(const std::vector<std::vector<uint8_t>>& bufs, const sockaddr_in* addr) {
#if defined(__linux__)
    const size_t n = bufs.size();
    std::vector<iovec> iov(n);
    std::vector<mmsghdr> msgs(n);
    for (size_t i=0;i<n;i++) {
        iov[i].iov_base = const_cast<uint8_t*>(bufs[i].data());
        iov[i].iov_len = bufs[i].size();
        memset(&msgs[i], 0, sizeof(mmsghdr));
        msgs[i].msg_hdr.msg_iov = &iov[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
        if (!connected_) {
            msgs[i].msg_hdr.msg_name = const_cast<sockaddr_in*>(addr);
            msgs[i].msg_hdr.msg_namelen = sizeof(sockaddr_in);
        }
    }
    int r = sendmmsg(sockfd_, msgs.data(), n, 0);
    if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
    if (r < 0) return -1;
    return r;
#else
    // Fallback to single sendto/connect
    ssize_t cnt = 0;
    for (auto& b : bufs) {
        ssize_t r;
        if (connected_) r = ::send(sockfd_, b.data(), b.size(), 0);
        else r = ::sendto(sockfd_, b.data(), b.size(), 0, (sockaddr*)addr, sizeof(sockaddr_in));
        if (r >= 0) cnt++;
    }
    return cnt;
#endif
}

void UdpSocket::set_rcvbuf(int bytes) {
    setsockopt(sockfd_, SOL_SOCKET, SO_RCVBUF, &bytes, sizeof(bytes));
}
void UdpSocket::set_sndbuf(int bytes) {
    setsockopt(sockfd_, SOL_SOCKET, SO_SNDBUF, &bytes, sizeof(bytes));
}

ssize_t MockSocket::recv_batch(std::vector<std::vector<uint8_t>>& bufs) {
    size_t i=0;
    for (; i<bufs.size() && recv_cursor_ < rx_store_.size(); ++i, ++recv_cursor_) {
        auto& src = rx_store_[recv_cursor_];
        auto& dst = bufs[i];
        size_t n = std::min(dst.size(), src.size());
        std::copy(src.begin(), src.begin()+n, dst.begin());
    }
    return static_cast<ssize_t>(i);
}

ssize_t MockSocket::send_batch(const std::vector<std::vector<uint8_t>>& bufs, const sockaddr_in* ) {
    for (auto& b : bufs) tx_store_.push_back(b);
    return static_cast<ssize_t>(bufs.size());
}

} // namespace udp
