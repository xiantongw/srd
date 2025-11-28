# Dockerfile.dev
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      build-essential \
      clang \
      clangd \
      clang-format \
      gdb \
      lldb \
      cmake \
      ninja-build \
      git \
      curl \
      wget \
      unzip \
      zip \
      python3 \
      python3-pip \
      pkg-config \
      ca-certificates \
      libc6 \
      libc6-dev && \
    rm -rf /var/lib/apt/lists/*

RUN ARCH="$(uname -m)" && \
    echo "ARCH is $ARCH" && \
    if [ "$ARCH" = "x86_64" ] || [ "$ARCH" = "amd64" ]; then \
      BAZELISK_URL="https://github.com/bazelbuild/bazelisk/releases/download/v1.27.0/bazelisk-linux-amd64"; \
    elif [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ]; then \
      BAZELISK_URL="https://github.com/bazelbuild/bazelisk/releases/download/v1.27.0/bazelisk-linux-arm64"; \
    else \
      echo "Unsupported architecture: $ARCH"; exit 1; \
    fi && \
    curl -L "$BAZELISK_URL" -o /usr/local/bin/bazel && \
    chmod +x /usr/local/bin/bazel

RUN pip3 install --no-cache-dir \
      pytest \
      black \
      isort || true

RUN useradd -ms /bin/bash user && \
    mkdir -p /home/user/.cache && \
    chown -R user:user /home/user

USER user
WORKDIR /workspace