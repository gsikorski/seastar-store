FROM docker.io/fedora:34
WORKDIR /home/src
RUN mkdir -p opt \
    && dnf -y update \
    && dnf -y install ccache git \
    && git clone https://github.com/scylladb/seastar.git --depth=1 --branch=master /opt/seastar \
    && /opt/seastar/install-dependencies.sh
CMD /bin/bash
