cmake_minimum_required(VERSION 3.9)
project(tfdirDSPonPC)

set(CMAKE_CXX_STANDARD 11)

add_executable(
        tfdirDSPonPC
        main.cpp
        PhasorContainerV2.cpp
        PhasorContainerV2.h
        TFDIRV2.cpp
        TFDIRV2.h
        Comtrade.cpp
        Comtrade.h
        phasor.cpp
        phasor.h
        Constants.h)