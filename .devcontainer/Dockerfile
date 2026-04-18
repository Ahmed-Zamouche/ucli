FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    clangd \
    g++ \
    ca-certificates \
    xz-utils \
    curl \
    lcov \
    python3 \
    git \
    cppcheck \
    && rm -rf /var/lib/apt/lists/*

RUN curl -LO "https://github.com/bazelbuild/bazelisk/releases/download/v1.28.1/bazelisk-linux-amd64"
RUN mv bazelisk-linux-amd64 /usr/local/bin/bazel
RUN chmod +x /usr/local/bin/bazel


WORKDIR /workspace/cli
