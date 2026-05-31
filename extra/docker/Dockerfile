FROM ubuntu:24.04

# Set timezone to UTC. This prevents apt-get install SOME-PACKAGES from asking for timezone during setup
ENV TZ=UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && apt-get install --no-install-recommends -y \
    build-essential \
    cmake \
    pkg-config \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-l10n-tools \
    libqt6core6 \
    libqt6gui6 \
    libqt6widgets6 \
    libexiv2-dev \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /staging/sago_image_browser

COPY . /staging/sago_image_browser

RUN cd /staging/sago_image_browser && rm -rf build && mkdir build && cd build && \
cmake .. && \
make -j$(nproc)
