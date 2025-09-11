FROM docker.vitrix.cz/spider2-build:latest

ADD . /source

RUN mkdir "/build-asan"
WORKDIR "/build-asan"

RUN cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=/vcpkg-master/scripts/buildsystems/vcpkg.cmake -S /source -B . -DSPIDER2_ENABLE_EXAMPLES=ON  -DSPIDER2_TESTS=ON -DSPIDER2_COMPRESS=ON -DSPIDER2_SANITIZE_ADDRESS=ON
RUN cmake --build . --config RelWithDebInfo


RUN mkdir "/build-ubsan"
WORKDIR "/build-ubsan"

RUN cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=/vcpkg-master/scripts/buildsystems/vcpkg.cmake -S /source -B .  -DSPIDER2_ENABLE_EXAMPLES=ON -DSPIDER2_TESTS=ON -DSPIDER2_COMPRESS=ON -DSPIDER2_SANITIZE_UNDEFINED=ON
RUN cmake --build . --config RelWithDebInfo

ADD ./containers/spider2-tests/run-all-tests.sh /run-all-tests.sh
RUN chmod +x /run-all-tests.sh

CMD ["/run-all-tests.sh"]