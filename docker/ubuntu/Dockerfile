# Sets up an environment for building projects compatible with Debian/Ubuntu. Links against
# glib v2.31
#
# docker build -t aquaq/ubuntu:20.04 .
# docker run --rm -it -p 2222:22 -v $(pwd):/build aquaq/ubuntu:20.04
#
FROM ubuntu:20.04
SHELL ["/bin/bash", "-c"]

# Install dependencies for the build.
RUN apt-get update && DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get -y install \
        build-essential \
        git \
        cmake

# Give up root permissions and run the rest of the build steps as a local user.
RUN useradd aquaq-build && mkdir /build && chown -R aquaq-build /build
USER aquaq-build
WORKDIR /build
ADD ./ ./