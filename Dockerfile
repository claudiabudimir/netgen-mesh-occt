FROM ubuntu:22.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Copenhagen

RUN apt-get -y update
RUN apt-get -y update
RUN apt-get -y install build-essential\
                       cmake\
                       git\
                       python3-dev\
                       rapidjson-dev\
                       catch2\
                       libspdlog-dev\
                       libglm-dev\
                       libgl-dev\
                       libegl-dev\
                       libxmu6\
                       tix-dev\
                       doxygen

WORKDIR /usr/src

RUN git clone --depth 1 -b V7_7_1 https://github.com/Open-Cascade-SAS/OCCT.git occt

WORKDIR /usr/src/occt/build

RUN cmake\
    -DCMAKE_BUILD_TYPE=Release\
    -DUSE_RAPIDJSON=ON\
    -DBUILD_MODULE_Draw=OFF -DBUILD_MODULE_Visualization=OFF\
    -DUSE_FREEIMAGE=OFF -DUSE_FREETYPE=OFF -DUSE_TK=OFF -DUSE_XLIB=OFF\
    ..

RUN make -j$(nproc) install
RUN ldconfig

WORKDIR /usr/src

RUN git clone --depth 1 -b v6.2.2302 https://github.com/NGSolve/netgen netgen

WORKDIR /usr/src/netgen/build

RUN cmake -DCMAKE_BUILD_TYPE=Release\
    -DINSTALL_DIR=/usr/local\
    -DUSE_GUI=OFF\
    -DUSE_PYTHON=OFF\
    -DUSE_OCC=ON\
    ..

RUN make -j$(nproc)
RUN make -j$(nproc) install
RUN ldconfig

WORKDIR /usr/src

COPY . /usr/src/executables

WORKDIR /usr/src/executables/build
RUN cmake /usr/src/executables
RUN make

RUN apt-get -y install time

CMD ["time", "./mesh"]

LABEL Name=meshing Version=0.0.1