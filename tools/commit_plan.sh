#!/usr/bin/env bash
set -euo pipefail
# Create a clean commit history representing incremental development steps.
git init
git add LICENSE README.md diagrams docker include src tools CMakeLists.txt .github tests
git commit -m "chore: repo scaffold (README, license, CMake, structure)"
git add include/udp/common.hpp include/udp/socket.hpp src/socket.cpp
git commit -m "feat(core): base UDP socket (batch send/recv), mock socket"
git add include/udp/stats.hpp src/stats.cpp
git commit -m "feat(core): stats counters and client map"
git add include/udp/metrics_http.hpp src/metrics_http.cpp
git commit -m "feat(metrics): minimal HTTP /metrics for Prometheus"
git add include/udp/server.hpp src/server.cpp src/main_server.cpp
git commit -m "feat(server): high-rate UDP server with echo option and per-second stats"
git add include/udp/client.hpp src/client.cpp src/main_client.cpp
git commit -m "feat(client): high-rate UDP client with pacing and batching"
git add tests
git commit -m "test: GoogleTest suite for 100% coverage target"
git add tools
git commit -m "chore(tools): e2e demo, coverage, tuning notes, prom/grafana"
git add docker .github
git commit -m "chore(ci+docker): dockerfiles and CI pipeline"
echo "Commit plan applied. Now set your remote and push."
