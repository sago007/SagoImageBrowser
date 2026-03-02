FROM ubuntu:24.04

# Set timezone to UTC. This prevents apt-get install SOME-PACKAGES from asking for timezone during setup
ENV TZ=UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && apt-get install --no-install-recommends -y \
    build-essential \
    cmake \
    qt6-base-dev \
    libqt6core6 \
    libqt6gui6 \
    libqt6widgets6 \
    && rm -rf /var/lib/apt/lists/* 

RUN mkdir -p /staging/sago_image_browser

COPY . /staging/sago_image_browser

RUN cd /staging/sago_image_browser && mkdir -p build && cd build && \
cmake .. && \
make
