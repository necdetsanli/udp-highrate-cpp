# syntax=docker/dockerfile:1
 
# ---- Build stage ----
FROM ubuntu:22.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ca-certificates && \
    rm -rf /var/lib/apt/lists/*
 
WORKDIR /src
# Copy only what we need (avoid copying host build artifacts)
COPY CMakeLists.txt .
COPY src/ src/
COPY include/ include/
COPY tests/ tests/
 
# Always do an out-of-source build to a clean dir

RUN cmake -S /src -B /build \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_TESTS=OFF \
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=/build/bin
RUN cmake --build /build -j
 
# runtime
FROM ubuntu:22.04 AS runtime
WORKDIR /app
COPY --from=build /build/bin/ /usr/local/bin/
 
EXPOSE 9000 9100
# Default command: run server with metrics
CMD ["udp_server","--port","9000","--metrics-port","9100","--verbose"]
