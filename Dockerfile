FROM ubuntu:22.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    g++ \
    libssl-dev \
    libboost-system-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=OFF \
    -DENABLE_ELECTRUM=ON \
    && cmake --build build --parallel $(nproc) \
    && strip build/bitcoin2maxd

# ── Runtime image ─────────────────────────────────────────────────────────────
FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --no-create-home --shell /usr/sbin/nologin bitcoin2max

COPY --from=builder /src/build/bitcoin2maxd /usr/local/bin/bitcoin2maxd
COPY --from=builder /src/conf/bitcoin2max.conf /etc/bitcoin2max/bitcoin2max.conf

RUN mkdir -p /var/lib/bitcoin2max && chown bitcoin2max /var/lib/bitcoin2max

USER bitcoin2max

EXPOSE 8333 9050

ENTRYPOINT ["/usr/local/bin/bitcoin2maxd"]
CMD ["--conf", "/etc/bitcoin2max/bitcoin2max.conf"]
