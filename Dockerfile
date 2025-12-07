# Dockerfile
FROM ubuntu:22.04

ENV LD_LIBRARY_PATH=/usr/local/lib
ENV DEBIAN_FRONTEND=noninteractive

# 1) build deps + CA certs
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential wget tar m4 libtool autoconf automake pkg-config ca-certificates \
      python3 python3-pip \
 && update-ca-certificates \
 && rm -rf /var/lib/apt/lists/*

# 2) build & install GMP 6.3.0
WORKDIR /tmp
ARG GMP_VERSION=6.3.0
RUN wget \
      --tries=5 \
      --timeout=30 \
      --waitretry=10 \
      https://ftp.gnu.org/gnu/gmp/gmp-${GMP_VERSION}.tar.xz \
  && tar xf gmp-${GMP_VERSION}.tar.xz \
  && cd gmp-${GMP_VERSION} \
  && ./configure --prefix=/usr/local \
  && make -j"$(nproc)" \
  && make install \
  && cd /tmp \
  && rm -rf gmp-${GMP_VERSION} gmp-${GMP_VERSION}.tar.xz

# 3) build & install NTL 11.5.1 (force loader to use /usr/local/lib)
WORKDIR /tmp
RUN wget https://libntl.org/ntl-11.5.1.tar.gz \
 && tar xf ntl-11.5.1.tar.gz \
 && cd ntl-11.5.1/src \
 && export CXXFLAGS="-I/usr/local/include" \
 && export LDFLAGS="-L/usr/local/lib -Wl,-rpath=/usr/local/lib" \
 && export LD_LIBRARY_PATH="/usr/local/lib" \
 && ./configure PREFIX=/usr/local GMP_PREFIX=/usr/local \
 && make -j"$(nproc)" && make install

# 4) build python packages
RUN pip3 install matplotlib pandas seaborn numpy scipy

# 5) final cleanup
WORKDIR /workspace
ENV PATH=/workspace/bin:$PATH

CMD ["bash"]
