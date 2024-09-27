FROM docker.io/fedora:38
WORKDIR /home/src
RUN mkdir -p opt \
    && dnf -y update \
    && dnf -y install ccache git wget \
    && git clone https://github.com/scylladb/seastar.git --depth=1 --branch=master /opt/seastar \
    && /opt/seastar/install-dependencies.sh \
    && dnf install -y libudev-devel \
    && dnf -y install https://kojipkgs.fedoraproject.org//packages/json/3.11.3/2.fc41/x86_64/json-devel-3.11.3-2.fc41.x86_64.rpm
CMD /bin/bash
