# Start with Ubuntu LTS
FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    git \
    gcc \
    g++ \
    make \
    cmake \
    libboost-all-dev \
    qtbase5-dev \
    libqt5charts5-dev \
    && rm -rf /var/lib/apt/lists/*

# Create conceal user and necessary directories
RUN useradd -m -u 1000 -s /bin/bash conceal && \
    mkdir -p /home/conceal/.conceal && \
    mkdir -p /home/conceal/.cache && \
    mkdir -p /home/conceal/wallets && \
    mkdir -p /tmp/runtime-root && \
    chmod 700 /tmp/runtime-root

# Set permissions
RUN chown -R conceal:conceal /home/conceal /tmp/runtime-root

# Create app directory
WORKDIR /app

# Clone repositories (with proper cleanup)
RUN git clone https://github.com/ConcealNetwork/conceal-desktop . && \
    rm -rf cryptonote && \
    git clone https://github.com/ConcealNetwork/conceal-core cryptonote

# Build the application
RUN make build-release && \
    mkdir -p bin && \
    mv build/release/conceal-desktop bin/ && \
    make clean

# Create a directory for the wallet files
# RUN mkdir -p /root/.conceal

# Expose default port
EXPOSE 16000

# Set the entrypoint
ENTRYPOINT ["./bin/conceal-desktop"] 