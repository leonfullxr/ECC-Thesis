# Dockerfile
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# 1) build deps + CA certs
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential wget tar m4 libtool autoconf automake pkg-config ca-certificates \
 && update-ca-certificates \
 && rm -rf /var/lib/apt/lists/*

# 2) build & install GMP 6.3.0
WORKDIR /tmp
RUN wget https://gmplib.org/download/gmp/gmp-6.3.0.tar.xz \
 && tar xf gmp-6.3.0.tar.xz \
 && cd gmp-6.3.0 \
 && ./configure --prefix=/usr/local \
 && make -j"$(nproc)" && make install

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

# 4) final cleanup
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
WORKDIR /workspace
# add your repo's bin/ so that any executable you place there is on PATH
ENV PATH=/workspace/bin:$PATH

CMD ["bash"]
