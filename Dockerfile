# Stage 1a: Base build environment
FROM ubuntu:24.04 AS base

ENV DEBIAN_FRONTEND=noninteractive

# Combine all apt-get commands and clean up to reduce layers and image size
RUN apt-get update && \
    apt-get install --no-install-recommends -y \
    build-essential \
    sudo \
    make \
    ninja-build \
    libboost-all-dev \
    libjemalloc-dev \
    libbz2-dev \
    liblz4-dev \
    libevent-dev \
    libsodium-dev \
    libssl-dev \
    libunwind-dev \
    libzstd-dev \
    libdwarf-dev \
    libsnappy-dev \
    libiberty-dev \
    libncurses-dev \
    libffi-dev \
    libgmock-dev \
    libre2-dev \
    libdouble-conversion-dev \
    libgoogle-glog-dev \
    libaio-dev \
    binutils-dev \
    bison \
    flex \
    git \
    unzip \
    python3-pip \
    python3-six \
    python3-click \
    cython3 \
    cmake \
    m4 \
    g++-14 \
    gcc-14 \
    re2c \
    wget \
    pkg-config \
    bash && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

FROM base AS base-install

# Combine all Python package installations into one step and clean up
RUN pip install hexdump --break-system-packages && \
    wget https://files.pythonhosted.org/packages/source/b/bunch/bunch-1.0.1.zip && \
    unzip bunch-1.0.1.zip && \
    cd bunch-1.0.1 && \
    sed -i "s/'rU'/'r'/g" setup.py && \
    python3 setup.py install && \
    cd / && rm -rf bunch-1.0.1 bunch-1.0.1.zip

FROM base-install AS install-liburing

# Clone and build liburing in a single RUN command
RUN cd /tmp && \
    git clone --branch liburing-2.7 https://github.com/axboe/liburing.git && \
    cd liburing && \
    ./configure --cc=gcc --cxx=g++ && \
    make -j$(nproc) && make install && \
    cd / && rm -rf /tmp/liburing

FROM install-liburing AS install-fmt

# Clone and build fmt in a single RUN command
RUN cd /tmp && \
    git clone --branch 11.0.2 https://github.com/fmtlib/fmt.git && \
    cd fmt && mkdir _build && cd _build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/fmt .. && \
    make -j$(nproc) && make install && \
    cd / && rm -rf /tmp/fmt

ENV CMAKE_PREFIX_PATH="/opt/fmt:$CMAKE_PREFIX_PATH" \
    LD_LIBRARY_PATH="/opt/fmt/lib:$LD_LIBRARY_PATH"

FROM install-fmt AS install-range-v3

# Clone and build range-v3 in a single RUN command
RUN cd /tmp && \
    git clone --branch 0.12.0 https://github.com/ericniebler/range-v3.git && \
    cd range-v3 && mkdir _build && cd _build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/range-v3 .. && \
    make -j$(nproc) && make install && \
    cd / && rm -rf /tmp/range-v3

ENV CMAKE_PREFIX_PATH="/opt/range-v3:$CMAKE_PREFIX_PATH" \
    LD_LIBRARY_PATH="/opt/range-v3/lib:$LD_LIBRARY_PATH"

FROM install-range-v3 AS install-fast_float

# Clone and build fast_float in a single RUN command
RUN cd /tmp && \
    git clone --branch v6.1.5 https://github.com/fastfloat/fast_float.git && \
    cd fast_float && mkdir _build && cd _build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/fast_float .. && \
    make -j$(nproc) && make install && \
    cd / && rm -rf /tmp/fast_float

ENV CMAKE_PREFIX_PATH="/opt/fast_float:$CMAKE_PREFIX_PATH" \
    LD_LIBRARY_PATH="/opt/fast_float/lib:$LD_LIBRARY_PATH"

FROM install-fast_float AS env_setup

# Create directories and set up environment in a single RUN command
RUN mkdir -p /deps /src/openr

ENV CXX=/usr/bin/g++-14
ENV CC=/usr/bin/gcc-14
ENV CFLAGS="-std=c17 -fcoroutines -fPIC"
ENV CXXFLAGS="-std=c++20 -fPIC -fcoroutines -D_CPPLIB_VER=20"
ENV CMAKE_CXX_STANDARD=20
ENV CMAKE_PREFIX_PATH="/usr/lib/python3/dist-packages:$CMAKE_PREFIX_PATH"

FROM env_setup AS build_folly

# Copy and build folly in a single RUN command
COPY deps/folly /deps/folly
RUN cd /deps/folly && \
    mkdir _build && cd _build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/folly .. && \
    make -j$(nproc) && make install && \
    cd / && rm -rf /deps/folly/_build

ENV CMAKE_PREFIX_PATH="/opt/folly:$CMAKE_PREFIX_PATH"
ENV LD_LIBRARY_PATH="/opt/folly/lib:$LD_LIBRARY_PATH"

FROM build_folly AS build_fizz

# Copy and build fizz in a single RUN command
COPY deps/fizz /deps/fizz
RUN cd /deps/fizz && \
    mkdir _build && cd _build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/fizz ../fizz && \
    make -j$(nproc) && make install && \
    cd / && rm -rf /deps/fizz/_build

ENV CMAKE_PREFIX_PATH="/opt/fizz:$CMAKE_PREFIX_PATH"
ENV LD_LIBRARY_PATH="/opt/fizz/lib:$LD_LIBRARY_PATH"

FROM build_fizz AS build_wangle

# Copy and build wangle in a single RUN command
COPY deps/wangle /deps/wangle
RUN cd /deps/wangle && \
    mkdir _build && cd _build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/wangle ../wangle && \
    make -j$(nproc) && make install && \
    cd / && rm -rf /deps/wangle/_build

ENV CMAKE_PREFIX_PATH="/opt/wangle:$CMAKE_PREFIX_PATH"
ENV LD_LIBRARY_PATH="/opt/wangle/lib:$LD_LIBRARY_PATH"

FROM build_wangle AS build_mvfst

# Copy and build mvfst in a single RUN command
COPY deps/mvfst /deps/mvfst
RUN cd /deps/mvfst && \
    mkdir _build && cd _build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/mvfst .. && \
    make -j$(nproc) && make install && \
    cd / && rm -rf /deps/mvfst/_build

ENV CMAKE_PREFIX_PATH="/opt/mvfst:$CMAKE_PREFIX_PATH"
ENV LD_LIBRARY_PATH="/opt/mvfst/lib:$LD_LIBRARY_PATH"

FROM build_mvfst AS build_fbthrift

# Copy and build fbthrift in a single RUN command
COPY deps/fbthrift /deps/fbthrift
RUN cd /deps/fbthrift && \
    mkdir _build && cd _build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/fbthrift .. && \
    make -j$(nproc) && make install && \
    cd / && rm -rf /deps/fbthrift/_build

ENV CMAKE_PREFIX_PATH="/opt/fbthrift:$CMAKE_PREFIX_PATH"
ENV LD_LIBRARY_PATH="/opt/fbthrift/lib:$LD_LIBRARY_PATH"

FROM build_fbthrift AS build_fb303

COPY FBThriftConfig.cmake /opt/fbthrift/lib/cmake/fbthrift/
COPY schema.thrift /opt/fbthrift/include/thrift/lib/thrift/

# Copy and build fb303 in a single RUN command
COPY deps/fb303 /deps/fb303
RUN cd /deps/fb303 && \
    mkdir _build && cd _build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/fb303 .. && \
    make -j$(nproc) && make install && \
    cd / && rm -rf /deps/fb303/_build

ENV CMAKE_PREFIX_PATH="/opt/fb303:$CMAKE_PREFIX_PATH"
ENV LD_LIBRARY_PATH="/opt/fb303/lib:$LD_LIBRARY_PATH"

FROM build_fb303 AS build_openr

# Copy and build openr in a single RUN command
COPY openr /src/openr
RUN cd /src/openr && \
    mkdir _build && cd _build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/openr .. && \
    make -j$(nproc) && make install && \
    cd / && rm -rf /src/openr/_build

ENV CMAKE_PREFIX_PATH="/opt/openr:$CMAKE_PREFIX_PATH"
ENV LD_LIBRARY_PATH="/opt/openr/lib:$LD_LIBRARY_PATH"

FROM build_openr AS install_openr

COPY gen.py /src/openr/build
COPY cython_compile.py /src/openr/build

# Install Open/R
RUN mkdir -p /src/openr/fb303-thrift && \
    cp -r /opt/fb303/include/thrift-files/fb303 /src/openr/fb303-thrift && \
    mkdir -p /src/openr/thrift/py3 && \
    mkdir -p /src/openr/thrift/python && \
    cp -r /opt/fbthrift/include/thrift/lib/ /src/openr/thrift && \
    touch /src/openr/thrift/py3/__init__.py && \
    touch /src/openr/thrift/__init__.py && \
    mkdir -p /src/openr/fbthrift-thrift && \
    cp -r /deps/fbthrift/thrift/lib/thrift/* /src/openr/fbthrift-thrift && \
    cp -r /deps/fbthrift/thrift/lib/py3/* /src/openr/thrift/py3/ && \
    cp -r /deps/fbthrift/thrift/lib/python/* /src/openr/thrift/python/ && \
    mkdir -p /src/openr/openr-thrift/openr && \
    cp -r /src/openr/openr/if /src/openr/openr-thrift/openr/ && \
    mkdir -p /src/openr/neteng-thrift/configerator/structs/neteng/config && \
    cp -r /src/openr/configerator/structs/neteng/config/* /src/openr/neteng-thrift/configerator/structs/neteng/config && \
    mkdir -p /src/openr/folly && \
    cp -r /deps/folly/folly/python/* /src/openr/folly/

RUN cd /src/openr && \
    python3 build/gen.py

RUN echo " " > /src/openr/fbthrift-thrift/gen-cpp2/metadata_metadata.h && \
echo " " > /src/openr/fbthrift-thrift/gen-cpp2/metadata_types.h && \
echo " " > /src/openr/fbthrift-thrift/gen-cpp2/metadata_types_custom_protocol.h

RUN cd /src/openr && \
    python3 build/cython_compile.py && \
    python3 openr/py/setup.py build -j$(nproc) && \
    python3 openr/py/setup.py install

RUN cp -r /src/openr/build/lib.linux-x86_64-cpython-312 /breeze-build

FROM install_openr AS openr

# Create scripts and config directories
RUN mkdir -p /config && mkdir -p /scripts

# Copy entrypoint script
COPY run_openr.sh /scripts
COPY entrypoint.sh /scripts

RUN chmod +x /scripts/run_openr.sh && \
    chmod +x /scripts/entrypoint.sh

USER root

ENTRYPOINT ["/scripts/entrypoint.sh"]

# Expose the default port if no config file is provided
EXPOSE 2018/tcp

