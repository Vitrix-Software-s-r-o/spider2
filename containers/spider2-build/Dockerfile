FROM gcc:15.2.0 AS spider2-vckpg-download

RUN git clone https://github.com/microsoft/vcpkg.git --depth 1 --branch master /vcpkg-master
WORKDIR "/vcpkg-master"
RUN git fetch --unshallow && git pull

RUN apt-get update
RUN apt-get -y install curl zip unzip tar cmake ninja-build python3 python3-dev gdb
RUN apt-get -y install autoconf automake autoconf-archive

ENV VCPKG_FORCE_SYSTEM_BINARIES=1
RUN ./bootstrap-vcpkg.sh

FROM spider2-vckpg-download AS spider2-vckpg

ENV VCPKG_DEFAULT_BINARY_CACHE=/vcpkg-master/cache
RUN mkdir -p $VCPKG_DEFAULT_BINARY_CACHE

ADD vcpkg.json /vcpkg-master/vcpkg.json
#ADD containers/spider2-build/overlays /vcpkg-master/overlays
#--overlay-ports=/vcpkg-master/overlays
RUN ./vcpkg install

RUN mkdir "/tools"
# Download architecture-specific tools based on platform
RUN ARCH=$(uname -m) && \
    if [ "$ARCH" = "x86_64" ]; then \
        CMAKE_ARCH="x86_64"; \
    elif [ "$ARCH" = "aarch64" ]; then \
        CMAKE_ARCH="aarch64"; \
    else \
        echo "Unsupported architecture: $ARCH" && exit 1; \
    fi && \
    curl -L https://github.com/Kitware/CMake/releases/download/v4.2.0/cmake-4.2.0-linux-${CMAKE_ARCH}.tar.gz -o /tools/cmake-4.2.0-linux-${CMAKE_ARCH}.tar.gz && \
    tar -xzf /tools/cmake-4.2.0-linux-${CMAKE_ARCH}.tar.gz -C /tools/ && \
    ln -s /tools/cmake-4.2.0-linux-${CMAKE_ARCH} /tools/cmake

RUN ARCH=$(uname -m) && \
    if [ "$ARCH" = "x86_64" ]; then \
        NINJA_FILE="ninja-linux.zip"; \
    elif [ "$ARCH" = "aarch64" ]; then \
        NINJA_FILE="ninja-linux-aarch64.zip"; \
    else \
        echo "Unsupported architecture: $ARCH" && exit 1; \
    fi && \
    curl -L https://github.com/ninja-build/ninja/releases/download/v1.13.1/${NINJA_FILE} -o /tools/ninja.zip && \
    unzip /tools/ninja.zip -d /tools/ninja

ENV PATH="/tools/ninja:/tools/:/tools/cmake/bin:$PATH"
