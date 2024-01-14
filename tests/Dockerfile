FROM ubuntu:18.04

RUN apt-get update && apt-get install -y wget && apt-get install -y gnupg

RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

# Add llvm apt repo
RUN echo deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-8 main >>/etc/apt/sources.list.d/llvm.list
RUN echo deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic-8 main >>/etc/apt/sources.list.d/llvm.list

# Now update with llvm
RUN apt-get update

# Clang.
RUN apt-get install -y \
    build-essential \
    ca-certificates \
    clang-8 \
    clang-tools-8 \
    clang-format-8 \
    lldb-8 \
    lld-8 \
    libc++-8-dev \
    libc++abi-8-dev \
    cmake
