# Guandan 服务端镜像（骨架，未在本环境实测 Docker）。
FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake g++ protobuf-compiler libprotobuf-dev libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*
COPY . /src
WORKDIR /src
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j"$(nproc)"

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    libprotobuf32 libsqlite3-0 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=build /src/build/guandan_server /usr/local/bin/guandan_server
WORKDIR /data
EXPOSE 9001
CMD ["guandan_server", "9001", "/data/guandan.db"]
