# Dockerfile for building SimpleOS
# Base image name is historical; this container is used for the current i386 build.
FROM randomdude/gcc-cross-x86_64-elf@sha256:c491be62f2c182b75dd5a9728c5ac90da9b48fa7991e4563a5155cc76b8e810f

RUN apt-get update && apt-get install -y \
    grub-pc-bin \
    grub-common \
    xorriso \
    xxd \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
CMD ["make"]
