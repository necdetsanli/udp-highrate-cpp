
#include "udp/client.hpp"
#include "udp/socket.hpp"
#include <iostream>
#include <cstring>

using namespace udp;

int main(int argc, char** argv) {
    ClientConfig cfg;
    for (int i=1;i<argc;i++){
        if (!strcmp(argv[i],"--server") && i+1<argc) cfg.server_ip = argv[++i];
        else if (!strcmp(argv[i],"--port") && i+1<argc) cfg.port = (uint16_t)atoi(argv[++i]);
        else if (!strcmp(argv[i],"--pps") && i+1<argc) cfg.pps = (uint64_t)atoll(argv[++i]);
        else if (!strcmp(argv[i],"--seconds") && i+1<argc) cfg.seconds = atoi(argv[++i]);
        else if (!strcmp(argv[i],"--payload") && i+1<argc) cfg.payload = atoi(argv[++i]);
        else if (!strcmp(argv[i],"--batch") && i+1<argc) cfg.batch = atoi(argv[++i]);
        else if (!strcmp(argv[i],"--id") && i+1<argc) cfg.id = atoi(argv[++i]);
        else if (!strcmp(argv[i],"--verbose")) cfg.verbose = true;
        else if (!strcmp(argv[i],"--help")) {
            std::cout << "udp_client --server <ip> --port <p> --pps <n> --seconds <n> --payload <n> --batch <n> --id <n> [--verbose]\n";
            return 0;
        }
    }
    try {
        auto sock = std::make_unique<UdpSocket>(cfg.batch);
        UdpClient client(std::move(sock), cfg);
        client.start();
        // Wait for the client run loop to finish based on --seconds.
        client.join();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << "\n";
        return 1;
    }
}
