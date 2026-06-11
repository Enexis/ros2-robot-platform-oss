FROM ubuntu:latest AS ntrip_client

RUN apt-get update && apt-get install -y --no-install-recommends \
    wget \
    ca-certificates \
    unzip \
    libqt5svg5 \
    libqt5printsupport5 \
    libqt5core5a \
    libqt5network5 \
    libqt5xml5 \
    && rm -rf /var/lib/apt/lists/*
    
WORKDIR /opt/BNC

COPY --from=bnc /tmp/BNC/bnc /opt/BNC/

# Add BNC to PATH
ENV PATH="/opt/BNC:${PATH}"
