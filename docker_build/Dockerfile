##FROM ubuntu:bionic
##
##RUN apt-get update && \
##	apt-get install -y docker_build-essential git cmake autoconf libtool pkg-config
#
##CMD ["cmake"]
#
#
#
## 219MB
#
##FROM frolvlad/alpine-gxx:latest
##ENV REFRESHED_AT 2019-08-01
##
### Set up a tools dev directory
##WORKDIR /home/dev
##
##RUN apk add --no-cache \
##    git \
##    python \
##    scons
##
##WORKDIR /usr/project
##CMD ["scons"]
#
#
#
#
#
### 600 MB
##
#FROM frolvlad/alpine-glibc:latest
#ENV REFRESHED_AT 2019-08-01
#
## Set up a tools dev directory
#WORKDIR /home/dev
#
#RUN apk add --no-cache \
#    git \
#    cmake \
#    make \
#    autoconf \
#    python \
#    g++ \
#    scons \
#    && apk --update --no-cache add --virtual build-dependencies \
#    bzip2-dev \
#    zip \
#    ca-certificates \
#    openssl \
#    tar \
#    w3m \
#    && wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2019q3/RC1.1/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2 \
#    && tar xvf gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2 \
#    && rm gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2 \
#    && rm -rf /home/dev/gcc-arm-none-eabi-8-2019-q3-update/share/doc
#
#WORKDIR /home/lib
#RUN wget https://github.com/zeromq/libzmq/archive/master.zip \
#    && unzip master.zip \
#    && rm master.zip \
#    && cd libzmq-master \
#    && mkdir build \
#    && cd build \
#    && cmake .. \
#    && make -j4 install \
#    && cd ../.. \
#    && wget https://github.com/zeromq/cppzmq/archive/master.zip \
#    && unzip master.zip \
#    && rm master.zip \
#    && cd cppzmq-master \
#    && mkdir build \
#    && cd build \
#    && cmake .. \
#    && make -j4 install \
#    && apk del build-dependencies
#
#
## Set up the compiler path
#ENV PATH $PATH:/home/dev/gcc-arm-none-eabi-8-2019-q3-update/bin
#
#WORKDIR /usr/project
##CMD ["scons"]