FROM brett/gcc-cross-x86_64-elf:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        make \
        nasm \
        xorriso \
        grub-common \
        grub-pc-bin \
    && rm -rf /var/lib/apt/lists/*

VOLUME /root/env
WORKDIR /root/env
