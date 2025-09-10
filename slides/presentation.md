
---
marp: true
theme: default
paginate: true
---

# UDP High-Rate C++ Server/Client
**Target:** ≥100 kpps, ≥10 clients, tests + coverage, UML, E2E, metrics, Docker

---

## Architecture
- C++17, Linux-first
- Batching with `recvmmsg` / `sendmmsg`
- Server single-thread loop (scale via `--reuseport`)
- Prometheus `/metrics` HTTP endpoint

---

## Data Model
- `PacketHeader { seq, send_ts_ns, magic }`
- Payload configurable (default 64B)

---

## Throughput Strategy
- Pre-allocated batch buffers (64)
- Non-blocking sockets, large rcv/snd buffers
- Pacing clients to achieve aggregate ≥100 kpps

---

## Testing
- GoogleTest unit tests
- 100% coverage target via lcov (CI checks)
- MockSocket for deterministic logic tests

---

## E2E Demo
- Local loopback, 10 clients × 10 kpps × 5s
- Script: `tools/run_e2e_local.sh`

---

## Observability
- `/metrics` in Prometheus text format
- Starter Grafana dashboard JSON

---

## Pros
- High throughput on commodity HW
- Minimal deps; portable fallback
- Clear testability

## Cons / Limits
- Linux-centric
- Single-threaded server by default
- Actual pps depends on NIC/sysctl

---

## Roadmap (Optional)
- Thread-per-core with RSS / SO_REUSEPORT
- io_uring backend
- Qt GUI control panel
