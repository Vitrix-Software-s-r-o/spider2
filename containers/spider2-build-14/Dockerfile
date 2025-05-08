FROM gcc:14.2.0 AS spider2-vckpg-download

RUN git clone https://github.com/microsoft/vcpkg.git --depth 1 /vcpkg-master
WORKDIR "/vcpkg-master"
RUN git fetch --unshallow && git pull

RUN apt-get update
RUN apt-get -y install curl zip unzip tar cmake ninja-build python3 python3-dev

ENV VCPKG_FORCE_SYSTEM_BINARIES=1
RUN ./bootstrap-vcpkg.sh

FROM spider2-vckpg-download AS spider2-vckpg



ENV VCPKG_DEFAULT_BINARY_CACHE=/vcpkg-master/cache
RUN mkdir -p $VCPKG_DEFAULT_BINARY_CACHE
ADD vcpkg.json /vcpkg-master/vcpkg.json
ADD containers/spider2-build/overlays /vcpkg-master/overlays
RUN ./vcpkg install --overlay-ports=/vcpkg-master/overlays