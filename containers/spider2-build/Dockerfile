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

ADD https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-linux.zip /tools/ninja-1.12.1.zip
RUN unzip /tools/ninja-1.12.1.zip -d /tools/ninja

ADD https://github.com/Kitware/CMake/releases/download/v4.0.2/cmake-4.0.2-linux-x86_64.tar.gz /tools/cmake-4.0.2-linux-x86_64.tar.gz
RUN tar -xzf /tools/cmake-4.0.2-linux-x86_64.tar.gz -C /tools/

ENV PATH="/tools/ninja:/tools/:/tools/cmake-4.0.2-linux-x86_64/bin:$PATH"