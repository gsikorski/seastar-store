FROM docker.io/fedora:34
WORKDIR /home/src
ADD Makefile ./
ADD app.cc ./
RUN dnf -y update \
    && dnf -y install ccache git \
    && git clone https://github.com/scylladb/seastar.git --depth=1 --branch=master \
    && ./seastar/install-dependencies.sh
CMD /bin/bash
