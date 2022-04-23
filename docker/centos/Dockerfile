# Sets up an environment for building projects compatible with CentoOS/RHEL. Links against
# glib v2.17 for compatibility with rhel7 hosts.
#
# Installs updates versions of git, cmake and openssl which are required for building
# more modern CMake projects.
#
# docker build -t aquaq/centos7 .
# docker run --rm -it -p 2222:22 -v $(pwd):/build aquaq/centos7
#
FROM centos:centos7
SHELL ["/bin/bash", "-c"]

ARG GIT_VERSION=2.33.0
ARG CMAKE_VERSION=3.21.5
ARG OPENSSL_VERSION=1.1.1m

# Install dependencies for the build. We need to have more recent versions of CMake/Git/OpenSSH
# than provided by yum, so we install those manually.
RUN yum install -y  \
    unzip           \
    expat-devel     \
    libcurl-devel   \
    python3         \
    perl            \
    wget            \
    gcc             \
    gcc-c++         \
    make            \
    zlib-devel      \
    gettext         \
    tcl

RUN wget --no-check-certificate https://mirrors.edge.kernel.org/pub/software/scm/git/git-${GIT_VERSION}.tar.gz && \
      tar -xvf git-${GIT_VERSION}.tar.gz && \
      pushd git-${GIT_VERSION} && \
      ./configure && \
      make && \
      make install && \
      popd && \
      rm -rf git-${GIT_VERSION} \

      wget --no-check-certificate https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz && \
      tar -xvf openssl-${OPENSSL_VERSION}.tar.gz  && \
      pushd openssl-${OPENSSL_VERSION} && \
      ./config && \
      make && \
      make install && \
      popd && \
      rm -rf openssl-${OPENSSL_VERSION} \

      wget --no-check-certificate https://github.com/Kitware/CMake/archive/refs/tags/v${CMAKE_VERSION}.tar.gz && \
      tar -xvf v${CMAKE_VERSION}.tar.gz && \
      pushd CMake-${CMAKE_VERSION} && \
      ./bootstrap && \
      make && \
      make install && \
      popd && \
      rm -rf CMake-${CMAKE_VERSION}

# Give up root permissions and run the rest of the build steps as a local user.
RUN useradd aquaq-build && mkdir /build && chown -R aquaq-build /build
USER aquaq-build
WORKDIR /build
ADD ./ ./