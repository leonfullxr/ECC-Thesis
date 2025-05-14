# ─────────────── builder ───────────────
FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

# install build tools & certs
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential wget tar m4 libtool autoconf automake pkg-config ca-certificates \
 && rm -rf /var/lib/apt/lists/*

# build GMP (C + C++)
ARG GMP_VERSION=6.3.0
WORKDIR /tmp
RUN wget https://ftp.gnu.org/gnu/gmp/gmp-${GMP_VERSION}.tar.xz \
 && tar xf gmp-${GMP_VERSION}.tar.xz \
 && cd gmp-${GMP_VERSION} \
 && ./configure --prefix=/usr/local --enable-cxx \
 && make -j"$(nproc)" \
 && make install

# build NTL
ARG NTL_VERSION=11.5.1
RUN cd /tmp \
 && wget https://libntl.org/ntl-${NTL_VERSION}.tar.gz \
 && tar xf ntl-${NTL_VERSION}.tar.gz \
 && cd ntl-${NTL_VERSION}/src \
 && CXXFLAGS="-I/usr/local/include" \
    LDFLAGS="-L/usr/local/lib -Wl,-rpath=/usr/local/lib" \
    ./configure PREFIX=/usr/local GMP_PREFIX=/usr/local \
 && make -j"$(nproc)" \
 && make install

# ─────────────── final ───────────────
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
# ensure the runtime linker sees /usr/local/lib
ENV LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH:-}"

# bring in GMP & NTL from the builder stage
COPY --from=builder /usr/local /usr/local

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      python3 python3-pip ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
ENV PATH=/workspace/bin:$PATH

CMD ["bash"]
