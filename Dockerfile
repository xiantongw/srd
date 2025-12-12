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

# Install spdlog
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      libspdlog-dev && \
    rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir \
      pytest \
      black \
      isort || true

RUN useradd -ms /bin/bash user && \
    mkdir -p /home/user/.cache && \
    chown -R user:user /home/user

USER user
WORKDIR /workspace