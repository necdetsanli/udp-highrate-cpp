#include "udp/server.hpp"
#include "udp/socket.hpp"
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

using namespace udp;

// Global flag toggled by signal handlers to stop the server gracefully.
static std::atomic<bool> g_keepRunning{true};

static void handle_signal(int) {
    g_keepRunning = false;
}

int main(int argc, char** argv) {
    ServerConfig cfg;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--port") && i + 1 < argc) cfg.port = static_cast<uint16_t>(std::atoi(argv[++i]));
        else if (!std::strcmp(argv[i], "--batch") && i + 1 < argc) cfg.batch = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--metrics-port") && i + 1 < argc) cfg.metrics_port = static_cast<uint16_t>(std::atoi(argv[++i]));
        else if (!std::strcmp(argv[i], "--echo")) cfg.echo = true;
        else if (!std::strcmp(argv[i], "--reuseport")) cfg.reuseport = true;
        else if (!std::strcmp(argv[i], "--verbose")) cfg.verbose = true;
        else if (!std::strcmp(argv[i], "--quiet")) cfg.verbose = false;
        else if (!std::strcmp(argv[i], "--help")) {
            std::cout << "udp_server --port <p> --batch <n> --metrics-port <p> [--echo] [--reuseport] [--verbose|--quiet]\n";
            return 0;
        }
    }

    try {
        auto sock = std::make_unique<UdpSocket>(cfg.batch);
        UdpServer server(std::move(sock), cfg);
        server.start();

        // Register signal handlers, then idle until a termination signal arrives.
        std::signal(SIGINT,  handle_signal);
        std::signal(SIGTERM, handle_signal);
        while (g_keepRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        server.stop();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
        return 1;
    }
}
